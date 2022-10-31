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
#include <algorithm>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stream_buffer.h"
#include "semphr.h"
#include "arm_math.h"

#include "amplifier_controller.hpp"
#include "iq_calculator.hpp"
#include "semaphore.hpp"
#include "task.hpp"
#include "project_configs.hpp"
#include "queue.hpp"
#include "mcp3462_driver.hpp"

extern TickType_t time_difference;
extern int        input_task_counter;

class SensorData {
  public:
    using ValueT                  = float;
    using QueueT [[maybe_unused]] = QueueHandle_t;

    [[maybe_unused]] void SetRawValue_I(ValueT new_value_i) noexcept { rawValue_I = new_value_i; }
    [[maybe_unused]] void SetRawValue_Q(ValueT new_value_q) noexcept { rawValue_Q = new_value_q; }
    [[maybe_unused]] void SetRawAbsoluteValue(ValueT new_absolutevalue) noexcept { rawAbsoluteValue = new_absolutevalue; }
    [[maybe_unused]] void SetRawDegree(ValueT new_degree) noexcept { rawDegree = new_degree; }
    [[maybe_unused]] void SetTrueValue_I(ValueT new_true_value_i) noexcept { trueValue_I = new_true_value_i; }
    [[maybe_unused]] void SetTrueValue_Q(ValueT new_true_value_q) noexcept { trueValue_Q = new_true_value_q; }
    [[maybe_unused]] void SetTrueAbsoluteValue(ValueT new_true_absolutevalue) noexcept
    {
        trueAbsoluteValue = new_true_absolutevalue;
    }
    [[maybe_unused]] void SetTrueDegree(ValueT new_true_degree) noexcept { trueDegree = new_true_degree; }

  private:
    friend class SensorController;

    [[maybe_unused]] ValueT rawValue_I{};
    [[maybe_unused]] ValueT rawValue_Q{};
    [[maybe_unused]] ValueT rawAbsoluteValue{};
    [[maybe_unused]] ValueT rawDegree{};

    ValueT                  trueValue_I{};
    [[maybe_unused]] ValueT trueValue_Q{};
    [[maybe_unused]] ValueT trueAbsoluteValue{};
    [[maybe_unused]] ValueT trueDegree{};
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
    using ToFilterStreamBuffer = StreamBuffer<ValueT>;
    enum Configs {
        SendTimeout    = 0,
        ReceiveTimeout = portMAX_DELAY
    };

    explicit SensorController(std::unique_ptr<AmplifierController> &&new_amplifier_controller,
                              std::shared_ptr<IQCalculator>          new_iq_calculator,
                              std::shared_ptr<InputStreamBufferr>    new_input_sb,
                              std::shared_ptr<ToFilterStreamBuffer>  new_to_filter_sb)
      : inputTask{ [this]() { this->InputTask(); },
                   ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorInput),
                   ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorInput),
                   "s_input" }
      , fromFilterTask{ [this]() { this->FromFilterDataHandlerTask(); },
                        ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorFromFilter),
                        ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorFromFilter),
                        "fromFilter" }
      , inputSB{ std::move(new_input_sb) }
      , toFilterSB{ std::move(new_to_filter_sb) }
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

    void Enable() noexcept
    {
        iqCalculator->ResetTickCounter();
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
    [[maybe_unused]] void SetGain(GainT new_gain) noexcept { amplifierController->SetGain(new_gain); }
    void                  SetMinGain() noexcept { amplifierController->SetMinGain(); }
    [[maybe_unused]] void SetMaxGain() noexcept { amplifierController->SetMaxGain(); }
    void                  SuspendTasks() noexcept
    {
        inputTask.Suspend();
        fromFilterTask.Suspend();
    }
    void ResumeTasks() noexcept
    {
        inputTask.Resume();
        fromFilterTask.Resume();
    }

    [[nodiscard]] decltype(auto)        GetInputStreamBuffer() const noexcept { return inputSB; }
    [[maybe_unused]] [[nodiscard]] auto GetToFilterSB() const noexcept { return toFilterSB; }
    [[nodiscard]] QueueOwned            GetFromFilterQueueI() const noexcept { return fromFilterQueueI; }
    [[nodiscard]] SemaphoreT            GetDataReadySemaphore() const noexcept { return dataReadySemaphore; }
    [[nodiscard]] ValueT                GetValue() const noexcept { return data.trueValue_I; }
    [[maybe_unused]] [[nodiscard]] bool IsActivated() const noexcept { return isActivated; }

  protected:
    // tasks
    // todo: cleanup after tests
    [[noreturn]] [[gnu::hot]] void InputTask() noexcept
    {
        constexpr auto                                 filterFirstBufferSize = 100;
        std::array<InputValueT, filterFirstBufferSize> inputBuffer{};
        std::array<ValueT, filterFirstBufferSize>      to_filter{};

        while (true) {
            inputBuffer = inputSB->ReceiveBlocking<filterFirstBufferSize>(ReceiveTimeout);
            input_task_counter++;

//            configASSERT(adc_interrupt_counter >= 100);
            adc_interrupt_counter = 0;

            for (auto counter{ 0 }, sin_counter{ 0 }; counter < filterFirstBufferSize; counter++) {
                auto [I, Q]           = iqCalculator->CalculateIQ(inputBuffer.at(counter));
                to_filter.at(counter) = I;
            }

            toFilterSB->Send<filterFirstBufferSize>(to_filter, SendTimeout);
        }
    }
    [[noreturn]] void FromFilterDataHandlerTask()
    {
        auto from_filter_value = ValueT{};

        while (true) {
            xQueueReceive(fromFilterQueueI, &from_filter_value, ReceiveTimeout);

            data.trueValue_I = from_filter_value;

            xSemaphoreGive(dataReadySemaphore);
        }
    }

  private:
    auto constexpr static fromFilterQueueLen = 20;

    SensorData data;

    Task inputTask;
    Task fromFilterTask;

    std::shared_ptr<InputStreamBufferr>   inputSB;
    std::shared_ptr<ToFilterStreamBuffer> toFilterSB;

    QueueOwned                  fromFilterQueueI;
    [[maybe_unused]] QueueOwned fromFilterQueueQ;
    SemaphoreT                  dataReadySemaphore;

    Mutex queuesMutex;

    std::unique_ptr<AmplifierController> amplifierController;
    std::shared_ptr<IQCalculator>        iqCalculator;

    ActivationCallback   activationCallback;
    DeactivationCallback deactivationCallback;
    bool                 isActivated = false;
};
