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
#include <utility>
#include <complex>

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
#include "shunt_sensor.hpp"
#include "clamp_sensor.hpp"

#include "filter.hpp"
#include "sensor.hpp"
#include "menu_model_item.hpp"
#include "iq_calculator.hpp"
#include "task.hpp"

#include "dsp_resources.hpp"
#include "menu_model_dialog.hpp"
#include "semaphore/semaphore.hpp"
#include "deviation_calculator.hpp"
#include "flash_controller.hpp"

#include "string_converter.hpp"

#include "buzzer.hpp"
#include "buzzer_with_task.hpp"

// todo: remove from here
#include "signal_conditioning.h"
#include "calibrations.hpp"
// todo: cleanup

extern "C" char *gcvtf(float, int, char *);

extern std::array<std::pair<float, float>, 21> calibration_data;
extern bool                                    flash_restore_result;
extern bool                                    flash_save_result;

template<size_t SensorsGainLevelsNumber>
class CalibrationData {
  public:
    using ValueT       = float;
    using DataPair     = std::pair<ValueT, ValueT>;
    using ClampDataT   = std::array<DataPair, SensorsGainLevelsNumber>;
    using ShuntDataT   = ClampDataT;
    using VoltageDataT = DataPair;
    using GainLevelT   = int;

    constexpr CalibrationData() = default;
    constexpr CalibrationData(VoltageDataT v_data, ShuntDataT shunt_data, ClampDataT clamp_data)
      : voltage{ std::move(v_data) }
      , shunt{ shunt_data }
      , clamp{ clamp_data }
    { }

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

CalibrationData<10> static constexpr reserve_calibrations{
    CalibrationData<10>::VoltageDataT{ 60378.5f, 141.756561f },
    CalibrationData<10>::ShuntDataT{ std::pair{ 18119.3613f, 211.836044f },
                                     std::pair{ 53983.3125f, 211.87207f },
                                     std::pair{ 134806.391f, 211.716782f },
                                     std::pair{ 267293.906f, 211.763092f },
                                     std::pair{ 8691632.f, 212.049988f },
                                     std::pair{ 17133480.f, 212.030548f },
                                     std::pair{ 34077676.f, 211.958069f },
                                     std::pair{ 60054472.f, 211.976196f },
                                     std::pair{ 65935656.f, 212.031769f },
                                     std::pair{ 67758896.f, 212.305054f } },

    CalibrationData<10>::ClampDataT{ std::pair{ 194050976.f, 48.4416809f },
                                     std::pair{ 583724544.f, 51.1335754f },
                                     std::pair{ 1.12981146e+009f, 44.1073303f },
                                     std::pair{ 1.66808934e+009f, 40.8379211f },
                                     std::pair{ 2.64954061e+010f, 35.2421265f },
                                     std::pair{ 2.63934403e+010f, 34.6728821f },
                                     std::pair{ 5.04917484e+010f, 33.9216614f },
                                     std::pair{ 6.41723433e+010f, 34.8967896f },
                                     std::pair{ 6.66922353e+010f, 34.6208801f },
                                     std::pair{ 6.70647747e+010f, 34.6916504f } }

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
    using ComplexT                      = std::complex<ValueT>;
    using IQCalcT                       = SynchronousIQCalculator<ValueT>;

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
                     std::shared_ptr<UniversalSafeType> v5,
                     std::shared_ptr<UniversalSafeType> v6,
                     std::shared_ptr<UniversalSafeType> v7,
                     std::shared_ptr<UniversalSafeType> v8,
                     std::shared_ptr<DialogT>           new_msg_box)
      : value1{ vout }
      , value2{ shunt }
      , value3{ clamp }
      , value4{ v4 }
      , value5{ v5 }
      , value6{ v6 }
      , value7{ v7 }
      , value8{ v8 }
      , generator{ ProjectConfigs::GeneratorAmplitude }
      , adc{ ProjectConfigs::ADCAddress, ProjectConfigs::ADCStreamBufferCapacity, ProjectConfigs::ADCStreamBufferTriggeringSize }
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
    //      , buzzer{ Buzzer::Get() }
    {
        calibrationTask.Suspend();

        InitializeSensors();
        SynchronizeAdcAndDac();
    }

