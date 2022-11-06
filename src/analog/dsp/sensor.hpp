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
#include "DSP_functions.h"

extern TickType_t time_difference;
extern int        input_task_counter;

class SensorData {
  public:
    using ValueT                  = float;
    using QueueT [[maybe_unused]] = QueueHandle_t;

    void SetI(ValueT new_true_value_i) noexcept { Value_I = new_true_value_i; }
    void SetQ(ValueT new_true_value_q) noexcept { Value_Q = new_true_value_q; }
    void SetAbsolute(ValueT new_true_absolutevalue) noexcept { AbsoluteValue = new_true_absolutevalue; }
    void SetDegree(ValueT new_true_degree) noexcept { Degree = new_true_degree; }

    [[nodiscard]] ValueT GetI() const noexcept { return Value_I; }
    [[nodiscard]] ValueT GetQ() const noexcept { return Value_Q; }
    [[nodiscard]] ValueT GetAbsolute() const noexcept { return AbsoluteValue; }
    [[nodiscard]] ValueT GetDegree() const noexcept { return Degree; }

  private:
    friend class SensorController;

    ValueT Value_I{};
    ValueT Value_Q{};
    ValueT AbsoluteValue{};
    ValueT Degree{};
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
    using GainLevelT           = GainController::GainLevelT;
    using GainT                = GainController::GainValueT;
    using InputStreamBufferr   = StreamBuffer<InputValueT>;
    using FilterT              = SuperFilterNoTask<ValueT>;
    using Filter               = std::shared_ptr<FilterT>;
    using ToMasterQueue        = Queue<SensorData>;

    enum Configs {
        SendTimeout    = 0,
        ReceiveTimeout = portMAX_DELAY
    };
    enum Mode {
        Normal,
        Calibration
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
      , inputSB{ std::move(new_input_stream_buffer) }
      , outputQueue{ toMasterQueue }
      , filterI{ std::move(new_filter_I) }
      , filterQ{ std::move(new_filter_Q) }

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

        if (mode == Mode::Normal) {
            amplifierController->EnableAGC();
        }
        else {
            amplifierController->DisableAGC();
        }

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
    void SetMode(Mode newmode) noexcept { mode = newmode; }
    void SetTrueValuesForCalibration(ValueT magnitude, ValueT phi_degree, GainLevelT for_gain_level) noexcept
    {
        calibrationTargets.magnitude      = magnitude;
        calibrationTargets.phi            = phi_degree;
        calibrationTargets.for_gain_level = for_gain_level;
    }
    void SetOnEnableCallback(ActivationCallback &&new_callback) noexcept
    {
        activationCallback = std::forward<decltype(new_callback)>(new_callback);
    }
    void SetOnDisableCallback(DeactivationCallback &&new_callback) noexcept
    {
        deactivationCallback = std::forward<decltype(new_callback)>(new_callback);
    }
    void                  SetGain(GainT new_gain) noexcept { amplifierController->SetGain(new_gain); }
    void                  SetMinGain() noexcept { amplifierController->SetMinGain(); }
    [[maybe_unused]] void SetMaxGain() noexcept { amplifierController->SetMaxGain(); }
    void                  SuspendTasks() noexcept { inputTask.Suspend(); }
    void                  ResumeTasks() noexcept { inputTask.Resume(); }

    [[nodiscard]] decltype(auto)            GetInputStreamBuffer() const noexcept { return inputSB; }
    [[nodiscard]] SemaphoreT                GetDataReadySemaphore() const noexcept { return dataReadySemaphore; }
    [[nodiscard]] std::pair<ValueT, ValueT> GetValue() const noexcept { return { data.Value_I, data.Value_Q }; }
    [[maybe_unused]] [[nodiscard]] bool     IsActivated() const noexcept { return isActivated; }

  protected:
    // tasks
    [[noreturn]] [[gnu::hot]] void InputTask() noexcept
    {
        std::array<InputValueT, ProjectConfigs::SensorFirstFilterBufferSize> inputBuffer{};
        auto                                                                 filterI_input_buffer = filterI->GetFirstBuffer();
        auto                                                                 filterQ_input_buffer = filterQ->GetFirstBuffer();

        while (true) {
            inputBuffer = inputSB->Receive<ProjectConfigs::SensorFirstFilterBufferSize>(ReceiveTimeout);

            for (auto counter{ 0 }; auto const abs_value : inputBuffer) {
                amplifierController->ForwardAmplitudeValueToAGCIfEnabled(static_cast<ValueT>(abs_value));
                amplifierController->ForwardAmplitudeValueToAGCIfEnabled(inputBuffer.at(counter));

                auto [I, Q]                       = iqCalculator->GetSynchronousIQ(static_cast<ValueT>(abs_value));
                filterI_input_buffer->at(counter) = I;
                filterQ_input_buffer->at(counter) = Q;

                counter++;
            }

            data.SetI(filterI->DoFilter());
            data.SetQ(filterQ->DoFilter());

            // measurement task // use calibration
            if (mode == Mode::Normal) {
                auto gain                     = amplifierController->GetGainValue();
                auto Ival_nocal               = filterI->DoFilter() / gain;
                auto Qval_nocal               = filterQ->DoFilter() / gain;
                auto [absolute_value, degree] = iqCalculator->GetAbsoluteAndDegreeFromIQ(Ival_nocal, Qval_nocal);
                data.Degree                   = iqCalculator->NormalizeAngle(degree - amplifierController->GetPhaseShift());
                data.AbsoluteValue            = absolute_value;

                auto [I, Q]  = iqCalculator->GetIQFromAmplitudeAndPhase(data.AbsoluteValue, data.Degree);
                data.Value_I = I;
                data.Value_Q = Q;

                outputQueue->SendImmediate(data);
            }
            // calibration task
            else if (mode == Calibration) {
                // calculations
            }
        }
    }

  private:
    struct CalibrationTargets {
        ValueT     magnitude, phi;
        GainLevelT for_gain_level;
    };
    SensorData         data;
    CalibrationTargets calibrationTargets;

    SemaphoreT dataReadySemaphore;
    Mode       mode{ Mode::Normal };

    Task                                inputTask;
    std::shared_ptr<InputStreamBufferr> inputSB;
    std::shared_ptr<ToMasterQueue>      outputQueue;

    Filter filterI;
    Filter filterQ;

    std::unique_ptr<AmplifierController> amplifierController;
    std::shared_ptr<IQCalculator>        iqCalculator;

    ActivationCallback   activationCallback;
    DeactivationCallback deactivationCallback;
    bool                 isActivated = false;
};
