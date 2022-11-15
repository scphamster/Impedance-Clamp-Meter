#pragma once
#ifdef min
#undef min
#endif   // min
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
#include <complex>

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
    using ComplexT                = std::complex<ValueT>;

    void SetI(ValueT new_true_value_i) noexcept { value.real(new_true_value_i); }
    void SetQ(ValueT new_true_value_q) noexcept { value.imag(new_true_value_q); }
    void Set(ComplexT new_value) noexcept { value = new_value; }
    void SetAbsolute(ValueT new_true_absolutevalue) noexcept { AbsoluteValue = new_true_absolutevalue; }
    void SetDegree(ValueT new_true_degree) noexcept { degree = new_true_degree; }
    void SetDegreeFromRad(ValueT new_degree) noexcept { degree = new_degree * 180.f / PI; }

    [[nodiscard]] ValueT   GetI() const noexcept { return value.real(); }
    [[nodiscard]] ValueT   GetQ() const noexcept { return value.imag(); }
    [[nodiscard]] ComplexT GetValue() const noexcept { return value; }
    [[nodiscard]] ValueT   GetAbsolute() const noexcept { return AbsoluteValue; }
    [[nodiscard]] ValueT   GetDegree() const noexcept { return degree; }
    [[nodiscard]] ValueT   GetDegreeNocal() const noexcept { return DegreeNocal; }
    [[nodiscard]] ValueT   GetGainValue() const noexcept { return gainValue; }
    [[nodiscard]] auto     GetGainLevel() const noexcept { return gainLevel; }
    [[nodiscard]] ValueT   GetRawAbsolute() const noexcept { return rawAbsolute; }
    [[nodiscard]] bool     AgcIsEnabled() const noexcept { return agcIsEnabled; }
    [[nodiscard]] int32_t  GetAdcMax() const noexcept { return adcMax; }
    [[nodiscard]] int32_t  GetAdcMin() const noexcept { return adcMin; }

  private:
    friend class SensorController;

    ComplexT value;
    ValueT   AbsoluteValue{};
    ValueT   degree{};

    // test
    ValueT DegreeNocal{};

    ValueT gainValue;
    ValueT rawAbsolute;
    int    gainLevel;
    bool   agcIsEnabled;

    int32_t adcMax;
    int32_t adcMin;
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
    using ComplexT             = SensorData::ComplexT;
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
      , dataReadySemaphore{ xSemaphoreCreateBinary() }
      , inputSB{ std::move(new_input_stream_buffer) }
      , outputQueue{ std::move(toMasterQueue) }
      , filterI{ std::move(new_filter_I) }
      , filterQ{ std::move(new_filter_Q) }
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
            amplifierController->ForceSetGain(amplifierController->GetMinGain());
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
        amplifierController->ForceSetGain(amplifierController->GetMinGain());

        if (deactivationCallback)
            deactivationCallback();

        isActivated = false;
    }
    void SetMode(Mode newmode) noexcept
    {
        mode = newmode;
        if (mode == Mode::Normal) {
            EnableAGC();
        }
        else if (mode == Mode::Calibration) {
            DisableAGC();
        }
    }
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
    void                  SetGain(GainLevelT new_gain) noexcept { amplifierController->SetGain(new_gain); }
    void                  SetMinGain() noexcept { amplifierController->SetMinGain(); }
    void                  EnableAGC() noexcept { amplifierController->EnableAGC(); }
    void                  DisableAGC() noexcept { amplifierController->DisableAGC(); }
    [[maybe_unused]] void SetMaxGain() noexcept { amplifierController->SetMaxGain(); }
    void                  SuspendTasks() noexcept { inputTask.Suspend(); }
    void                  ResumeTasks() noexcept { inputTask.Resume(); }

    [[nodiscard]] decltype(auto)        GetInputStreamBuffer() const noexcept { return inputSB; }
    [[nodiscard]] SemaphoreT            GetDataReadySemaphore() const noexcept { return dataReadySemaphore; }
    [[maybe_unused]] [[nodiscard]] bool IsActivated() const noexcept { return isActivated; }
    [[nodiscard]] Mode                  GetMode() const noexcept { return mode; }
    std::shared_ptr<GainController>     GetGainController() const noexcept { return amplifierController->GetGainController(); }

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

                auto [I, Q]                       = iqCalculator->GetSynchronousIQ(static_cast<ValueT>(abs_value));
                filterI_input_buffer->at(counter) = I;
                filterQ_input_buffer->at(counter) = Q;

                counter++;
            }

            if (mode == Mode::Normal) {
                ComplexT from_filter{ filterI->DoFilter(), filterQ->DoFilter() };

                data.SetAbsolute(std::abs(from_filter) / amplifierController->GetGainValue());
                data.SetDegree(
                  iqCalculator->NormalizeAngle(std::arg(from_filter) * 180.f / PI - amplifierController->GetPhaseShift()));
                data.Set(iqCalculator->GetIQFromAmplitudeAndPhase(data.AbsoluteValue, data.degree));
            }

            // calibration task
            else if (mode == Calibration) {
                // test
                auto dataI         = filterI->DoFilter();
                auto dataQ         = filterQ->DoFilter();
                auto [abs, degree] = iqCalculator->GetAbsoluteAndDegreeFromIQ(dataI, dataQ);

                data.SetAbsolute(abs / calibrationTargets.magnitude);
                data.SetDegree(iqCalculator->NormalizeAngle(degree - calibrationTargets.phi));

                // todo: remove
                data.SetI(abs);
            }

            outputQueue->SendImmediate(data);
        }
    }

  private:
    struct CalibrationTargets {
        ValueT     magnitude, phi;
        GainLevelT for_gain_level;
    };
    SensorData         data;
    CalibrationTargets calibrationTargets;

    Task       inputTask;
    SemaphoreT dataReadySemaphore;

    Mode                                mode{ Mode::Normal };
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
