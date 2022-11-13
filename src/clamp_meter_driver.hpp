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

#include <memory>
#include <functional>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pio.h"
#include "semphr.h"

#include "project_configs.hpp"
#include "mcp3462_driver.hpp"
#include "HVPowerSupply.hpp"
#include "OutputRelay.hpp"
#include "output_generator.hpp"
#include "sensor_preamp.hpp"
#include "filter.hpp"
#include "external_periph_ctrl.h"
#include "signal_conditioning.h"
#include "sensor.hpp"
#include "menu_model_item.hpp"
#include "iq_calculator.hpp"
#include "task.hpp"
#include "project_configs.hpp"
#include "freertos_handlers.h"

#include "DG442.hpp"
#include "analog_switch.hpp"
#include "dsp_resources.hpp"
#include "menu_model_dialog.hpp"
#include "semaphore/semaphore.hpp"
#include "deviation_calculator.hpp"
#include "flash_controller.hpp"

#include "static_string.hpp"

// todo: remove from here
#include "signal_conditioning.h"
// todo: cleanup

extern "C" char *gcvtf(float, int, char *);

extern std::array<std::pair<float, float>, 21> calibration_data;
extern bool                                    flash_restore_result;
extern bool                                    flash_save_result;
std::array<std::pair<float, float>, 21> static constexpr reserve_calibration{
    // voltage sensor
    std::pair{ 60314.3242f, 161.143814f },
    // shunt sensor
    std::pair{ 53784.875f / 3, 90.9220352f },
    std::pair{ 53784.875f, 90.9220352f },
    std::pair{ 132666.203f, 90.9662323f },
    std::pair{ 264311.406f, 91.0643845f },
    std::pair{ 8696278.f, 90.829567f },
    std::pair{ 17165270.f, 90.9678421f },
    std::pair{ 17165270.f * 2, 90.9678421f },
    std::pair{ 17165270.f * 4, 90.9678421f },
    std::pair{ 17165270.f * 8, 90.9678421f },
    std::pair{ 17165270.f * 16, 90.9678421f },
    // clamp sensor
    std::pair{ 338750944.f / 3, 255.35321f },
    std::pair{ 338750944.f, 255.35321f },
    std::pair{ 645587648.f, 90.9662323f },
    std::pair{ 955409024.f, 91.0643845f },
    std::pair{ 1.53519985e+010f, 90.829567f },
    std::pair{ 1.50437837e+010f, 90.9678421f },
    std::pair{ 1.50437837e+010f * 2, 90.9678421f },
    std::pair{ 1.50437837e+010f * 4, 90.9678421f },
    std::pair{ 1.50437837e+010f * 8, 90.9678421f },
    std::pair{ 1.50437837e+010f * 16, 90.9678421f },
};

template<size_t SensorsGainLevelsNumber>
class CalibrationData {
  public:
    using ValueT       = float;
    using DataPair     = std::pair<ValueT, ValueT>;
    using ClampDataT   = std::array<DataPair, SensorsGainLevelsNumber>;
    using ShuntDataT   = ClampDataT;
    using VoltageDataT = DataPair;
    using GainLevelT   = int;

    void SetVoltageSensorData(VoltageDataT new_cals) noexcept { voltage = new_cals; }
    void InsertShuntSensorData(GainLevelT for_gain_level, DataPair new_cals) noexcept { shunt.at(for_gain_level - 1) = new_cals; }
    void InsertClampSensorData(GainLevelT for_gain_level, DataPair new_cals) noexcept { clamp.at(for_gain_level - 1) = new_cals; }

    [[nodiscard]] DataPair GetVoltageSensorCalData() const noexcept { return voltage; }
    [[nodiscard]] DataPair GetShuntSensorCalData(GainLevelT for_gain_level) const noexcept { return shunt.at(for_gain_level - 1); }
    [[nodiscard]] DataPair GetClampSensorCalData(GainLevelT for_gain_level) const noexcept { return clamp.at(for_gain_level - 1); }
    [[nodiscard]] ShuntDataT GetShuntSensorCalData() const noexcept { return shunt; }
    [[nodiscard]] ClampDataT GetClampSensorCalData() const noexcept { return clamp; }

    bool operator==(CalibrationData const &other) const noexcept
    {
        if (voltage == other.voltage and shunt == other.shunt and clamp == other.clamp)
            return true;
    }

  private:
    VoltageDataT voltage{};
    ShuntDataT   shunt{};
    ClampDataT   clamp{};
};

class ClampMeterDriver {
  public:
    using ValueT                        = float;
    using DataType                      = std::pair<ValueT, ValueT>;
    using SensorDataInsertionCallback   = std::function<void()>;
    using CalculationCompletionCallback = std::function<void()>;
    using AdcDriverT                    = MCP3462_driver;
    using AdcValueT                     = AdcDriverT::ValueT;
    using SensorPreampT                 = SensorPreamp<AdcDriverT::ValueT>;
    using FilterT                       = SuperFilterNoTask<ValueT>;
    using Filter                        = std::shared_ptr<FilterT>;
    using FromSensorQueueT              = Queue<SensorData>;
    using DialogT                       = MenuModelDialog;

