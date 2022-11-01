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

class ClampMeterDriver {
  public:
    using ValueT                        = float;
    using DataType                      = std::pair<ValueT, ValueT>;
    using SensorDataInsertionCallback   = std::function<void()>;
    using CalculationCompletionCallback = std::function<void()>;
    using AdcDriverT                    = MCP3462_driver;
    using AdcValueT                     = AdcDriverT::ValueT;
    using SensorPreampT                 = SensorPreamp<AdcDriverT::ValueT>;
    using QueueT                        = QueueHandle_t;
    using Semaphore                     = SemaphoreHandle_t;
    using FilterT                       = SuperFilterNoTask<ValueT>;
    using Filter                        = std::shared_ptr<FilterT>;
    using FromSensorQueueT              = Queue<SensorData>;

    enum Configs {
        NumberOfSensors                 = 3,
        NumberOfSensorPreamps           = 2,
        QueueReceiveFromADCTimeout      = portMAX_DELAY,
        SemaphoreTakeTimeout            = portMAX_DELAY,
        PowerSupplyDelayAfterActivation = pdMS_TO_TICKS(300)
    };
    enum class Sensor {
        Shunt = 0,
        Clamp,
        Voltage
    };

    ClampMeterDriver(std::shared_ptr<UniversalSafeType> vout, std::shared_ptr<UniversalSafeType> shunt)
      : VoutValue{ vout }
      , VShunt{ shunt }
      , adc{ ProjectConfigs::ADCAddress, ProjectConfigs::ADCStreamBufferCapacity, ProjectConfigs::ADCStreamBufferTriggeringSize }
      , filterI{ std::make_shared<FilterT>() }
      , filterQ{ std::make_shared<FilterT>() }
      , fromVoltageSensorQueue{ std::make_shared<FromSensorQueueT>(ProjectConfigs::FromSensorOutputQueueLength) }
      , fromShuntSensorQueue{ std::make_shared<FromSensorQueueT>(ProjectConfigs::FromSensorOutputQueueLength) }
      , fromClampSensorQueue{ std::make_shared<FromSensorQueueT>(ProjectConfigs::FromSensorOutputQueueLength) }
      , voltageTask{ [this]() { this->VoltageSensorTask(); },
                     ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::ClampDriverSensor),
                     ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::ClampDriverSensor),
                     "voltage sensor" }
      , shuntTask{ [this]() { this->ShuntSensorTask(); },
                   ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::ClampDriverSensor),
                   ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::ClampDriverSensor),
                   "shunt sensor" }
      , clampTask{ [this]() { this->ClampSensorTask(); },
                   ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::ClampDriverSensor),
                   ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::ClampDriverSensor),
                   "clamp sensor" }
    {
        InitializeFilters();
        InitializeSensors();
    }

    void StartMeasurements() noexcept
    {
        if (firstTimeEntry) {
            // fixme: very very ugly workaround for synchronization of adc and dac
            SwitchSensor(Sensor::Voltage);
            adc.StartMeasurement();
            Task::DelayMs(100);
            adc.StopMeasurement();

            firstTimeEntry = false;
        }

        SwitchSensor(Sensor::Voltage);
        peripherals.powerSupply.Activate();
        vTaskDelay(PowerSupplyDelayAfterActivation);

        adc.StartMeasurement();
        generator.StartGenerating();
        peripherals.outputRelay.Activate();
    }
    void StopMeasurements() noexcept
    {
        peripherals.outputRelay.Deactivate();
        generator.StopGenerating();
        peripherals.powerSupply.Deactivate();
        adc.StopMeasurement();
        vTaskDelay(pdMS_TO_TICKS(200));
        sensors.at(activeSensor).Disable();
    }

    [[noreturn]] void ShuntSensorTask()
    {
        ValueT new_value;

        while (true) {
            xSemaphoreTake(sensors.at(Sensor::Shunt).GetDataReadySemaphore(), SemaphoreTakeTimeout);

            auto some_value = sensors.at(Sensor::Shunt).GetValue();
            //            *VoutValue      = some_value;
        }
    }
    [[noreturn]] void VoltageSensorTask()
    {
        ValueT new_value;

        while (true) {
            //            xSemaphoreTake(sensors.at(Sensor::Voltage).GetDataReadySemaphore(), SemaphoreTakeTimeout);
            auto data = fromVoltageSensorQueue->Receive();

            *VoutValue = data.GetTrueValue_I();
            *VShunt    = data.GetTrueValue_Q();
        }
    }
    [[noreturn]] void ClampSensorTask()
    {
        ValueT new_value;

        while (true) {
            xSemaphoreTake(sensors.at(Sensor::Clamp).GetDataReadySemaphore(), SemaphoreTakeTimeout);

            auto some_value = sensors.at(Sensor::Clamp).GetValue();
            //            *VoutValue      = some_value;
        }
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

    [[noreturn]] void TestSenderTask() noexcept
    {
        auto   some_value = float{};
        size_t counter    = 0;
        while (true) {
            //            some_value = sinus_table.at(counter);
            //
            //            auto send_result = xQueueSend(sensors.at(Sensor::Voltage).GetInputQueue(), &some_value, 0);
            //            if (send_result == errQUEUE_FULL)
            //                QueueFullHook(xTaskGetCurrentTaskHandle(), "sender");
            //
            //            counter++;
            //
            //            if (counter == 22)
            //                counter = 0;

            //            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    void SwitchSensor(Sensor new_sensor) noexcept
    {
        sensors.at(activeSensor).Disable();
        activeSensor = new_sensor;
        sensors.at(activeSensor).Enable();
    }
    void StandardSensorOnEnableCallback() noexcept
    {
        using UnderType = std::underlying_type_t<Sensor>;

        adc.SetOutputStreamBuffer(sensors.at(activeSensor).GetInputStreamBuffer());
        adc.SetMux(adcChannelsMuxSettings.at(static_cast<UnderType>(activeSensor)).first,
                   adcChannelsMuxSettings.at(static_cast<UnderType>(activeSensor)).second);
        sensors.at(activeSensor).SetMinGain();
    }
    void StandardSensorOnDisableCallback() noexcept { sensors.at(activeSensor).SetMinGain(); }
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

        InitializeIO();

        {
            auto v_gain_controller = std::make_shared<GainController>(1, 1);
            v_gain_controller->SetGainChangeFunctor(
              1,
              [this]() { adc.SetGain(MCP3462_driver::Gain::GAIN_1); },
              60314.3242f,
              161.143814f);
            auto v_amplifier_controller = std::make_unique<AmplifierController>(std::move(v_gain_controller));
            auto [_unused_, emplaced]   = sensors.emplace(std::piecewise_construct,
                                                        std::forward_as_tuple(Sensor::Voltage),
                                                        std::forward_as_tuple(std::move(v_amplifier_controller),
                                                                              iq_controller,
                                                                              adc.CreateNewStreamBuffer(),
                                                                              fromVoltageSensorQueue,
                                                                              filterI,
                                                                              filterQ));

            configASSERT(emplaced);

            sensors.at(Sensor::Voltage).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
        }

        {
            auto sh_gain_controller = std::make_shared<GainController>(1, 10);
            // clang-format off
            sh_gain_controller->SetGainChangeFunctor(1, [this]() { shunt_sensor_set_gain(0);  adc.SetGain(AdcDriverT::Gain::GAIN_1V3);},  53784.875f / 3  , 90.9220352f);
            sh_gain_controller->SetGainChangeFunctor(2, [this]() { shunt_sensor_set_gain(0); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  53784.875f      , 90.9220352f);
            sh_gain_controller->SetGainChangeFunctor(3, [this]() { shunt_sensor_set_gain(1); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  132666.203f     , 90.9662323f);
            sh_gain_controller->SetGainChangeFunctor(4, [this]() { shunt_sensor_set_gain(2); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  264311.406f     , 91.0643845f);
            sh_gain_controller->SetGainChangeFunctor(5, [this]() { shunt_sensor_set_gain(3); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  8696278.f       , 90.829567f);
            sh_gain_controller->SetGainChangeFunctor(6, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_1);}   ,  17165270.f      , 90.9678421f);
            sh_gain_controller->SetGainChangeFunctor(7, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_2);}   ,  17165270.f * 2  , 90.9678421f);
            sh_gain_controller->SetGainChangeFunctor(8, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_4);}   ,  17165270.f * 4  , 90.9678421f);
            sh_gain_controller->SetGainChangeFunctor(9, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_8);}   ,  17165270.f * 8  , 90.9678421f);
            sh_gain_controller->SetGainChangeFunctor(10, [this]() { shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_16);} ,  17165270.f * 16 , 90.9678421f);
            // clang-format on
            auto sh_agc                  = std::make_unique<AGC>(1, 10, 100000UL, 3000000UL, 50, 500);
            auto sh_amplifier_controller = std::make_unique<AmplifierController>(sh_gain_controller, std::move(sh_agc), true);
            sensors.emplace(std::piecewise_construct,
                            std::forward_as_tuple(Sensor::Shunt),
                            std::forward_as_tuple(std::move(sh_amplifier_controller),
                                                  iq_controller,
                                                  adc.CreateNewStreamBuffer(),
                                                  fromShuntSensorQueue,
                                                  filterI,
                                                  filterQ));
            sensors.at(Sensor::Shunt).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
            sensors.at(Sensor::Shunt).SetOnDisableCallback([this]() { this->StandardSensorOnDisableCallback(); });
        }

        {
            auto clamp_gain_controller = std::make_shared<GainController>(1, 10);
            // clang-format off
            clamp_gain_controller->SetGainChangeFunctor(1, [this]() {clamp_sensor_set_gain(0);adc.SetGain(AdcDriverT::Gain::GAIN_1V3);}, 338750944.f / 3      , 255.35321f );
            clamp_gain_controller->SetGainChangeFunctor(2, [this]() {clamp_sensor_set_gain(0);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , 338750944.f          , 255.35321f );
            clamp_gain_controller->SetGainChangeFunctor(3, [this]() {clamp_sensor_set_gain(1);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , 645587648.f          , 90.9662323f);
            clamp_gain_controller->SetGainChangeFunctor(4, [this]() {clamp_sensor_set_gain(2);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , 955409024.f          , 91.0643845f);
            clamp_gain_controller->SetGainChangeFunctor(5, [this]() {clamp_sensor_set_gain(3);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , 1.53519985e+010f     , 90.829567f );
            clamp_gain_controller->SetGainChangeFunctor(6, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_1);}  , 1.50437837e+010f     , 90.9678421f);
            clamp_gain_controller->SetGainChangeFunctor(7, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_2);}  , 1.50437837e+010f * 2 , 90.9678421f);
            clamp_gain_controller->SetGainChangeFunctor(8, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_4);}  , 1.50437837e+010f * 4 , 90.9678421f);
            clamp_gain_controller->SetGainChangeFunctor(9, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_8);}  , 1.50437837e+010f * 8 , 90.9678421f);
            clamp_gain_controller->SetGainChangeFunctor(10, [this]() {clamp_sensor_set_gain(4);adc.SetGain(AdcDriverT::Gain::GAIN_16);}, 1.50437837e+010f * 16, 90.9678421f);
            // clang-format on
            auto clamp_agc = std::make_unique<AGC>(1, 10, 100000UL, 3000000UL, 50, 500);
            auto clamp_amplifier_controller =
              std::make_unique<AmplifierController>(clamp_gain_controller, std::move(clamp_agc), true);
            sensors.emplace(std::piecewise_construct,
                            std::forward_as_tuple(Sensor::Clamp),
                            std::forward_as_tuple(std::move(clamp_amplifier_controller),
                                                  iq_controller,
                                                  adc.CreateNewStreamBuffer(),
                                                  fromClampSensorQueue,
                                                  filterI,
                                                  filterQ));
            sensors.at(Sensor::Clamp).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
            sensors.at(Sensor::Clamp).SetOnDisableCallback([this]() { this->StandardSensorOnDisableCallback(); });
        }
    }

  private:
    auto constexpr static firstBufferSize = 100;
    auto constexpr static lastBufferSize  = 1;

    // todo test
    std::shared_ptr<UniversalSafeType> VoutValue;
    std::shared_ptr<UniversalSafeType> VShunt;
    bool                               firstTimeEntry = true;

    PeripheralsController peripherals;

    OutputGenerator generator;
    AdcDriverT      adc;   // todo: make type independent
                           //    SuperFilterWithTask<float>                 filter_I;
                           //    SuperFilterWithTask<float>                 filter_Q;

    std::shared_ptr<SuperFilterNoTask<ValueT>> filterI;
    std::shared_ptr<SuperFilterNoTask<ValueT>> filterQ;

    std::map<Sensor, SensorController> sensors;
    //    std::shared_ptr<SensorController>  voltageSensor;
    Sensor activeSensor;

    std::shared_ptr<FromSensorQueueT> fromVoltageSensorQueue;
    std::shared_ptr<FromSensorQueueT> fromShuntSensorQueue;
    std::shared_ptr<FromSensorQueueT> fromClampSensorQueue;

    Task voltageTask;
    Task shuntTask;
    Task clampTask;

    std::array<std::pair<MCP3462_driver::Reference, MCP3462_driver::Reference>, 3> static constexpr adcChannelsMuxSettings{
        std::pair{ MCP3462_driver::Reference::CH2, MCP3462_driver::Reference::CH3 },
        std::pair{ MCP3462_driver::Reference::CH0, MCP3462_driver::Reference::CH1 },
        std::pair{ MCP3462_driver::Reference::CH4, MCP3462_driver::Reference::CH5 }
    };
    //    Task testSenderTask;
};
