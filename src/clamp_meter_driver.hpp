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

    ClampMeterDriver(std::shared_ptr<UniversalSafeType> new_test_value)
      : testValue{ new_test_value }
      , adc{ ProjectConfigs::ADCAddress, ProjectConfigs::ADCStreamBufferCapacity, ProjectConfigs::ADCStreamBufferTriggeringSize }
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
    /*, testSenderTask([this]() { this->TestSenderTask(); }, 400, 3, "test sender")*/
    {
        InitializeFilters();
        InitializeSensors();

        //        testSenderTask.Suspend();         // test
    }

    void StartMeasurements() noexcept
    {
        SwitchSensor(Sensor::Voltage);
        filter.Resume();
        peripherals.powerSupply.Activate();
        vTaskDelay(PowerSupplyDelayAfterActivation);

        adc.StartMeasurement();
        generator.StartGenerating();
        peripherals.outputRelay.Activate();
    }
    void StopMeasurements() noexcept
    {
        // test
        //        testSenderTask.Suspend();
        //        return;
        peripherals.outputRelay.Deactivate();
        generator.StopGenerating();
        peripherals.powerSupply.Deactivate();
        adc.StopMeasurement();
        filter.Suspend();
    }

    [[noreturn]] void ShuntSensorTask()
    {
        ValueT new_value;

        while (true) {
            xSemaphoreTake(sensors.at(Sensor::Shunt).GetDataReadySemaphore(), SemaphoreTakeTimeout);

            auto some_value = sensors.at(Sensor::Shunt).GetValue();
            *testValue      = some_value;
        }
    }
    [[noreturn]] void VoltageSensorTask()
    {
        ValueT new_value;

        while (true) {
            xSemaphoreTake(sensors.at(Sensor::Voltage).GetDataReadySemaphore(), SemaphoreTakeTimeout);

            auto some_value = sensors.at(Sensor::Voltage).GetValue();
            *testValue      = some_value;
        }
    }
    [[noreturn]] void ClampSensorTask()
    {
        ValueT new_value;

        while (true) {
            xSemaphoreTake(sensors.at(Sensor::Clamp).GetDataReadySemaphore(), SemaphoreTakeTimeout);

            auto some_value = sensors.at(Sensor::Clamp).GetValue();
            *testValue      = some_value;
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
        filter.SetOutputQueue(sensors.at(activeSensor).GetFromFilterQueueI());
        sensors.at(activeSensor).SetMinGain();
    }
    void StandardSensorOnDisableCallback() noexcept { sensors.at(activeSensor).SetMinGain(); }
    void InitializeFilters() noexcept
    {
        auto constexpr fir1_dec_blocksize = 100;

        auto constexpr biquad1_nstages  = 2;
        auto constexpr biquad1_ncoeffs  = biquad1_nstages * 5;
        auto constexpr biquad1_buffsize = fir1_dec_blocksize;

        auto constexpr fir1_dec_ncoeffs = 5;
        auto constexpr fir_dec_factor   = 10;

        auto constexpr fir2_dec_blocksize = 10;
        auto constexpr fir2_dec_ncoeffs   = 5;

        auto constexpr biquad2_nstages  = 2;
        auto constexpr biquad2_ncoeffs  = biquad2_nstages * 5;
        auto constexpr biquad2_buffsize = fir2_dec_blocksize;

        auto constexpr biquad3_nstages  = 3;
        auto constexpr biquad3_ncoeffs  = biquad3_nstages * 5;
        auto constexpr biquad3_buffsize = 1;

        using CoefficientT = BiquadCascadeDF2TFilter<0, 0>::CoefficientT;

        auto sin_filter1 = std::make_unique<BiquadCascadeDF2TFilter<biquad1_nstages, biquad1_buffsize>>(
          std::array<CoefficientT, biquad1_ncoeffs>{ 0.00018072660895995795726776123046875f,
                                                     0.0003614532179199159145355224609375f,
                                                     0.00018072660895995795726776123046875f,

                                                     1.9746592044830322265625f,
                                                     -0.975681483745574951171875f,

                                                     0.00035528256557881832122802734375f,
                                                     0.0007105651311576366424560546875f,
                                                     0.00035528256557881832122802734375f,

                                                     1.94127738475799560546875f,
                                                     -0.9422824382781982421875f },
          filter.GetFirstBuffer());

        auto sin_filter2 = std::make_unique<FirDecimatingFilter<fir1_dec_blocksize, fir1_dec_ncoeffs, fir_dec_factor>>(
          std::array<CoefficientT, fir1_dec_ncoeffs>{ 0.02868781797587871551513671875f,
                                                      0.25f,
                                                      0.4426243603229522705078125f,
                                                      0.25f,
                                                      0.02868781797587871551513671875f },
          sin_filter1->GetOutputBuffer());

        auto sin_filter3 = std::make_unique<BiquadCascadeDF2TFilter<biquad2_nstages, biquad2_buffsize>>(
          std::array<CoefficientT, biquad2_ncoeffs>{ 0.000180378541699610650539398193359375f,
                                                     0.00036075708339922130107879638671875f,
                                                     0.000180378541699610650539398193359375f,

                                                     1.97468459606170654296875f,
                                                     -0.975704848766326904296875f,

                                                     0.00035460057551972568035125732421875f,
                                                     0.0007092011510394513607025146484375f,
                                                     0.00035460057551972568035125732421875f,

                                                     1.941333770751953125f,
                                                     -0.942336857318878173828125f },
          sin_filter2->GetOutputBuffer());

        auto sin_filter4 = std::make_unique<FirDecimatingFilter<fir2_dec_blocksize, fir2_dec_ncoeffs, fir_dec_factor>>(
          std::array<CoefficientT, fir2_dec_ncoeffs>{ 0.02867834083735942840576171875f,
                                                      0.25f,
                                                      0.4426433145999908447265625f,
                                                      0.25f,
                                                      0.02867834083735942840576171875f },
          sin_filter3->GetOutputBuffer());

        auto sin_filter5 = std::make_unique<BiquadCascadeDF2TFilter<biquad3_nstages, biquad3_buffsize>>(
          std::array<CoefficientT, biquad3_ncoeffs>{ 0.0000231437370530329644680023193359375f,
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
          sin_filter4->GetOutputBuffer());
        filter.SetLastBuffer(sin_filter5->GetOutputBuffer());

        filter.InsertFilter(std::move(sin_filter1));
        filter.InsertFilter(std::move(sin_filter2));
        filter.InsertFilter(std::move(sin_filter3));
        filter.InsertFilter(std::move(sin_filter4));
        filter.InsertFilter(std::move(sin_filter5));
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
            v_gain_controller->SetGainChangeFunctor(1, [this]() { adc.SetGain(MCP3462_driver::Gain::GAIN_1); });
            auto v_amplifier_controller = std::make_unique<AmplifierController>(std::move(v_gain_controller));
            auto [_unused_, emplaced]   = sensors.emplace(std::piecewise_construct,
                                                        std::forward_as_tuple(Sensor::Voltage),
                                                        std::forward_as_tuple(std::move(v_amplifier_controller),
                                                                              iq_controller,
                                                                              adc.CreateNewStreamBuffer(),
                                                                              filter.GetInputStreamBuffer()));

            configASSERT(emplaced);

            sensors.at(Sensor::Voltage).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
        }

        {
            auto sh_gain_controller = std::make_shared<GainController>(1, 10);
            sh_gain_controller->SetGainChangeFunctor(1, [this]() {
                shunt_sensor_set_gain(0);
                adc.SetGain(AdcDriverT::Gain::GAIN_1V3);
            });
            sh_gain_controller->SetGainChangeFunctor(2, [this]() {
                shunt_sensor_set_gain(0);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            sh_gain_controller->SetGainChangeFunctor(3, [this]() {
                shunt_sensor_set_gain(1);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            sh_gain_controller->SetGainChangeFunctor(4, [this]() {
                shunt_sensor_set_gain(2);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            sh_gain_controller->SetGainChangeFunctor(5, [this]() {
                shunt_sensor_set_gain(3);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            sh_gain_controller->SetGainChangeFunctor(6, [this]() {
                shunt_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            sh_gain_controller->SetGainChangeFunctor(7, [this]() {
                shunt_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_2);
            });
            sh_gain_controller->SetGainChangeFunctor(8, [this]() {
                shunt_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_4);
            });
            sh_gain_controller->SetGainChangeFunctor(9, [this]() {
                shunt_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_8);
            });
            sh_gain_controller->SetGainChangeFunctor(10, [this]() {
                shunt_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_16);
            });

            auto sh_agc                  = std::make_unique<AGC>(1, 10, 100000UL, 3000000UL, 50, 500);
            auto sh_amplifier_controller = std::make_unique<AmplifierController>(sh_gain_controller, std::move(sh_agc), true);
            sensors.emplace(std::piecewise_construct,
                            std::forward_as_tuple(Sensor::Shunt),
                            std::forward_as_tuple(std::move(sh_amplifier_controller),
                                                  iq_controller,
                                                  adc.CreateNewStreamBuffer(),
                                                  filter.GetInputStreamBuffer()));
            sensors.at(Sensor::Shunt).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
            sensors.at(Sensor::Shunt).SetOnDisableCallback([this]() { this->StandardSensorOnDisableCallback(); });
        }

        {
            auto clamp_gain_controller = std::make_shared<GainController>(1, 10);
            clamp_gain_controller->SetGainChangeFunctor(1, [this]() {
                clamp_sensor_set_gain(0);
                adc.SetGain(AdcDriverT::Gain::GAIN_1V3);
            });
            clamp_gain_controller->SetGainChangeFunctor(2, [this]() {
                clamp_sensor_set_gain(0);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            clamp_gain_controller->SetGainChangeFunctor(3, [this]() {
                clamp_sensor_set_gain(1);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            clamp_gain_controller->SetGainChangeFunctor(4, [this]() {
                clamp_sensor_set_gain(2);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            clamp_gain_controller->SetGainChangeFunctor(5, [this]() {
                clamp_sensor_set_gain(3);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            clamp_gain_controller->SetGainChangeFunctor(6, [this]() {
                clamp_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_1);
            });
            clamp_gain_controller->SetGainChangeFunctor(7, [this]() {
                clamp_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_2);
            });
            clamp_gain_controller->SetGainChangeFunctor(8, [this]() {
                clamp_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_4);
            });
            clamp_gain_controller->SetGainChangeFunctor(9, [this]() {
                clamp_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_8);
            });
            clamp_gain_controller->SetGainChangeFunctor(10, [this]() {
                clamp_sensor_set_gain(4);
                adc.SetGain(AdcDriverT::Gain::GAIN_16);
            });

            auto clamp_agc = std::make_unique<AGC>(1, 10, 100000UL, 3000000UL, 50, 500);
            auto clamp_amplifier_controller =
              std::make_unique<AmplifierController>(clamp_gain_controller, std::move(clamp_agc), true);
            sensors.emplace(std::piecewise_construct,
                            std::forward_as_tuple(Sensor::Clamp),
                            std::forward_as_tuple(std::move(clamp_amplifier_controller),
                                                  iq_controller,
                                                  adc.CreateNewStreamBuffer(),
                                                  filter.GetInputStreamBuffer()));
            sensors.at(Sensor::Clamp).SetOnEnableCallback([this]() { this->StandardSensorOnEnableCallback(); });
            sensors.at(Sensor::Clamp).SetOnDisableCallback([this]() { this->StandardSensorOnDisableCallback(); });
        }
    }

  private:
    auto constexpr static firstBufferSize = 100;
    auto constexpr static lastBufferSize  = 1;

    // todo test
    std::shared_ptr<UniversalSafeType> testValue;

    PeripheralsController peripherals;

    OutputGenerator                                     generator;
    AdcDriverT                                          adc;   // todo: make type independent
    SuperFilter<float, firstBufferSize, lastBufferSize> filter;
    std::map<Sensor, SensorController>                  sensors;
    std::shared_ptr<SensorController>                   voltageSensor;
    Sensor                                              activeSensor;

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
