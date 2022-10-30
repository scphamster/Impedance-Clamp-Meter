#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stream_buffer.h"
#include "semphr.h"

#include "amplifier_controller.hpp"
#include "iq_calculator.hpp"
#include "semaphore.hpp"
#include "task.hpp"
#include "project_configs.hpp"
#include "queue.hpp"

class SensorData {
  public:
    using ValueT = float;
    using QueueT = QueueHandle_t;

    void SetNewValue(ValueT value) { }

    void SetRawValue_I(ValueT new_value_i) noexcept { rawValue_I = new_value_i; }
    void SetRawValue_Q(ValueT new_value_q) noexcept { rawValue_Q = new_value_q; }
    void SetRawAbsoluteValue(ValueT new_absolutevalue) noexcept { rawAbsoluteValue = new_absolutevalue; }
    void SetRawDegree(ValueT new_degree) noexcept { rawDegree = new_degree; }
    void SetTrueValue_I(ValueT new_true_value_i) noexcept { trueValue_I = new_true_value_i; }
    void SetTrueValue_Q(ValueT new_true_value_q) noexcept { trueValue_Q = new_true_value_q; }
    void SetTrueAbsoluteValue(ValueT new_true_absolutevalue) noexcept { trueAbsoluteValue = new_true_absolutevalue; }
    void SetTrueDegree(ValueT new_true_degree) noexcept { trueDegree = new_true_degree; }

  private:
    friend class SensorController;

    ValueT rawValue_I{};
    ValueT rawValue_Q{};
    ValueT rawAbsoluteValue{};
    ValueT rawDegree{};

    ValueT trueValue_I{};
    ValueT trueValue_Q{};
    ValueT trueAbsoluteValue{};
    ValueT trueDegree{};
};

class SensorController {
  public:
    using InputValueT          = int32_t;
    using ValueT               = float;
    using QueueOwned           = QueueHandle_t;
    using QueueBorrowed        = QueueOwned;
    using ActivationCallback   = std::function<void()>;
    using DeactivationCallback = ActivationCallback;
    using SemaphoreT           = SemaphoreHandle_t;
    using IQCalculator         = SynchronousIQCalculator<ValueT>;   // todo make configurable
    using Lock                 = std::lock_guard<Mutex>;
    using GainT                = GainController::GainLevelT;
    using InputStreamBufferr   = StreamBuffer<InputValueT>;
    enum Configs {
        QueueSendTimeout    = 0,
        QueueReceiveTimeout = portMAX_DELAY
    };

    explicit SensorController(std::unique_ptr<AmplifierController> &&new_amplifier_controller,
                              std::shared_ptr<IQCalculator>          new_iq_calculator,
                              std::shared_ptr<InputStreamBufferr>    new_input_sb,
                              QueueBorrowed                          new_to_filter_queue)
      : inputTask{ [this]() { this->InputTaskMsgBuffer(); },
                   ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorInput),
                   ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorInput),
                   "s_input" }
      , fromFilterTask{ [this]() { this->FromFilterDataHandlerTask(); },
                        ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorFromFilter),
                        ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorFromFilter),
                        "fromFilter" }
      , inputSB{ std::move(new_input_sb) }
      , toFilterQueueI{ std::move(new_to_filter_queue) }
      , fromFilterQueueI{ xQueueCreate(fromFilterQueueLen, sizeof(ValueT)) }
      , dataReadySemaphore{ xSemaphoreCreateBinary() }
      , amplifierController{ std::forward<decltype(new_amplifier_controller)>(new_amplifier_controller) }
      , iqCalculator{ std::move(new_iq_calculator) }
    {
        SuspendTasks();
    }

    SensorController()                                    = delete;
    SensorController(SensorController const &)            = delete;
    SensorController &operator=(SensorController const &) = delete;

    void SetQueues(QueueOwned to_filter_I /*, OwnedQueue to_filter_Q*/) noexcept
    {
        if (toFilterQueueI == nullptr /*or to_filter_Q == nullptr*/)
            std::terminate();

        Lock{ queuesMutex };
        toFilterQueueI = to_filter_I;
        //        toFilterQueueQ = to_filter_Q;
    }
    void Enable() noexcept
    {
        ResumeTasks();

        if (activationCallback)
            activationCallback();

        isActivated = true;
    }
    void Disable() noexcept
    {
        SuspendTasks();

        if (deactivationCallback)
            deactivationCallback();

        isActivated = false;
    }
    void SetOnEnableCallback(ActivationCallback &&new_callback) noexcept
    {
        activationCallback = std::forward<decltype(new_callback)>(new_callback);
    }
    void SetOnDisableCallback(DeactivationCallback &&new_callback) noexcept
    {
        deactivationCallback = std::forward<decltype(new_callback)>(new_callback);
    }
    void SetGain(GainT new_gain) noexcept { amplifierController->SetGain(new_gain); }
    void SetMinGain() noexcept { amplifierController->SetMinGain(); }
    void SetMaxGain() noexcept { amplifierController->SetMaxGain(); }
    void SuspendTasks() noexcept
    {
        inputTask.Suspend();
        fromFilterTask.Suspend();
    }
    void ResumeTasks() noexcept
    {
        inputTask.Resume();
        fromFilterTask.Resume();
    }

    [[nodiscard]] decltype(auto) GetInputStreamBuffer() const noexcept { return inputSB; }
    [[nodiscard]] QueueOwned     GetOutputQueue() const noexcept { return toFilterQueueI; }
    [[nodiscard]] QueueOwned     GetFromFilterQueueI() const noexcept { return fromFilterQueueI; }
    [[nodiscard]] SemaphoreT     GetDataReadySemaphore() const noexcept { return dataReadySemaphore; }
    [[nodiscard]] ValueT         GetValue() const noexcept { return data.trueValue_I; }
    [[nodiscard]] bool           IsActivated() const noexcept { return isActivated; }

  protected:
    [[noreturn]] void InputTaskMsgBuffer() noexcept
    {
        constexpr auto                                 filterFirstBufferSize = 100;
        std::array<InputValueT, filterFirstBufferSize> inputBuffer{};

        while (1) {
            inputBuffer = inputSB->Receive<filterFirstBufferSize>(QueueReceiveTimeout);

            for (auto const &value : inputBuffer) {
                auto [I, Q /*,todo: Degree*/] = iqCalculator->CalculateIQ(value);
                xQueueSend(toFilterQueueI, &I, QueueSendTimeout);
            }
        }
    }
    [[noreturn]] void FromFilterDataHandlerTask()
    {
        auto from_filter_value = ValueT{};

        while (true) {
            xQueueReceive(fromFilterQueueI, &from_filter_value, QueueReceiveTimeout);

            data.trueValue_I = from_filter_value;

            xSemaphoreGive(dataReadySemaphore);
        }
    }

  private:
    auto constexpr static fromFilterQueueLen = 20;

    SensorData data;

    Task inputTask;
    Task fromFilterTask;

    std::shared_ptr<InputStreamBufferr> inputSB;
    QueueBorrowed                       toFilterQueueI;
    QueueBorrowed                       toFilterQueueQ;
    QueueOwned                          fromFilterQueueI;
    QueueOwned                          fromFilterQueueQ;
    SemaphoreT                          dataReadySemaphore;

    Mutex queuesMutex;

    std::unique_ptr<AmplifierController> amplifierController;
    std::shared_ptr<IQCalculator>        iqCalculator;

    ActivationCallback   activationCallback;
    DeactivationCallback deactivationCallback;
    bool                 isActivated = false;
};
