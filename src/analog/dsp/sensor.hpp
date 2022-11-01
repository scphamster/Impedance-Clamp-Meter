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
#include "filter.hpp"

extern TickType_t time_difference;
extern int        input_task_counter;

class SensorData {
  public:
    using ValueT                  = float;
    using QueueT [[maybe_unused]] = QueueHandle_t;

    void SetTrueValue_I(ValueT new_true_value_i) noexcept { trueValue_I = new_true_value_i; }
    void SetTrueValue_Q(ValueT new_true_value_q) noexcept { trueValue_Q = new_true_value_q; }
    void SetTrueAbsoluteValue(ValueT new_true_absolutevalue) noexcept { trueAbsoluteValue = new_true_absolutevalue; }
    void SetTrueDegree(ValueT new_true_degree) noexcept { trueDegree = new_true_degree; }

    [[nodiscard]] ValueT GetTrueValue_I() const noexcept { return trueValue_I; }
    [[nodiscard]] ValueT GetTrueValue_Q() const noexcept { return trueValue_Q; }
    [[nodiscard]] ValueT GetTrueAbsoluteValue() const noexcept { return trueAbsoluteValue; }
    [[nodiscard]] ValueT GetTrueDegree() const noexcept { return trueDegree; }

  private:
    friend class SensorController;

    ValueT trueValue_I{};
    ValueT trueValue_Q{};
    ValueT trueAbsoluteValue{};
    ValueT trueDegree{};
};

class SensorController {
  public:
    using InputValueT          = int32_t;
    using ValueT               = float;
    using ActivationCallback   = std::function<void()>;
    using DeactivationCallback = ActivationCallback;
    using SemaphoreT           = SemaphoreHandle_t;
    using IQCalculator         = SynchronousIQCalculator<ValueT>;   // todo make configurable
    using Lock                 = std::lock_guard<Mutex>;
    using GainT                = GainController::GainLevelT;
    using InputStreamBufferr   = StreamBuffer<InputValueT>;
    using FilterT              = SuperFilterNoTask<ValueT>;
    using Filter               = std::shared_ptr<FilterT>;
    using ToMasterQueue        = Queue<SensorData>;

    enum Configs {
        SendTimeout    = 0,
        ReceiveTimeout = portMAX_DELAY
    };

    explicit SensorController(std::unique_ptr<AmplifierController> &&new_amplifier_controller,
                              std::shared_ptr<IQCalculator>          new_iq_calculator,
                              std::shared_ptr<InputStreamBufferr>    new_input_stream_buffer,
                              std::shared_ptr<ToMasterQueue>         toMasterQueue,
                              Filter                                 new_filter_I,
                              Filter                                 new_filter_Q)
      : inputTask{ [this]() { this->InputTask(); },
                   ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorInput),
                   ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorInput),
                   "s_input" }
      //      , fromFilterTask{ [this]() { this->FromFilterIDataHandlerTask(); },
      //                        ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::SensorFromFilter),
      //                        ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::SensorFromFilter),
      //                        "fromFilter" }
      , inputSB{ std::move(new_input_stream_buffer) }
      , outputQueue{ toMasterQueue }
      , filterI{ std::move(new_filter_I) }
      , filterQ{ std::move(new_filter_Q) }   //      , toFilterI_SB{ std::move(new_to_filterI_SB) }
                                             //      , fromFilterI_Queue{ xQueueCreate(fromFilterQueueLen, sizeof(ValueT)) }
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
        configASSERT(inputSB->Reset());
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
    void                  SuspendTasks() noexcept { inputTask.Suspend(); }
    void                  ResumeTasks() noexcept { inputTask.Resume(); }

    [[nodiscard]] decltype(auto) GetInputStreamBuffer() const noexcept { return inputSB; }
    //    [[maybe_unused]] [[nodiscard]] auto GetToFilterSB() const noexcept { return toFilterI_SB; }
    [[nodiscard]] SemaphoreT                GetDataReadySemaphore() const noexcept { return dataReadySemaphore; }
    [[nodiscard]] std::pair<ValueT, ValueT> GetValue() const noexcept { return { data.trueValue_I, data.trueValue_Q }; }
    [[maybe_unused]] [[nodiscard]] bool     IsActivated() const noexcept { return isActivated; }

  protected:
    // tasks
    // todo: cleanup after tests
    [[noreturn]] [[gnu::hot]] void InputTask() noexcept
    {
        std::array<InputValueT, ProjectConfigs::SensorFirstFilterBufferSize> inputBuffer{};
        auto                                                                 filterI_input_buffer = filterI->GetFirstBuffer();
        auto                                                                 filterQ_input_buffer = filterQ->GetFirstBuffer();

        while (true) {
            inputBuffer = inputSB->Receive<ProjectConfigs::SensorFirstFilterBufferSize>(ReceiveTimeout);

            for (auto counter{ 0 }; auto const abs_value : inputBuffer) {
                amplifierController->ForwardAmplitudeValueToAGCIfEnabled(static_cast<ValueT>(abs_value));

                auto [I, Q] = iqCalculator->CalculateIQ(static_cast<ValueT>(abs_value));
                amplifierController->ForwardAmplitudeValueToAGCIfEnabled(inputBuffer.at(counter));
                filterI_input_buffer->at(counter) = I;
                filterQ_input_buffer->at(counter) = Q;

                counter++;
            }

            auto gain        = amplifierController->GetGainValue();
            data.trueValue_I = filterI->DoFilter() / gain;
            data.trueValue_Q = filterQ->DoFilter() / gain;

            auto [absolute_value, degree] = iqCalculator->GetAbsoluteAndDegreeFromIQ(data.trueValue_I, data.trueValue_Q);
            data.trueDegree               = degree - amplifierController->GetPhaseShift();
            data.trueAbsoluteValue        = absolute_value;

//            xSemaphoreGive(dataReadySemaphore);

            outputQueue->SendImmediate(data);
        }
    }

  private:
    SensorData data;
    SemaphoreT dataReadySemaphore;

    Task                                inputTask;
    std::shared_ptr<InputStreamBufferr> inputSB;
    std::shared_ptr<ToMasterQueue>      outputQueue;

    Filter filterI;
    Filter filterQ;

    // todo: use queue instead of semaphore
    std::unique_ptr<AmplifierController> amplifierController;
    std::shared_ptr<IQCalculator>        iqCalculator;

    ActivationCallback   activationCallback;
    DeactivationCallback deactivationCallback;
    bool                 isActivated = false;
};
