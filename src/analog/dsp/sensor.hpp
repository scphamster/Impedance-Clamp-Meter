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

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "semphr.h"
#include "amplifier_controller.hpp"
#include "iq_calculator.hpp"
#include "semaphore.hpp"
#include "task.hpp"
#include "project_configs.hpp"

extern "C" void InputTaskWrapper(void *sensor_instance);

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
    using OwnedQueue           = QueueHandle_t;
    using ListenedQueue        = OwnedQueue;
    using ActivationCallback   = std::function<void()>;
    using DeactivationCallback = ActivationCallback;
    using SemaphoreT           = SemaphoreHandle_t;
    using IQCalculator         = SynchronousIQCalculator<ValueT>;   // todo make configurable
    using Lock                 = std::lock_guard<Mutex>;
    using GainT                = GainController::GainLevelT;

    enum Configs {
        QueueSendTimeout    = 0,
        QueueReceiveTimeout = QueueSendTimeout
    };

    explicit SensorController(std::shared_ptr<UniversalSafeType>     testvalue,
                              std::unique_ptr<AmplifierController> &&new_amplifier_controller,
                              std::shared_ptr<IQCalculator>          new_iq_calculator,
                              OwnedQueue                             new_input_queue,
                              ListenedQueue                          new_to_filter_queue)
      : /*inputTask{ [this]() { this->InputTask(); },
                   ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorInput),
                   ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorInput),
                   "s_input" }
      ,*/
      simpleSender([this]() { this->SimpleSenderTask(); }, 300, 4, "sender")
      , simpleReceiver([this]() { this->SimpleReceiverTask(); }, 300, 4, "receiver")
      , testValue{ testvalue }
//      , fromFilterTask{ [this]() { this->FromFilterDataHandlerTask(); },
//                        ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorFromFilter),
//                        ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorFromFilter),
//                        "fromFilter" }
      , inputQueue{ new_input_queue }
      , toFilterQueueI{ std::move(new_to_filter_queue) }
      , fromFilterQueueI{ xQueueCreate(fromFilterQueueLen, sizeof(ValueT)) }
      , dataReadySemaphore{ xSemaphoreCreateBinary() }
      , amplifierController{ std::forward<decltype(new_amplifier_controller)>(new_amplifier_controller) }
      , iqCalculator{ std::move(new_iq_calculator) }
    {
        // test
        //         InitializeTasks();
        InitializeSimpleTestTasks();
    }

    SensorController() = delete;
    SensorController(SensorController const &) = delete;
    SensorController &operator=(SensorController const &) = delete;

    void SetQueues(OwnedQueue to_filter_I /*, OwnedQueue to_filter_Q*/) noexcept
    {
        if (toFilterQueueI == nullptr /*or to_filter_Q == nullptr*/)
            std::terminate();

        Lock{ queuesMutex };
        toFilterQueueI = to_filter_I;
        //        toFilterQueueQ = to_filter_Q;
    }
    void Enable() noexcept
    {
        activationCallback();
        isActivated = true;
    }
    void Disable() noexcept
    {
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

    [[nodiscard]] OwnedQueue GetInputQueue() const noexcept { return inputQueue; }
    [[nodiscard]] OwnedQueue GetOutputQueue() const noexcept { return toFilterQueueI; }
    [[nodiscard]] OwnedQueue GetFromFilterQueueI() const noexcept { return fromFilterQueueI; }
    [[nodiscard]] SemaphoreT GetDataReadySemaphore() const noexcept { return dataReadySemaphore; }
    [[nodiscard]] ValueT     GetValue() const noexcept { return data.trueValue_I; }
    [[nodiscard]] bool       IsActivated() const noexcept { return isActivated; }

    [[noreturn]] void InputTask()
    {
        auto new_value = ValueT{};

        while (true) {
            xQueueReceive(inputQueue, &new_value, portMAX_DELAY);
            amplifierController->ForwardAmplitudeValueToAGCIfEnabled(new_value);

            auto [I, Q /*,todo: Degree*/] = iqCalculator->CalculateIQ(new_value);
            data.rawAbsoluteValue         = new_value;
            // todo insert all values to data struct
            Lock{ queuesMutex };
            xQueueSend(toFilterQueueI, &I, QueueSendTimeout);
            //            xQueueSend(toFilterQueueQ, &Q, QueueSendTimeout); //todo for test
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

  protected:
    void InitializeTasks() noexcept
    {
        xTaskCreate(InputTaskWrapper,
                    "input",
                    ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorInput),
                    this,
                    ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorInput),
                    nullptr);
    }

    void InitializeSimpleTestTasks() noexcept
    {
        auto constexpr testlen = 50;

        testQueue = xQueueCreate(testlen, sizeof(int));
        configASSERT(testQueue != nullptr);
    }
    [[noreturn]] void SimpleSenderTask() noexcept
    {
        auto input_value = float{};

        while (1) {
            xQueueSend(testQueue, &input_value, 0);

            input_value += 0.7;

            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    [[noreturn]] void SimpleReceiverTask() noexcept
    {
        auto output_value = float{};

        while (1) {
            xQueueReceive(testQueue, &output_value, portMAX_DELAY);

            *testValue = output_value;
        }
    }

  private:
    auto constexpr static fromFilterQueueLen = 20;

    //test
    //    Task inputTask;
    //    Task fromFilterTask;

    OwnedQueue testQueue = nullptr;
    Task       simpleSender;
    Task       simpleReceiver;
    std::shared_ptr<UniversalSafeType> testValue;

    //end test

    SensorData data;

    OwnedQueue    inputQueue;
    ListenedQueue toFilterQueueI;
    ListenedQueue toFilterQueueQ;
    OwnedQueue    fromFilterQueueI;
    OwnedQueue    fromFilterQueueQ;
    SemaphoreT    dataReadySemaphore;

    Mutex queuesMutex;

    std::unique_ptr<AmplifierController> amplifierController;
    std::shared_ptr<IQCalculator>        iqCalculator;
    ActivationCallback                   activationCallback;
    DeactivationCallback                 deactivationCallback;
    bool                                 isActivated = false;
};
