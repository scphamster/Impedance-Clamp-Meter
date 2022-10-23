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
#include <mutex>
#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

#include "mutex/semaphore.hpp"
#include "clamp_meter_drawing.hpp"
#include "clamp_meter_concepts.hpp"

#include "menu_model.hpp"
#include "menu_model_item.hpp"
#include "menu_model_drawer.hpp"

#include "mcp3462_driver.hpp"
#include "sensor_preamp.hpp"
#include "queue.h"

#include "filter.hpp"

// test
#include "signal_conditioning.h"
#include "system_init.h"
#include "timer.hpp"
#include "mcp23016_driver_fast.hpp"
#include "button.hpp"
#include "keyboard_fast.hpp"

[[noreturn]] void tasks_setup2();
extern "C" void   ClampMeterMeasurementsTaskWrapper(void *);
extern "C" void   ClampMeterDisplayMeasurementsTaskWrapper(void *);
extern "C" void   ClampMeterAnalogTaskWrapper(void *);

template<typename Drawer>
class ClampMeterInTaskHandler;

auto constexpr firstBufferSize = 100;

template<typename Drawer, KeyboardC Keyboard = Keyboard<MCP23016_driver, TimerFreeRTOS, MCP23016Button>>
class ClampMeter {
  public:
    using Page           = MenuModelPage<Keyboard>;
    using Model          = MenuModel<Keyboard>;
    using ModelItemDataT = std::shared_ptr<MenuModelPageItemData>;
    using SensorPreampT  = SensorPreamp<MCP3462_driver::ValueT>;

    enum class Sensor {
        Voltage = 0,
        Shunt,
        Clamp
    };

    enum Configs {
        NumberOfSensors = 3
    };