    void SwitchToNextSensor() noexcept
    {
        StopMeasurements();

        switch (activeSensor) {
        case Sensor::Voltage: SetAndActivateSensor(Sensor::Shunt); break;
        case Sensor::Shunt: SetAndActivateSensor(Sensor::Clamp); break;
        case Sensor::Clamp: SetAndActivateSensor(Sensor::Voltage); break;
        }

        StartMeasurements();
    }

    void CalculateAppliedVoltage() noexcept
    {
        data.appliedVoltage = data.voltageSensorData.GetValue() - data.shuntSensorData.GetValue();
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
            sensor.SetMinGain();
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

    [[nodiscard]] std::pair<Filter, Filter> CreateDoubleFilter() noexcept
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

        auto filterI = std::make_shared<FilterT>();
        filterI->InsertFilter(std::move(sin_filter1));
        filterI->InsertFilter(std::move(sin_filter2));
        filterI->InsertFilter(std::move(sin_filter3));
        filterI->InsertFilter(std::move(sin_filter4));
        filterI->InsertFilter(std::move(sin_filter5));

        auto filterQ = std::make_shared<FilterT>();
        filterQ->InsertFilter(std::move(cos_filter1));
        filterQ->InsertFilter(std::move(cos_filter2));
        filterQ->InsertFilter(std::move(cos_filter3));
        filterQ->InsertFilter(std::move(cos_filter4));
        filterQ->InsertFilter(std::move(cos_filter5));

        return std::pair{ filterI, filterQ };
    }
//    void InitializeIO() noexcept
//    {
//        pio_set_input(PIOA, SH_SENSOR_GAIN_A_PIN | CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, 0);
//
//        pio_set_input(PIOD, CLAMP_SENSOR_GAIN_A_PIN | SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, 0);
//
//        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);
//
//        pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
//
//        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
//        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
//    }
    void InitializeSensors() noexcept
    {
        using AGC          = AutomaticGainController<GainController, ValueT>;
        using GainT        = GainController::GainLevelT;
        auto iq_controller = std::make_shared<SynchronousIQCalculator<float>>(sinus_table.size());

        // todo: make this initializer part of gain controllers
//        InitializeIO();

        auto [calibs, result] = FlashController2::Recall(COEFFS_FLASH_START_ADDR);

        // todo: clean after debug

        if (not result) {
            calibs       = reserve_calibrations;
            isCalibrated = false;
        }
        else {
            isCalibrated = true;
        }

        auto [filterI, filterQ] = CreateDoubleFilter();

        // Voltage sensor
        {
            auto v_gain_controller = std::make_shared<GainController>(1, 1);
            v_gain_controller->SetGainChangeFunctor(
              1,
              [this]() { adc.SetGain(MCP3462_driver::Gain::GAIN_1); },
              calibs.GetVoltageSensorCalData());
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
            sh_gain_controller->SetGainChangeFunctor(1, [this]() { shuntSensor->SetGain(0);  adc.SetGain(AdcDriverT::Gain::GAIN_1V3);},  calibs.GetShuntSensorCalData(1));
            sh_gain_controller->SetGainChangeFunctor(2, [this]() { shuntSensor->SetGain(0); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  calibs.GetShuntSensorCalData(2));
            sh_gain_controller->SetGainChangeFunctor(3, [this]() { shuntSensor->SetGain(1); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  calibs.GetShuntSensorCalData(3));
            sh_gain_controller->SetGainChangeFunctor(4, [this]() { shuntSensor->SetGain(2); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  calibs.GetShuntSensorCalData(4));
            sh_gain_controller->SetGainChangeFunctor(5, [this]() { shuntSensor->SetGain(3); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  calibs.GetShuntSensorCalData(5));
            sh_gain_controller->SetGainChangeFunctor(6, [this]() { shuntSensor->SetGain(4); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  calibs.GetShuntSensorCalData(6));
            sh_gain_controller->SetGainChangeFunctor(7, [this]() { shuntSensor->SetGain(4); adc.SetGain(AdcDriverT::Gain::GAIN_2);}   ,  calibs.GetShuntSensorCalData(7));
            sh_gain_controller->SetGainChangeFunctor(8, [this]() { shuntSensor->SetGain(4); adc.SetGain(AdcDriverT::Gain::GAIN_4);}   ,  calibs.GetShuntSensorCalData(8));
            sh_gain_controller->SetGainChangeFunctor(9, [this]() { shuntSensor->SetGain(4); adc.SetGain(AdcDriverT::Gain::GAIN_8);}   ,  calibs.GetShuntSensorCalData(9));
            sh_gain_controller->SetGainChangeFunctor(10, [this](){ shuntSensor->SetGain(4); adc.SetGain(AdcDriverT::Gain::GAIN_16);} ,  calibs.GetShuntSensorCalData(10));
            // clang-format on
            auto sh_agc                  = std::make_unique<AGC>(1, 10, 1e5f, 3e6f, 50, 500);
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
            clamp_gain_controller->SetGainChangeFunctor(1, [this]()  {clampSensor->SetGain(0);adc.SetGain(AdcDriverT::Gain::GAIN_1V3);}, calibs.GetClampSensorCalData(1));
            clamp_gain_controller->SetGainChangeFunctor(2, [this]()  {clampSensor->SetGain(0);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , calibs.GetClampSensorCalData(2));
            clamp_gain_controller->SetGainChangeFunctor(3, [this]()  {clampSensor->SetGain(1);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , calibs.GetClampSensorCalData(3));
            clamp_gain_controller->SetGainChangeFunctor(4, [this]()  {clampSensor->SetGain(2);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , calibs.GetClampSensorCalData(4));
            clamp_gain_controller->SetGainChangeFunctor(5, [this]()  {clampSensor->SetGain(3);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , calibs.GetClampSensorCalData(5));
            clamp_gain_controller->SetGainChangeFunctor(6, [this]()  {clampSensor->SetGain(4);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , calibs.GetClampSensorCalData(6));
            clamp_gain_controller->SetGainChangeFunctor(7, [this]()  {clampSensor->SetGain(4);adc.SetGain(AdcDriverT::Gain::GAIN_2);}  , calibs.GetClampSensorCalData(7));
            clamp_gain_controller->SetGainChangeFunctor(8, [this]()  {clampSensor->SetGain(4);adc.SetGain(AdcDriverT::Gain::GAIN_4);}  , calibs.GetClampSensorCalData(8));
            clamp_gain_controller->SetGainChangeFunctor(9, [this]()  {clampSensor->SetGain(4);adc.SetGain(AdcDriverT::Gain::GAIN_8);}  , calibs.GetClampSensorCalData(9));
            clamp_gain_controller->SetGainChangeFunctor(10, [this]() {clampSensor->SetGain(4);adc.SetGain(AdcDriverT::Gain::GAIN_16);}, calibs.GetClampSensorCalData(10));
            // clang-format on
            auto clamp_agc = std::make_unique<AGC>(1, 10, 1e5f, 4e6f, 50, 500);
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
            sensors.at(Sensor::Clamp).SetOnEnableCallback([this]() {
                this->StandardSensorOnEnableCallback();
                if (workMode == Mode::Normal)
                    //                    buzzer->Enable();
                    buzz.Start();
            });
            sensors.at(Sensor::Clamp).SetOnDisableCallback([this]() {
                this->StandardSensorOnDisableCallback();
                //                buzzer->Disable();
                buzz.Stop();
            });
        }
    }

    void StartMeasurements() noexcept
    {
        // todo: move this initialization from here
        if (firstStart) {
            // todo: make this procedure more pretty
            SetAndActivateSensor(Sensor::Voltage);
            adc.StartSynchronousMeasurements();
            Task::DelayMs(100);
            adc.StopMeasurement();
            Task::DelayMs(200);

            firstStart = false;
        }

        SetAndActivateSensor(activeSensor);

        peripherals.powerSupply.Activate();
        Task::DelayMs(PowerSupplyDelayAfterActivation);

        Task::SuspendAll();
        adc.StartSynchronousMeasurements();
        generator.StartGenerating();
        Task::ResumeAll();

        peripherals.outputRelay.Activate();

        isActive = true;
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

        Task::DelayMs(200);

        isActive = false;
    }
    void SetAndActivateSensor(Sensor new_sensor) noexcept
    {
        bool wasActive = isActive;

        if (isActive) {
            wasActive = true;
            StopMeasurements();
        }

        sensors.at(activeSensor).Disable();
        activeSensor = new_sensor;
        sensors.at(activeSensor).Enable();

        if (wasActive) {
            StartMeasurements();
        }
    }

    void SynchronizeAdcAndDac() noexcept
    {
        generator.SetFirstInterruptCallback([this]() { adc.SynchronizationCallback(); });
    }

    void WriteDebugInfo(SensorData const &sensor_data) noexcept
    {
        size_t static counter                        = 0;
        size_t constexpr print_at_tick_counter_limit = 100;

        counter++;

        if (counter >= print_at_tick_counter_limit) {
            std::array<char, 10> val1;
            std::array<char, 10> val2;
            std::array<char, 10> val3;

            gcvtf(sensor_data.GetGainValue(), 4, val1.data());
            gcvtf(sensor_data.GetRawAbsolute(), 4, val2.data());

            messageBox->ShowMsg("glvl=" + std::to_string(sensor_data.GetGainLevel()) + " gval=" + std::string(val1.data()) +
                                " raw=" + std::string(val2.data()) + " vApl=" + std::string(val3.data()));

            counter = 0;
        }
    }
    void ShowCalculationsInfo() noexcept
    {
        auto msg = std::string{ "sensor: " };

        switch (activeSensor) {
        case Sensor::Voltage: msg.append("Voltage "); break;
        case Sensor::Shunt: msg.append("Shunt "); break;
        case Sensor::Clamp: msg.append("Clamp "); break;
        }

        //        msg.append(" XOverall=" + StringConverter::ToString<3>(data.XOverall));
        //        msg.append(" ROverall=" + StringConverter::ToString<3>(data.ROverall));
        //        msg.append(" XClamp=" + StringConverter::ToString<3>(data.XClamp));
        //        msg.append(" RClamp=" + StringConverter::ToString<3>(data.RClamp));
        msg.append(" IClamp=" + StringConverter::ToString<3>(data.clampSensorData.GetAbsolute()));

        messageBox->ShowMsg(std::move(msg));
    }

    /////////////////////// task helpers //////////////////////////////
    void SetBuzzerFreq() noexcept
    {
        auto constexpr modulation_k    = 0.9772f;
        auto constexpr modulation_zero = 5.5f;
        auto constexpr tone_k          = 716.f;
        auto constexpr tone_zero       = 9950.f;

        auto tone_freq = (log2f(data.clampSensorData.GetAbsolute())) * tone_k + tone_zero;
        auto modulation_freq =
          log2f(abs(data.clampSensorData.GetI() / data.clampSensorData.GetQ())) * modulation_k + modulation_zero;
        buzz.SetFrequency(tone_freq, modulation_freq);
    }
    void CheckDataStability(ValueT data) noexcept
    {
        deviationCalculator.PushBackAndCalculate(data);
        deviation = deviationCalculator.GetRelativeStdDev();
        deviation2Calculator.PushBackAndCalculate(deviationCalculator.GetRelativeStdDev());
        deviationOfDeviation = deviation2Calculator.GetStandardDeviation();
    }
    void CalculateVoltageSensor() noexcept
    {
        if (workMode == Mode::Calibration)
            return;

        *value1 = data.voltageSensorData.GetAbsolute();
        *value4 = data.voltageSensorData.GetDegree();
    }
    void CalculateShuntSensor() noexcept
    {
        if (workMode == Mode::Calibration)
            return;

        CalculateAppliedVoltage();

        auto admitance = (data.shuntSensorData.GetValue() / shuntResistorValue) / data.appliedVoltage;
        data.ZOverall.CalculateFromAdmitance(admitance);

        *value2 = data.ZOverall.GetZParallel();
        *value4 = data.ZOverall.GetDegreeParalel();
        *value5 = data.ZOverall.GetX();
        *value6 = data.ZOverall.GetR();
    }
    void CalculateClampSensor() noexcept
    {
        if (workMode == Mode::Calibration)
            return;

        auto q = data.clampSensorData.GetValue() / data.appliedVoltage;
        data.clamp.CalculateFromAdmitance(q);

        *value3 = data.clamp.GetZParallel();
        *value4 = data.clamp.GetDegreeParalel();
        *value7 = data.clamp.GetX();
        *value8 = data.clamp.GetR();
        SetBuzzerFreq();
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
        case Sensor::Voltage:
        configASSERT(0)   // todo implement;
          case Sensor::Shunt : {
            auto cal_targets = FindCalibrationTargetsForShunt(gain_level);
            sensors.at(sensor).SetTrueValuesForCalibration(cal_targets.first, cal_targets.second, gain_level);

            messageBox->ShowMsg("Calibration target for shunt: mag=" + std::to_string(cal_targets.first) +
                                " phi=" + std::to_string(cal_targets.second));

            Task::DelayMs(4000);

            break;
        }

        case Sensor::Clamp: {
            auto cal_targets = FindCalibrationTargetsForClamp(gain_level);
            sensors.at(sensor).SetTrueValuesForCalibration(cal_targets.first, cal_targets.second, gain_level);

            messageBox->ShowMsg("Calibration target for clamp: mag=" + std::to_string(cal_targets.first) +
                                " phi=" + std::to_string(cal_targets.second));

            Task::DelayMs(4000);

            break;
        }
        }
    }
    std::pair<ValueT, ValueT> FindCalibrationTargetsForShunt(auto gain_level) noexcept
    {
        auto magnitude = shuntResistorValue *
                         (data.voltageSensorData.GetAbsolute() / (shuntResistorValue + calibrationResistances.at(gain_level - 1)));

        return { magnitude, data.voltageSensorData.GetDegree() };
    }
    std::pair<ValueT, ValueT> FindCalibrationTargetsForClamp(auto gain_level) noexcept
    {
        auto circuit_current =
          data.voltageSensorData.GetAbsolute() / (shuntResistorValue + calibrationResistances.at(gain_level - 1));

        return { circuit_current, data.voltageSensorData.GetDegree() };
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
                sensors.at(Sensor::Shunt).SetMinGain();
                cal_data.InsertClampSensorData(gain_level, CalibrateSensor(Sensor::Clamp, gain_level, clamp_calibration_data));
                sensors.at(Sensor::Clamp).SetMinGain();
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

            if (writeDebugInfo)
                WriteDebugInfo(sensor_data);

            CheckDataStability(sensor_data.GetI());

            if (workMode == Mode::Calibration) {
                *value1 = sensor_data.GetI();
                *value2 = deviationOfDeviation;
                *value3 = sensor_data.GetAbsolute();
                *value4 = sensor_data.GetDegree();
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

            if (workMode == Mode::Normal) {
                //                ShowCalculationsInfo();
            }
        }
    }

  private:
    class Impedance {
      public:
        static ValueT FindRFromAdmitance(ComplexT admitance) noexcept { return 1 / admitance.real(); }
        static ValueT FindXFromAdmitance(ComplexT admitance) noexcept { return -1 / admitance.imag(); }

        [[nodiscard]] ValueT   GetDegree() const noexcept { return std::arg(z) * rad2deg; }
        [[nodiscard]] ValueT   GetRadians() const noexcept { return std::arg(z); }
        [[nodiscard]] ValueT   GetZAbs() const noexcept { return std::abs(z); }
        [[nodiscard]] ValueT   GetZParallel() const noexcept { return zParallel; }
        [[nodiscard]] ComplexT GetZ() const noexcept { return z; }
        [[nodiscard]] ValueT   GetR() const noexcept { return z.real(); }
        [[nodiscard]] ValueT   GetX() const noexcept { return z.imag(); }
        [[nodiscard]] ValueT   GetDegreeParalel() const noexcept { return degreeParallelForm; };

        void CalculateFromAdmitance(ComplexT admitance) noexcept
        {
            z                  = { FindRFromAdmitance(admitance), FindXFromAdmitance(admitance) };
            zParallel          = abs(1 / abs(admitance));
            degreeParallelForm = -std::arg(admitance) * rad2deg;
        }
        void SetZ(ValueT new_z) noexcept { z = new_z; }
        void SetR(ValueT new_r) noexcept { z.real(new_r); }
        void SetX(ValueT new_x) noexcept { z.imag(new_x); }

      private:
        ComplexT z;
        ValueT   degreeParallelForm{};
        ValueT   zParallel{};

        auto static constexpr rad2deg = 180.f / PI;
    };

    struct ClampMeterData {
      public:
        SensorData voltageSensorData;
        SensorData shuntSensorData;
        SensorData clampSensorData;

        ComplexT appliedVoltage;

        Impedance ZOverall;
        Impedance clamp;
    };

    // todo: compress
    std::shared_ptr<UniversalSafeType> value1;
    std::shared_ptr<UniversalSafeType> value2;
    std::shared_ptr<UniversalSafeType> value3;
    std::shared_ptr<UniversalSafeType> value4;
    std::shared_ptr<UniversalSafeType> value5;
    std::shared_ptr<UniversalSafeType> value6;
    std::shared_ptr<UniversalSafeType> value7;
    std::shared_ptr<UniversalSafeType> value8;

    PeripheralsController peripherals;
    OutputGenerator       generator;
    AdcDriverT            adc;   // todo: make type independent

    bool firstStart = true;
    bool isCalibrated{ false };
    bool isActive{ false };
    bool writeDebugInfo{ false };
    Mode workMode = Mode::Normal;

    Semaphore universalSemaphore;

    DeviationCalculator<ValueT, 100> deviationCalculator;
    DeviationCalculator<ValueT, 100> deviation2Calculator;
    ValueT                           deviationOfDeviation;
    ValueT                           deviation;

    // todo: use this class
    class CompositeSensorsController {
      private:
        std::map<Sensor, SensorController> sensors;
        Sensor                             activeSensor{ Sensor::Voltage };
        ClampMeterData                     data;
        std::shared_ptr<FromSensorQueueT>  fromSensorDataQueue;

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

        ValueT static constexpr voltageSensorDeviationTarget = 7e-8;
        auto static constexpr gainLevelsQuantity             = 10;
        ValueT static constexpr shuntResistorValue           = 1000.f;
    };

    std::map<Sensor, SensorController> sensors;
    Sensor                             activeSensor;
    ClampMeterData                     data;
    std::shared_ptr<FromSensorQueueT>  fromSensorDataQueue;

    std::shared_ptr<ShuntSensor> shuntSensor{ ShuntSensor::Get() };
    std::shared_ptr<ClampSensor> clampSensor{ ClampSensor::Get() };

    Task sensorDataManagerTask;
    Task calibrationTask;

    std::shared_ptr<DialogT> messageBox;
    BuzzerTask               buzz;

    // todo: this should go to composite sensors controller
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

    ValueT static constexpr voltageSensorDeviationTarget = 7e-8;
    auto static constexpr gainLevelsQuantity             = 10;
    ValueT static constexpr shuntResistorValue           = 1000.f;
};
