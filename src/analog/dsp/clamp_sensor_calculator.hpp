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
#include "pio.h"

#include "mcp3462_driver.hpp"
#include "HVPowerSupply.hpp"
#include "OutputRelay.hpp"
#include "output_generator.hpp"
#include "sensor_preamp.hpp"
#include "filter.hpp"
#include "external_periph_ctrl.h"
#include "signal_conditioning.h"

extern "C" void ClampSensorTaskWrapper(void *param);

class ClampMeterAnalogImplementation {
  public:
    using ValueT                        = float;
    using DataType                      = std::pair<ValueT, ValueT>;
    using SensorDataInsertionCallback   = std::function<void()>;
    using CalculationCompletionCallback = std::function<void()>;
    using AdcDriverT                    = MCP3462_driver;
    using SensorPreampT                 = SensorPreamp<AdcDriverT::ValueT>;
    enum Configs {
        NumberOfSensors       = 3,
        NumberOfSensorPreamps = 2
    };

    enum class Sensor {
        Shunt = 0,
        Clamp,
        Voltage
    };

    ClampMeterAnalogImplementation()
      : adc{ 0x40, 200 }
      , sensorPreamps{ SensorPreampT{ 0, 9, 100000, 3000000, 50, 500 }, SensorPreampT{ 0, 9, 100000, 3000000, 50, 500 } }
      , powerSupply{ PIOD, ID_PIOD, 31 }
      , outputRelay{ PIOA, ID_PIOA, 21 }
    {
        xTaskCreate(ClampSensorTaskWrapper, "clamp calculator", 500, this, 4, nullptr);
    }

    void SetCalculationCompletionCallback(CalculationCompletionCallback &&new_callback) noexcept
    {
        calcCompletedCallback = std::forward<CalculationCompletionCallback>(new_callback);
    }
    void InsertNewData(const DataType &new_value) noexcept
    {
        //        switch (activeSensor) {
        //        case SensorType::Clamp {
        //            ClampSensorNewDataCallback();
        //          } break;
        //        }

        //        sensors.at(activeSensor).InsertNewData(std::forward<DataType>(new_value);)
        //        insertionCallbacks.at(activeSensor)();
    }
    ValueT GetClampResistance() noexcept { return 1; }   // todo: implement

    [[noreturn]] void Task()
    {
        while (true) {
            AdcDriverT::ValueT adc_value{};
            adc_data_queue = adc.GetDataQueue();
            SetSensor(Sensor::Clamp);
            adc.StartMeasurement();
            int counter = 0;

            while (true) {
                xQueueReceive(adc_data_queue, &adc_value, portMAX_DELAY);

                //                *vShunt = adc_value;
                if (activeSensor != Sensor::Voltage) {
                    sensorPreamps[static_cast<int>(activeSensor)].CheckAmplitudeAndCorrectGainIfNeeded(adc_value);
                }

                //                auto sinprod = sin_table[counter] * 1;
                //                filter.Push(sinprod);
                //
                //                if (counter == (DACC_PACKETLEN - 1))
                //                    counter = 0;
                //                else
                //                    counter++;
            }
        }
    }

  protected:
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

        //        mybuffer = sin_filter5->GetOutputBuffer();

        filter.InsertFilter(std::move(sin_filter1));
        filter.InsertFilter(std::move(sin_filter2));
        filter.InsertFilter(std::move(sin_filter3));
        filter.InsertFilter(std::move(sin_filter4));
        filter.InsertFilter(std::move(sin_filter5));

        //        filter.SetDataReadyCallback([this]() { *vOverall = (*mybuffer)[0]; });
    }

    void InitializeSensorPreamps() noexcept
    {
        // todo: move to preamp initializers
        pio_set_input(PIOA,
                      SH_SENSOR_GAIN_A_PIN | CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN,
                      0);

        pio_set_input(PIOD,
                      CLAMP_SENSOR_GAIN_A_PIN | SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN,
                      0);

        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);

        pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);

        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);

        // clang-format off
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetIninitializingFunctor([](){});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).InvokeInitializer();
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(0, [this]() {shunt_sensor_set_gain(0); adc.SetGain(AdcDriverT::Gain::GAIN_1V3);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(1, [this]() {shunt_sensor_set_gain(0); adc.SetGain(AdcDriverT::Gain::GAIN_1);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(2, [this]() {shunt_sensor_set_gain(1); adc.SetGain(AdcDriverT::Gain::GAIN_1);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(3, [this]() {shunt_sensor_set_gain(2); adc.SetGain(AdcDriverT::Gain::GAIN_1);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(4, [this]() {shunt_sensor_set_gain(3); adc.SetGain(AdcDriverT::Gain::GAIN_1);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(5, [this]() {shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_1);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(6, [this]() {shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_2);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(7, [this]() {shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_4);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(8, [this]() {shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_8);});
    sensorPreamps.at(static_cast<int>(Sensor::Shunt)).SetGainChangeFunctor(9, [this]() {shunt_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_16);});

    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetIninitializingFunctor([](){});
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).InvokeInitializer();
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(0, [this]() {clamp_sensor_set_gain(0); adc.SetGain(AdcDriverT::Gain::GAIN_1V3); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(1, [this]() {clamp_sensor_set_gain(0); adc.SetGain(AdcDriverT::Gain::GAIN_1); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(2, [this]() {clamp_sensor_set_gain(1); adc.SetGain(AdcDriverT::Gain::GAIN_1); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(3, [this]() {clamp_sensor_set_gain(2); adc.SetGain(AdcDriverT::Gain::GAIN_1); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(4, [this]() {clamp_sensor_set_gain(3); adc.SetGain(AdcDriverT::Gain::GAIN_1); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(5, [this]() {clamp_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_1); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(6, [this]() {clamp_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_2); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(7, [this]() {clamp_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_4); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(8, [this]() {clamp_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_8); });
    sensorPreamps.at(static_cast<int>(Sensor::Clamp)).SetGainChangeFunctor(9, [this]() {clamp_sensor_set_gain(4); adc.SetGain(AdcDriverT::Gain::GAIN_16); });
        // clang-format on
    }

    void SetSensor(Sensor new_sensor) noexcept
    {
        using Reference = AdcDriverT::Reference;

        if (new_sensor == Sensor::Voltage) {
            adc.SetMux(Reference::CH4, Reference::CH5);
            activeSensor = Sensor::Voltage;
        }
        else if (new_sensor == Sensor::Shunt) {
            adc.SetMux(Reference::CH2, Reference::CH3);
            activeSensor = Sensor::Shunt;
        }
        else if (new_sensor == Sensor::Clamp) {
            adc.SetMux(Reference::CH0, Reference::CH1);
            activeSensor = Sensor::Clamp;
        }
    }

  private:
    auto constexpr static firstBufferSize = 100;

    // peripherals
    HVPowerSupply                                                       powerSupply;
    OutputRelay                                                         outputRelay;
    OutputGenerator                                                     generator;
    Sensor                                                              activeSensor;
    AdcDriverT                                                          adc;   // todo: make type independent
    std::array<SensorPreamp<AdcDriverT::ValueT>, NumberOfSensorPreamps> sensorPreamps;

    SuperFilter<float, firstBufferSize> filter;
    QueueHandle_t                       adc_data_queue = nullptr;

    // callbacks
    CalculationCompletionCallback calcCompletedCallback;
};