    ClampMeter(std::unique_ptr<Drawer> &&display_to_be_used, std::unique_ptr<Keyboard> &&new_keyboard)
      : model{ std::make_shared<Model>() }
      , drawer{ std::forward<decltype(display_to_be_used)>(display_to_be_used),
                std::forward<decltype(new_keyboard)>(new_keyboard) }
      , adc{ 0x40, 200 }
      , sensorPreamps{ SensorPreampT{ 0, 4, 100000, 3000000, 50, 500 },
                       SensorPreampT{ 0, 4, 100000, 3000000, 50, 500 },
                       SensorPreampT{ 0, 4, 100000, 3000000, 50, 500 } }
      , vOverall{ std::make_shared<MenuModelPageItemData>(static_cast<float>(0)) }
      , vShunt{ std::make_shared<MenuModelPageItemData>(static_cast<float>(0)) }
      , zClamp{ std::make_shared<MenuModelPageItemData>(static_cast<float>(0)) }
      , sensorMag{ std::make_shared<MenuModelPageItemData>(static_cast<float>(0)) }
      , sensorPhi{ std::make_shared<MenuModelPageItemData>(static_cast<float>(0)) }
    {
        {
            auto vout_info = std::make_shared<Page>(model);
            vout_info->SetName("V out");
            vout_info->SetData(vOverall);
            vout_info->SetIndex(0);

            auto iout_info = std::make_shared<Page>(model);
            iout_info->SetName("V shunt");
            iout_info->SetData(vShunt);
            iout_info->SetIndex(1);

            auto zout_info = std::make_shared<Page>(model);
            zout_info->SetName("Z out");
            zout_info->SetData(zClamp);
            zout_info->SetIndex(2);

            auto measurements_page = std::make_shared<Page>(model);
            measurements_page->SetIndex(0);
            measurements_page->SetName("Measurement");
            measurements_page->InsertChild(vout_info);
            measurements_page->InsertChild(iout_info);
            measurements_page->InsertChild(zout_info);
            measurements_page->SetKeyCallback(Keyboard::ButtonName::F1, []() { measurement_start(); });
            measurements_page->SetKeyCallback(Keyboard::ButtonName::F2, []() { measurement_stop(); });
            measurements_page->SetKeyCallback(Keyboard::ButtonName::F3, []() {
                if (Analog.generator_is_active) {
                    switch (Analog.selected_sensor) {
                    case VOLTAGE_SENSOR: ::switch_sensing_chanel(SHUNT_SENSOR); break;

                    case SHUNT_SENSOR:
                        ::switch_sensing_chanel(CLAMP_SENSOR);
                        ::buzzer_enable();
                        break;

                    case CLAMP_SENSOR:
                        ::switch_sensing_chanel(VOLTAGE_SENSOR);
                        ::buzzer_disable();
                        break;
                    }
                }
            });

            auto calibration_page0_vout_value = std::make_shared<Page>(model);
            calibration_page0_vout_value->SetName("magnitude = ");
            calibration_page0_vout_value->SetData(sensorMag);
            calibration_page0_vout_value->SetIndex(1);

            auto calibration_page0_phi_value = std::make_shared<Page>(model);
            calibration_page0_phi_value->SetName("phi = ");
            calibration_page0_phi_value->SetData(sensorPhi);
            calibration_page0_phi_value->SetIndex(2);

            auto calibration_G1 = std::make_shared<Page>(model);
            calibration_G1->SetName("Enter if data is stable");

            auto calibration_G0 = std::make_shared<Page>(model);
            calibration_G0->SetName("Put resistor 6.73kOhm");
            calibration_G0->SetEventCallback(Page::Event::Entrance,
                                             []() { calibration_semiauto(CALIBRATOR_GO_NEXT_STEP); });
            calibration_G0->SetHeader("Calibration");
            calibration_G0->InsertChild(calibration_G1);
            calibration_G0->InsertChild(calibration_page0_vout_value);
            calibration_G0->InsertChild(calibration_page0_phi_value);
            calibration_G0->SetKeyCallback(Keyboard::ButtonName::F1,
                                           [calibration_G1]() { calibration_semiauto(CALIBRATOR_GO_NEXT_STEP); });

            auto calibration_vout = std::make_shared<Page>(model);
            calibration_vout->SetName("enter if data is stable");
            calibration_vout->SetHeader("Calibration");
            calibration_vout->SetEventCallback(Page::Event::Entrance, []() {
                calibration_semiauto(CALIBRATOR_GO_NEXT_STEP);
                calibration_semiauto(CALIBRATOR_GO_NEXT_STEP);
                calibration_semiauto(CALIBRATOR_GO_NEXT_STEP);   // warning g0 shown
            });
            calibration_vout->InsertChild(calibration_G0);

            auto start_calibration = std::make_shared<Page>(model);
            start_calibration->SetName("enter to start");
            start_calibration->SetHeader("Calibration");
            start_calibration->InsertChild(calibration_vout);
            start_calibration->InsertChild(calibration_page0_vout_value);
            start_calibration->InsertChild(calibration_page0_phi_value);
            start_calibration->SetEventCallback(Page::Event::Entrance, []() {
                calibration_semiauto(CALIBRATOR_START);
                calibration_semiauto(CALIBRATOR_GO_NEXT_STEP);
                calibration_semiauto(CALIBRATOR_GO_NEXT_STEP);
            });

            auto calibration_page = std::make_shared<Page>(model);
            calibration_page->SetIndex(1);
            calibration_page->SetName("Calibration");
            calibration_page->InsertChild(start_calibration);

            auto main_page = std::make_shared<Page>(model);
            main_page->SetName("Main");
            main_page->SetHeader(" ");
            main_page->InsertChild(measurements_page);
            main_page->InsertChild(calibration_page);

            model->SetTopLevelItem(main_page);
            drawer.SetModel(model);
        }

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

            auto sin_filter2 =
              std::make_unique<FirDecimatingFilter<fir1_dec_blocksize, fir1_dec_ncoeffs, fir_dec_factor>>(
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

            auto sin_filter4 =
              std::make_unique<FirDecimatingFilter<fir2_dec_blocksize, fir2_dec_ncoeffs, fir_dec_factor>>(
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

            filter.InsertFilter(std::move(sin_filter1));
            filter.InsertFilter(std::move(sin_filter2));
            filter.InsertFilter(std::move(sin_filter3));
            filter.InsertFilter(std::move(sin_filter4));
            filter.InsertFilter(std::move(sin_filter5));
        }
        StartDisplayMeasurementsTask();
        StartMeasurementsTask();
        StartAnalogTask();
    }

    ClampMeter(ClampMeter &&other)          = default;
    ClampMeter &operator=(ClampMeter &&rhs) = default;
    ~ClampMeter()                           = default;

    void StartMeasurementsTask() volatile
    {
        xTaskCreate(ClampMeterMeasurementsTaskWrapper,
                    "measure",
                    300,
                    std::remove_volatile_t<void *const>(this),
                    3,
                    nullptr);
    }

    void StartDisplayMeasurementsTask() volatile
    {
        xTaskCreate(ClampMeterDisplayMeasurementsTaskWrapper,
                    "display",
                    400,
                    std::remove_volatile_t<void *const>(this),
                    2,
                    nullptr);
    }

    void StartAnalogTask() volatile
    {
        xTaskCreate(ClampMeterAnalogTaskWrapper, "analog", 200, std::remove_volatile_t<void *const>(this), 4, nullptr);
    }

  protected:
    void DisplayMeasurementsTask()
    {
        drawer.DrawerTask();
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    void MeasurementsTask()
    {
        if (Analog.generator_is_active) {
            dsp_integrating_filter();

            if (clamp_measurements_result.new_data_is_ready) {
                clamp_measurements_result.new_data_is_ready = false;
                *vOverall                                   = clamp_measurements_result.V_ovrl;
                *vShunt                                     = clamp_measurements_result.V_shunt;
                *zClamp                                     = clamp_measurements_result.Z_clamp;
            }

            if ((Calibrator.is_calibrating) && (Calibrator.new_data_is_ready)) {
                //                calibration_display_measured_data();
                *sensorMag                   = Calibrator.sensor_mag;
                *sensorPhi                   = Calibrator.sensor_phi;
                Calibrator.new_data_is_ready = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    [[noreturn]] void AnalogTask()
    {
        MCP3462_driver::ValueT adc_value{};
        adc_data_queue = adc.GetDataQueue();

        adc.SetGain(MCP3462_driver::Gain::GAIN_1);
        adc.SetMux(MCP3462_driver::Reference::AVDD, MCP3462_driver::Reference::AGND);

        adc.EnableClock();
        int counter = 0;
        while (true) {
            xQueueReceive(adc_data_queue, &adc_value, portMAX_DELAY);

            counter++;

            if (counter % 100 == 0)
                *vOverall = adc_value;

            //            sensorPreamps[activeSensor].CheckAmplitudeAndCorrectGainIfNeeded(adc_value);
        }
    }

  private:
    friend class ClampMeterInTaskHandler<Drawer>;

    std::shared_ptr<MenuModel<Keyboard>> model;
    MenuModelDrawer<Drawer, Keyboard>    drawer;

    MCP3462_driver adc;
    QueueHandle_t  adc_data_queue = nullptr;

    SuperFilter<float, firstBufferSize> filter;

    std::array<SensorPreamp<MCP3462_driver::ValueT>, NumberOfSensors> sensorPreamps;
    Sensor                                                            activeSensor = Sensor::Voltage;
    //    filter;
    //    dsp;

    ModelItemDataT vOverall;
    ModelItemDataT vShunt;
    ModelItemDataT zClamp;
    ModelItemDataT sensorMag;
    ModelItemDataT sensorPhi;
};

template<typename Drawer>
class ClampMeterInTaskHandler {
  public:
    ClampMeterInTaskHandler(ClampMeter<Drawer> &instance)
      : true_instance{ instance }
    { }

    void DisplayMeasurementsTask() { true_instance.DisplayMeasurementsTask(); }
    void MeasurementsTask() { true_instance.MeasurementsTask(); }
    void AnalogTask() { true_instance.AnalogTask(); }

  protected:
  private:
    ClampMeter<Drawer> &true_instance;
};