    using CalibrationDataT      = std::array<std::pair<ValueT, ValueT>, 21>;
    using ShuntCalibrationDataT = std::array<std::pair<ValueT, ValueT>, 10>;
    using ClampCalibrationDataT = std::array<std::pair<ValueT, ValueT>, 10>;

    using FlashController2 = FlashController<CalibrationData<10>, uint32_t>;

    enum Configs {
        NumberOfSensors                 = 3,
        NumberOfSensorPreamps           = 2,
        QueueReceiveFromADCTimeout      = portMAX_DELAY,
        SemaphoreTakeTimeout            = portMAX_DELAY,
        PowerSupplyDelayAfterActivation = 300
    };
    enum class Sensor {
        Shunt = 0,
        Clamp,
        Voltage
    };
    enum class Mode {
        Normal,
        Calibration
    };

    ClampMeterDriver(std::shared_ptr<UniversalSafeType> vout,
                     std::shared_ptr<UniversalSafeType> shunt,
                     std::shared_ptr<UniversalSafeType> clamp,
                     std::shared_ptr<UniversalSafeType> v4,
                     std::shared_ptr<DialogT>           new_msg_box)
      : value1{ vout }
      , value2{ shunt }
      , value3{ clamp }
      , value4{ v4 }
      , generator{ ProjectConfigs::GeneratorAmplitude }
      , adc{ ProjectConfigs::ADCAddress, ProjectConfigs::ADCStreamBufferCapacity, ProjectConfigs::ADCStreamBufferTriggeringSize }
      , filterI{ std::make_shared<FilterT>() }
      , filterQ{ std::make_shared<FilterT>() }
      , fromSensorDataQueue{ std::make_shared<FromSensorQueueT>(ProjectConfigs::FromSensorOutputQueueLength, "sensor") }
      , sensorDataManagerTask{ [this]() { this->ManageSensorsDataTask(); },
                               ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::ClampDriverSensor),
                               ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::ClampDriverSensor),
                               "sensors" }
      , calibrationTask{ [this]() { this->CalibrationTask(); },
                         ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::ClampDriverCalibration),
                         ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::ClampDriverCalibration),
                         "calibration" }
      , messageBox{ std::move(new_msg_box) }
    {
        calibrationTask.Suspend();

        InitializeFilters();
        InitializeSensors();

        SynchronizeAdcAndDac();
    }

    void SwitchToNextSensor() noexcept
    {
        switch (activeSensor) {
        case Sensor::Voltage: SetAndActivateSensor(Sensor::Shunt); break;
        case Sensor::Shunt: SetAndActivateSensor(Sensor::Clamp); break;
        case Sensor::Clamp: SetAndActivateSensor(Sensor::Voltage); break;
        }
    }

    void StartNormalModeOperation() noexcept
    {
        messageBox->ShowMsg("Calibrated? " + std::to_string(isCalibrated));

        workMode = Mode::Normal;
        SetAndActivateSensor(Sensor::Voltage);

        for (auto &[sensor_id, sensor] : sensors) {
            sensor.SetMode(SensorController::Mode::Normal);
        }

        StartMeasurements();
    }
    void StartCalibration() noexcept
    {
        workMode = Mode::Calibration;
        SetAndActivateSensor(Sensor::Voltage);
        for (auto &[sensor_id, sensor] : sensors) {
            sensor.SetMode(SensorController::Mode::Calibration);
        }

        calibrationTask.Resume();
    }
    void Stop() noexcept
    {
        calibrationTask.Suspend();
        StopMeasurements();
    }

    // test: debug functions
    void DisableAGC() noexcept
    {
        auto [calibs, result] = FlashController2::Recall(COEFFS_FLASH_START_ADDR);

        messageBox->ShowMsg("AGC disabled for current sensor");
        sensors.at(activeSensor).DisableAGC();
    }
    void IncreaseGain() noexcept
    {
        sensors.at(activeSensor).GetGainController()->IncreaseGain();
        messageBox->ShowMsg("agcGain=" + std::to_string(sensors.at(activeSensor).GetGainController()->GetGainLevel()));
    }
    void DecreaseGain() noexcept
    {
        sensors.at(activeSensor).GetGainController()->DecreaseGain();
        messageBox->ShowMsg("agcGain=" + std::to_string(sensors.at(activeSensor).GetGainController()->GetGainLevel()));
    }

  protected:
    class PeripheralsController {
      public:
        PeripheralsController()
          : powerSupply{ PIOD, ID_PIOD, 31 }
          , outputRelay{ PIOA, ID_PIOA, 21 }
        { }

      private:
        friend class ClampMeterDriver;
        HVPowerSupply powerSupply;
        OutputRelay   outputRelay;
    };

    void StandardSensorOnEnableCallback() noexcept
    {
        using UnderType = std::underlying_type_t<Sensor>;

        adc.SetOutputStreamBuffer(sensors.at(activeSensor).GetInputStreamBuffer());
        adc.SetMux(adcChannelsMuxSettings.at(static_cast<UnderType>(activeSensor)).first,
                   adcChannelsMuxSettings.at(static_cast<UnderType>(activeSensor)).second);
    }
    void StandardSensorOnDisableCallback() noexcept
    { /*sensors.at(activeSensor).SetMinGain();*/
    }

    void InitializeFilters() noexcept
    {
        auto constexpr filter2_block_size = 100;

        auto constexpr biquad1_nstages    = 2;
        auto constexpr biquad1_ncoeffs    = biquad1_nstages * 5;
        auto constexpr filter1_block_size = filter2_block_size;

        auto constexpr fir1_dec_ncoeffs  = 5;
        auto constexpr decimating_factor = 10;

        auto constexpr filter4_block_size = 10;
        auto constexpr fir2_dec_ncoeffs   = 5;

        auto constexpr biquad2_nstages    = 2;
        auto constexpr biquad2_ncoeffs    = biquad2_nstages * 5;
        auto constexpr filter3_block_size = filter4_block_size;

        auto constexpr biquad3_nstages    = 3;
        auto constexpr biquad3_ncoeffs    = biquad3_nstages * 5;
        auto constexpr filter5_block_size = 1;

        using CoefficientsPack = AbstractFilter::CoefficientsPack;
        auto sin_filter1       = std::make_unique<BiquadCascadeDF2TFilter>(CoefficientsPack{ 0.00018072660895995795726776123046875f,
                                                                                       0.0003614532179199159145355224609375f,
                                                                                       0.00018072660895995795726776123046875f,

                                                                                       1.9746592044830322265625f,
                                                                                       -0.975681483745574951171875f,

                                                                                       0.00035528256557881832122802734375f,
                                                                                       0.0007105651311576366424560546875f,
                                                                                       0.00035528256557881832122802734375f,

                                                                                       1.94127738475799560546875f,
                                                                                       -0.9422824382781982421875f },
                                                                     filter1_block_size);

        auto sin_filter2 = std::make_unique<FirDecimatingFilter>(CoefficientsPack{ 0.02868781797587871551513671875f,
                                                                                   0.25f,
                                                                                   0.4426243603229522705078125f,
                                                                                   0.25f,
                                                                                   0.02868781797587871551513671875f },
                                                                 decimating_factor,
                                                                 filter2_block_size);

        auto sin_filter3 = std::make_unique<BiquadCascadeDF2TFilter>(CoefficientsPack{ 0.000180378541699610650539398193359375f,
                                                                                       0.00036075708339922130107879638671875f,
                                                                                       0.000180378541699610650539398193359375f,

                                                                                       1.97468459606170654296875f,
                                                                                       -0.975704848766326904296875f,

                                                                                       0.00035460057551972568035125732421875f,
                                                                                       0.0007092011510394513607025146484375f,
                                                                                       0.00035460057551972568035125732421875f,

                                                                                       1.941333770751953125f,
                                                                                       -0.942336857318878173828125f },
                                                                     filter3_block_size);

        auto sin_filter4 = std::make_unique<FirDecimatingFilter>(CoefficientsPack{ 0.02867834083735942840576171875f,
                                                                                   0.25f,
                                                                                   0.4426433145999908447265625f,
                                                                                   0.25f,
                                                                                   0.02867834083735942840576171875f },
                                                                 decimating_factor,
                                                                 filter4_block_size);

        auto sin_filter5 = std::make_unique<BiquadCascadeDF2TFilter>(CoefficientsPack{ 0.0000231437370530329644680023193359375f,
                                                                                       0.000046287474106065928936004638671875f,
                                                                                       0.0000231437370530329644680023193359375f,

                                                                                       1.98140633106231689453125f,
                                                                                       -0.981498897075653076171875f,

                                                                                       0.000020184184904792346060276031494140625f,
                                                                                       0.00004036836980958469212055206298828125f,
                                                                                       0.000020184184904792346060276031494140625f,

                                                                                       1.99491560459136962890625f,
                                                                                       -0.99500882625579833984375f,

                                                                                       0.000026784562578541226685047149658203125f,
                                                                                       0.00005356912515708245337009429931640625f,
                                                                                       0.000026784562578541226685047149658203125f,

                                                                                       1.9863297939300537109375f,
                                                                                       -0.986422598361968994140625f },
                                                                     filter5_block_size);

        auto cos_filter1 = std::make_unique<BiquadCascadeDF2TFilter>(*sin_filter1);
        auto cos_filter2 = std::make_unique<FirDecimatingFilter>(*sin_filter2);
        auto cos_filter3 = std::make_unique<BiquadCascadeDF2TFilter>(*sin_filter3);
        auto cos_filter4 = std::make_unique<FirDecimatingFilter>(*sin_filter4);
        auto cos_filter5 = std::make_unique<BiquadCascadeDF2TFilter>(*sin_filter5);

        filterI->InsertFilter(std::move(sin_filter1));
        filterI->InsertFilter(std::move(sin_filter2));
        filterI->InsertFilter(std::move(sin_filter3));
        filterI->InsertFilter(std::move(sin_filter4));
        filterI->InsertFilter(std::move(sin_filter5));

        filterQ->InsertFilter(std::move(cos_filter1));
        filterQ->InsertFilter(std::move(cos_filter2));
        filterQ->InsertFilter(std::move(cos_filter3));
        filterQ->InsertFilter(std::move(cos_filter4));
        filterQ->InsertFilter(std::move(cos_filter5));
    }
    void InitializeIO() noexcept
    {
        pio_set_input(PIOA, SH_SENSOR_GAIN_A_PIN | CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, 0);

        pio_set_input(PIOD, CLAMP_SENSOR_GAIN_A_PIN | SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, 0);

        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);

        pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);

        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
    }
    void InitializeSensors() noexcept
    {
        using AGC          = AutomaticGainController<GainController, float>;
        using GainT        = GainController::GainLevelT;
        auto iq_controller = std::make_shared<SynchronousIQCalculator<float>>(sinus_table.size());

        // todo: make this initializer part of gain controllers
        InitializeIO();

        auto [calibs, result] = FlashController2::Recall(COEFFS_FLASH_START_ADDR);

        // todo: clean after debug
        CalibrationDataT cal{
            calibs.GetVoltageSensorCalData(), calibs.GetShuntSensorCalData(1),  calibs.GetShuntSensorCalData(2),
            calibs.GetShuntSensorCalData(3),  calibs.GetShuntSensorCalData(4),  calibs.GetShuntSensorCalData(5),
            calibs.GetShuntSensorCalData(6),  calibs.GetShuntSensorCalData(7),  calibs.GetShuntSensorCalData(8),
            calibs.GetShuntSensorCalData(9),  calibs.GetShuntSensorCalData(10), calibs.GetClampSensorCalData(1),
            calibs.GetClampSensorCalData(2),  calibs.GetClampSensorCalData(3),  calibs.GetClampSensorCalData(4),
            calibs.GetClampSensorCalData(5),  calibs.GetClampSensorCalData(6),  calibs.GetClampSensorCalData(7),
            calibs.GetClampSensorCalData(8),  calibs.GetClampSensorCalData(9),  calibs.GetClampSensorCalData(10),
        };

        result = false;

        if (not result) {
            cal          = reserve_calibration;
            isCalibrated = false;
        }
        else {
            isCalibrated = true;
        }

        // Voltage sensor
        {
            auto v_gain_controller = std::make_shared<GainController>(1, 1);
            v_gain_controller->SetGainChangeFunctor(
              1,
              [this]() { adc.SetGain(MCP3462_driver::Gain::GAIN_1); },
              cal.at(0));
            auto v_amplifier_controller        = std::make_unique<AmplifierController>(std::move(v_gain_controller));
            auto [_unused_, emplace_succeeded] = sensors.emplace(std::piecewise_construct,
                                                                 std::forward_as_tuple(Sensor::Voltage),
                                                                 std::forward_as_tuple(std::move(v_amplifier_controller),
                                                                                       iq_controller,
                                                                                       adc.CreateNewStreamBuffer(),
                                                                                       fromSensorDataQueue,
                                                                                       filterI,
                                                                                       filterQ));

            configASSERT(emplace_succeeded);

            sensors.at(Sensor::Voltage).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
        }

        // Shunt sensor
        {
            auto sh_gain_controller = std::make_shared<GainController>(1, 10);
            // clang-format off
            sh_gain_controller->SetGainChangeFunctor(1, [this]() { shunt_sensor_set_gain(0);  adc.SetGain(AdcDriverT::Gain::GAIN_1V3);},  cal.at(1));
            sh_gain_controller->SetGainChangeFunctor(2, [this]() { shunt_sensor_set_gain(0); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  cal.at(2));
            sh_gain_controller->SetGainChangeFunctor(3, [this]() { shunt_sensor_set_gain(1); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  cal.at(3));
            sh_gain_controller->SetGainChangeFunctor(4, [this]() { shunt_sensor_set_gain(2); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  cal.at(4));
            sh_gain_controller->SetGainChangeFunctor(5, [this]() { shunt_sensor_set_gain(3); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  cal.at(5));
            sh_gain_controller->SetGainChangeFunctor(6, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  cal.at(6));
            sh_gain_controller->SetGainChangeFunctor(7, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_2);}   ,  cal.at(7));
            sh_gain_controller->SetGainChangeFunctor(8, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_4);}   ,  cal.at(8));
            sh_gain_controller->SetGainChangeFunctor(9, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_8);}   ,  cal.at(9));
            sh_gain_controller->SetGainChangeFunctor(10, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_16);} ,  cal.at(10));
            // clang-format on
            auto sh_agc                  = std::make_unique<AGC>(1, 10, 100000UL, 3000000UL, 50, 500);
            auto sh_amplifier_controller = std::make_unique<AmplifierController>(sh_gain_controller, std::move(sh_agc), true);
            sensors.emplace(std::piecewise_construct,
                            std::forward_as_tuple(Sensor::Shunt),
                            std::forward_as_tuple(std::move(sh_amplifier_controller),
                                                  iq_controller,
                                                  adc.CreateNewStreamBuffer(),
                                                  fromSensorDataQueue,
                                                  filterI,
                                                  filterQ));
            sensors.at(Sensor::Shunt).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
            sensors.at(Sensor::Shunt).SetOnDisableCallback([this]() { this->StandardSensorOnDisableCallback(); });
        }

        // Clamp sensor
        {
            auto clamp_gain_controller = std::make_shared<GainController>(1, 10);
            // clang-format off
            clamp_gain_controller->SetGainChangeFunctor(1, [this]() {clamp_sensor_set_gain(0);adc.SetGain(AdcDriverT::Gain::GAIN_1V3);}, cal.at(11));
            clamp_gain_controller->SetGainChangeFunctor(2, [this]() {clamp_sensor_set_gain(0);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , cal.at(12));
            clamp_gain_controller->SetGainChangeFunctor(3, [this]() {clamp_sensor_set_gain(1);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , cal.at(13));
            clamp_gain_controller->SetGainChangeFunctor(4, [this]() {clamp_sensor_set_gain(2);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , cal.at(14));
            clamp_gain_controller->SetGainChangeFunctor(5, [this]() {clamp_sensor_set_gain(3);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , cal.at(15));
            clamp_gain_controller->SetGainChangeFunctor(6, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , cal.at(16));
            clamp_gain_controller->SetGainChangeFunctor(7, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_2);}  , cal.at(17));
            clamp_gain_controller->SetGainChangeFunctor(8, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_4);}  , cal.at(18));
            clamp_gain_controller->SetGainChangeFunctor(9, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_8);}  , cal.at(19));
            clamp_gain_controller->SetGainChangeFunctor(10, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_16);}, cal.at(20));
            // clang-format on
            auto clamp_agc = std::make_unique<AGC>(1, 10, 100000UL, 3000000UL, 50, 500);
            auto clamp_amplifier_controller =
              std::make_unique<AmplifierController>(clamp_gain_controller, std::move(clamp_agc), true);
            sensors.emplace(std::piecewise_construct,
                            std::forward_as_tuple(Sensor::Clamp),
                            std::forward_as_tuple(std::move(clamp_amplifier_controller),
                                                  iq_controller,
                                                  adc.CreateNewStreamBuffer(),
                                                  fromSensorDataQueue,
                                                  filterI,
                                                  filterQ));
            sensors.at(Sensor::Clamp).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
            sensors.at(Sensor::Clamp).SetOnDisableCallback([this]() { this->StandardSensorOnDisableCallback(); });
        }
    }

    void StartMeasurements() noexcept
    {
        // todo: move this initialization from here
        if (firstMeasurementsStart) {
            // todo: make this procedure more pretty
            SetAndActivateSensor(Sensor::Voltage);
            adc.StartSynchronousMeasurements();
            Task::DelayMs(100);
            adc.StopMeasurement();
            Task::DelayMs(200);

            firstMeasurementsStart = false;
        }

        SetAndActivateSensor(activeSensor);

        peripherals.powerSupply.Activate();
        Task::DelayMs(PowerSupplyDelayAfterActivation);

        Task::SuspendAll();
        adc.StartSynchronousMeasurements();
        generator.StartGenerating();
        Task::ResumeAll();

        peripherals.outputRelay.Activate();
    }
    void StopMeasurements() noexcept
    {
        // test sensor deactivation moved to end
        adc.StopMeasurement();
        Task::DelayMs(100);
        sensors.at(activeSensor).Disable();
        Task::DelayMs(10);

        peripherals.outputRelay.Deactivate();
        peripherals.powerSupply.Deactivate();
        generator.StopGenerating();
    }
    void SetAndActivateSensor(Sensor new_sensor) noexcept
    {
        sensors.at(activeSensor).Disable();
        activeSensor = new_sensor;
        sensors.at(activeSensor).Enable();
    }

    void SynchronizeAdcAndDac() noexcept
    {
        generator.SetFirstTimeInterruptCallback([this]() { adc.SynchronizationCallback(); });
    }

    // task helpers
    void CheckDataStability(ValueT data) noexcept
    {
        deviationCalc.PushBackAndCalculate(data);
        devValue = deviationCalc.GetRelativeStdDev();
        devdevCalc.PushBackAndCalculate(deviationCalc.GetRelativeStdDev());
        deviationOfDeviation = devdevCalc.GetStandardDeviation();
    }
    void CalculateAppliedVoltage() noexcept
    {
        data.AppliedVoltageI = data.voltageSensorData.GetI() - data.shuntSensorData.GetI();
        data.AppliedVoltageQ = data.voltageSensorData.GetQ() - data.shuntSensorData.GetQ();
        arm_sqrt_f32(data.AppliedVoltageI * data.AppliedVoltageI + data.AppliedVoltageQ * data.AppliedVoltageQ,
                     &data.AppliedVoltage);

        data.AppliedVoltagePhi =
          SynchronousIQCalculator<ValueT>::FindAngle(data.AppliedVoltageI, data.AppliedVoltageQ, data.AppliedVoltage);
    }
    void CalculateVoltageSensor() noexcept
    {
        if (workMode == Mode::Calibration)
            return;

        *value1 = data.voltageSensorData.GetAbsolute();
    }
    void CalculateShuntSensor() noexcept
    {
        if (workMode == Mode::Calibration)
            return;

        CalculateAppliedVoltage();

        auto admitance_mag = data.shuntSensorData.GetAbsolute() / (shuntResistorValue * data.AppliedVoltage);
        auto admitance_phi = data.shuntSensorData.GetDegree() - data.AppliedVoltagePhi;

        auto [sin, cos] = SynchronousIQCalculator<ValueT>::GetSinCosFromAngle(admitance_phi);

        auto conductance = admitance_mag * cos;
        auto susceptance = admitance_mag * sin;

        data.ROverall    = 1 / conductance;
        data.XOverall    = 1 / susceptance;
        data.ZOverall    = 1 / admitance_mag;
        data.ZOverallPhi = admitance_phi;

        *value2 = data.ZOverall;
    }
    void CalculateClampSensor() noexcept
    {
        if (workMode == Mode::Calibration)
            return;

        data.clampSensorData.SetDegree(data.clampSensorData.GetDegree() - data.AppliedVoltagePhi -
                                       data.voltageSensorData.GetDegree());

        data.ZClamp     = data.AppliedVoltage / data.clampSensorData.GetAbsolute();
        auto [sin, cos] = SynchronousIQCalculator<ValueT>::GetSinCosFromAngle(data.clampSensorData.GetDegree());

        data.RClamp = data.AppliedVoltage / (data.clampSensorData.GetI() * cos);
        data.XClamp = data.AppliedVoltage / (data.clampSensorData.GetQ() * sin);

        *value3 = data.ZClamp;
    }

    void WaitForStableData(ValueT max_deviation) noexcept
    {
        Task::DelayMs(2000);

        for (;;) {
            if (deviationOfDeviation < 0)
                deviationOfDeviation = -deviationOfDeviation;

            if (deviationOfDeviation > max_deviation)
                Task::DelayMs(100);
            else
                break;
        }
        Task::DelayMs(3000);
    }
    void RequestResistorChangeAndCheckVout(int gain_level) noexcept
    {
        messageBox->ShowMsg("Set Resistor " + std::to_string(calibrationResistances.at(gain_level - 1)) +
                            " Ohm at output and press Enter");
        messageBox->WaitForUserReaction();

        sensors.at(Sensor::Voltage).SetMode(SensorController::Mode::Normal);
        messageBox->ShowMsg("Gain level " + std::to_string(gain_level) + " Calibration, waiting for stable output voltage...");

        StartMeasurements();
        WaitForStableData(5e-7);
        StopMeasurements();
    }

    // todo: hide this from here
    void SetSensorCalibrationTargets(Sensor sensor, auto gain_level) noexcept
    {
        switch (sensor) {
        case Sensor::Voltage: return;
        case Sensor::Shunt:
            sensors.at(sensor).SetTrueValuesForCalibration(
              (data.voltageSensorData.GetAbsolute() / (calibrationResistances.at(gain_level - 1) + shuntResistorValue)) *
                shuntResistorValue,
              data.voltageSensorData.GetDegree(),
              gain_level);
            break;

        case Sensor::Clamp:
            sensors.at(sensor).SetTrueValuesForCalibration(data.voltageSensorData.GetAbsolute() /
                                                             (calibrationResistances.at(gain_level - 1) + shuntResistorValue),
                                                           data.voltageSensorData.GetDegree(),
                                                           gain_level);
            break;
        }
    }

    std::pair<ValueT, ValueT> CalibrateSensor(Sensor sensor, int gain_level, ShuntCalibrationDataT &cal) noexcept
    {
        SetAndActivateSensor(sensor);
        sensors.at(sensor).SetMode(SensorController::Calibration);
        sensors.at(sensor).SetGain(gain_level);

        SetSensorCalibrationTargets(sensor, gain_level);

        ValueT deviationTarget = (sensor == Sensor::Shunt) ? shuntSensorDeviationTargets.at(gain_level - 1)
                                                           : clampCalibrationDeviationTargets.at(gain_level - 1);

        std::array<char, 10> string_buffer;
        gcvtf(deviationTarget, 4, string_buffer.data());

        std::string msg{ "Step " };
        msg.append(std::to_string(gain_level));
        std::string sensor_name = (sensor == Sensor::Shunt) ? "Shunt" : "Clamp";

        msg.append("/10, Sensor: " + sensor_name + " Deviation^2 target is:" + string_buffer.data());
        messageBox->ShowMsg(std::move(msg));

        StartMeasurements();
        WaitForStableData(deviationTarget);

        messageBox->ShowMsg("End of sensor calibration, gain level at end is:" +
                            std::to_string(sensors.at(sensor).GetGainController()->GetGainLevel()));
        StopMeasurements();

        Task::DelayMs(3000);

        ValueT true_gain, true_phase_shift;

        if (sensor == Sensor::Shunt) {
            true_gain        = data.shuntSensorData.GetAbsolute();
            true_phase_shift = data.shuntSensorData.GetDegree();

            sensors.at(Sensor::Shunt).GetGainController()->SetGainValueAndPhaseShift(gain_level, true_gain, true_phase_shift);
        }
        else {
            true_gain        = data.clampSensorData.GetAbsolute();
            true_phase_shift = data.clampSensorData.GetDegree();

            sensors.at(Sensor::Clamp).GetGainController()->SetGainValueAndPhaseShift(gain_level, true_gain, true_phase_shift);
        }

        return { true_gain, true_phase_shift };
    }

    // tasks
    [[noreturn]] void CalibrationTask() noexcept
    {
        while (true) {
            // todo: implement data input from user
            auto request_resistor_change_and_check_vout = [this](int gain_level) noexcept {
                auto msg_step_header = "Step " + std::to_string(gain_level) + "/10";

                std::array<char, 6> string_buffer;
                gcvtf(calibrationResistances.at(gain_level - 1), 4, string_buffer.data());

                messageBox->ShowMsg(msg_step_header + " Set Resistor " + string_buffer.data() +
                                    "Ohm between terminals and press Enter");
                messageBox->WaitForUserReaction();

                sensors.at(Sensor::Voltage).SetGain(1);
                sensors.at(Sensor::Voltage).SetMode(SensorController::Mode::Normal);
                SetAndActivateSensor(Sensor::Voltage);
                messageBox->ShowMsg(msg_step_header +
                                    " waiting for stable output voltage... ADC Gain=" + std::to_string(adc.GetGainLevel()));

                StartMeasurements();
                WaitForStableData(voltageSensorDeviationTarget);
                StopMeasurements();
            };

            auto constexpr backup_vOut_value = 36.47f;
            CalibrationData<gainLevelsQuantity> cal_data;
            ShuntCalibrationDataT               shunt_calibration_data;
            ClampCalibrationDataT               clamp_calibration_data;

            // voltage sensor calibration
            {
                SetAndActivateSensor(Sensor::Voltage);
                sensors.at(Sensor::Voltage).SetMode(SensorController::Mode::Calibration);
                sensors.at(Sensor::Voltage).SetGain(1);
                StartMeasurements();

                // request measured vout from user
                auto input_value = std::make_shared<UniversalSafeType>(backup_vOut_value);
                messageBox->SetType(MenuModelDialog::DialogType::InputBox);
                messageBox->SetValue(input_value);
                messageBox->ShowMsg("Insert output voltage value:");
                messageBox->WaitForUserReaction();

                sensors.at(Sensor::Voltage).SetTrueValuesForCalibration(std::get<ValueT>(input_value->GetValue()), 0, 0);

                messageBox->SetHasValue(false);
                messageBox->ShowMsg("Waiting for stable vout... ADC gain = " + std::to_string(adc.GetGainLevel()));

                WaitForStableData(voltageSensorDeviationTarget);
                StopMeasurements();

                cal_data.SetVoltageSensorData({ data.voltageSensorData.GetAbsolute(), data.voltageSensorData.GetDegree() });
                sensors.at(Sensor::Voltage).GetGainController()->SetGainValueAndPhaseShift(1, cal_data.GetVoltageSensorCalData());

                messageBox->ShowMsg("Voltage sensor calibration completed.");
                Task::DelayMs(1000);
            }

            for (auto gain_level = 1; gain_level < gainLevelsQuantity + 1; gain_level++) {
                if (gain_level > 1) {
                    auto diff = calibrationResistances.at(gain_level - 1) - calibrationResistances.at(gain_level - 2);
                    if (diff < 0)
                        diff = -diff;

                    if (diff > 1)
                        request_resistor_change_and_check_vout(gain_level);
                }
                else
                    request_resistor_change_and_check_vout(gain_level);

                cal_data.InsertShuntSensorData(gain_level, CalibrateSensor(Sensor::Shunt, gain_level, shunt_calibration_data));
                cal_data.InsertClampSensorData(gain_level, CalibrateSensor(Sensor::Clamp, gain_level, clamp_calibration_data));
            }

            if (FlashController2::Store(cal_data, COEFFS_FLASH_START_ADDR)) {
                messageBox->ShowMsg("calibration data stored successfully");
                Task::DelayMs(5000);
            }

            Stop();
        }
    }
    [[noreturn]] [[gnu::hot]] void ManageSensorsDataTask() noexcept
    {
        while (true) {
            auto sensor_data = fromSensorDataQueue->Receive();

            CheckDataStability(sensor_data.GetI());
            if (workMode == Mode::Calibration) {
                *value1 = sensor_data.GetI();
                *value2 = deviationOfDeviation;
                *value3 = sensor_data.GetAbsolute();
                *value4 = sensor_data.GetDegree();
            }
            else {
                *value4 = sensor_data.GetDegreeNocal();
            }
            switch (activeSensor) {
            case Sensor::Voltage:
                data.voltageSensorData = sensor_data;
                CalculateVoltageSensor();
                break;
            case Sensor::Shunt:
                data.shuntSensorData = sensor_data;
                CalculateShuntSensor();
                break;
            case Sensor::Clamp:
                data.clampSensorData = sensor_data;
                CalculateClampSensor();
                break;
            }
        }
    }

  private:
    struct ClampMeterData {
      public:
        SensorData voltageSensorData;
        SensorData shuntSensorData;
        SensorData clampSensorData;

        ValueT AppliedVoltageI{};
        ValueT AppliedVoltageQ{};
        ValueT AppliedVoltage{};
        ValueT AppliedVoltagePhi{};

        ValueT ZOverall;
        ValueT ROverall;
        ValueT XOverall;
        ValueT ZOverallPhi;

        ValueT ZClamp;
        ValueT RClamp;
        ValueT XClamp;
    };

    auto constexpr static firstBufferSize        = 100;
    auto constexpr static lastBufferSize         = 1;
    auto constexpr static dataStabilityBufferLen = 10;

    // todo: compress
    std::shared_ptr<UniversalSafeType> value1;
    std::shared_ptr<UniversalSafeType> value2;
    std::shared_ptr<UniversalSafeType> value3;
    std::shared_ptr<UniversalSafeType> value4;

    PeripheralsController peripherals;
    OutputGenerator       generator;
    AdcDriverT            adc;   // todo: make type independent

    bool firstMeasurementsStart = true;
    Mode workMode               = Mode::Normal;

    Semaphore universalSemaphore;

    // todo: take this from here
    std::shared_ptr<SuperFilterNoTask<ValueT>> filterI;
    std::shared_ptr<SuperFilterNoTask<ValueT>> filterQ;

    DeviationCalculator<ValueT, 100> deviationCalc;
    DeviationCalculator<ValueT, 100> devdevCalc;
    ValueT                           deviationOfDeviation;
    ValueT                           devValue;

    std::map<Sensor, SensorController>         sensors;
    Sensor                                     activeSensor;
    ClampMeterData                             data;
    std::array<ValueT, dataStabilityBufferLen> stabilityCheckBuffer;

    std::shared_ptr<FromSensorQueueT> fromSensorDataQueue;

    Task                     sensorDataManagerTask;
    Task                     calibrationTask;
    std::shared_ptr<DialogT> messageBox;

    bool isCalibrated{ false };

    // todo: take this from here
    std::array<std::pair<MCP3462_driver::Reference, MCP3462_driver::Reference>, 3> static constexpr adcChannelsMuxSettings{
        std::pair{ MCP3462_driver::Reference::CH2, MCP3462_driver::Reference::CH3 },
        std::pair{ MCP3462_driver::Reference::CH0, MCP3462_driver::Reference::CH1 },
        std::pair{ MCP3462_driver::Reference::CH4, MCP3462_driver::Reference::CH5 }
    };

    std::array<ValueT, 10> static constexpr calibrationResistances{
        8.15e3f, 8.15e3f, 55.4e3f, 55.4e3f, 465e3f, 465e3f, 465e3f, 465e3f, 465e3f, 465e3f,
    };
    std::array<ValueT, 10> static constexpr clampCalibrationDeviationTargets{ 9e-7f, 1e-6f, 5e-5f, 5e-5f, 1e-4f,
                                                                              1e-4,  1e-4,  1e-4,  1e-4,  1e-4 };

    std::array<ValueT, 10> static constexpr shuntSensorDeviationTargets{ 1e-7f, 1e-7f, 3e-7f, 3e-7f, 3e-7f,
                                                                         3e-7f, 3e-7f, 3e-7f, 3e-7f, 3e-7f };

    ValueT voltageSensorDeviationTarget      = 7e-8;
    auto static constexpr gainLevelsQuantity = 10;

    ValueT static constexpr shuntResistorValue = 1000.f;
};
