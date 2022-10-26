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

// #include "mcp3462_driver.hpp"
#include "sensor_preamp.hpp"
#include "queue.h"

#include "filter.hpp"
#include "HVPowerSupply.hpp"
#include "OutputRelay.hpp"
#include "output_generator.hpp"
#include "clamp_sensor_calculator.hpp"

// test
#include "signal_conditioning.h"
#include "system_init.h"
#include "timer.hpp"
#include "mcp23016_driver_fast.hpp"
#include "button.hpp"
#include "keyboard_fast.hpp"

// todo refactor headers below into statics
#include "external_periph_ctrl.h"
#include "signal_conditioning.h"

[[noreturn]] void tasks_setup2();
extern "C" void   ClampMeterMeasurementsTaskWrapper(void *);
extern "C" void   ClampMeterDisplayMeasurementsTaskWrapper(void *);
extern "C" void   ClampMeterAnalogTaskWrapper(void *);

template<typename Drawer>
class ClampMeterInTaskHandler;

template<typename DrawerT, KeyboardC Keyboard = Keyboard<MCP23016_driver, TimerFreeRTOS, MCP23016Button>>
class ClampMeter {
  public:
    using Page      = MenuModelPage<Keyboard>;
    using Menu      = MenuModel<Keyboard>;
    using PageDataT = std::shared_ptr<UniversalSafeType>;
    using Drawer    = std::unique_ptr<DrawerT>;

    ClampMeter(std::unique_ptr<DrawerT> &&display_to_be_used, std::unique_ptr<Keyboard> &&new_keyboard)
      : model{ std::make_shared<Menu>() }
      , drawer{ std::forward<decltype(display_to_be_used)>(display_to_be_used),
                std::forward<decltype(new_keyboard)>(new_keyboard) }
      , vOverall{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }    // test
      , vShunt{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }      // test
      , zClamp{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }      // test
      , sensorMag{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }   // test
      , sensorPhi{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }   // test
    {
        InitializeMenu();
        //        InitializeFilters();
        //        InitializeSensorPreamps();

        InitializeClampMeter();
        InitializeTasks();
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
                    3,
                    nullptr);
    }
    void StartAnalogTask() volatile
    {
        xTaskCreate(ClampMeterAnalogTaskWrapper, "analog", 500, std::remove_volatile_t<void *const>(this), 3, nullptr);
    }

  protected:
    void DisplayMeasurementsTask() noexcept
    {
        drawer.DrawerTask();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    void MeasurementsTask() noexcept
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
    [[noreturn]] void AnalogTask() noexcept { }

    void InitializeMenu() noexcept
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
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F1, [this]() {
            //            generator.StartGenerating();
            //            vTaskDelay(pdMS_TO_TICKS(1000));
            //            powerSupply.Activate();
            //            vTaskDelay(pdMS_TO_TICKS(1000));
            //            outputRelay.Activate();
            /*measurement_star1t();*/
        });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F2, [this]() {
            //            outputRelay.Deactivate();
            //            generator.StopGenerating();
            //            powerSupply.Deactivate();
            //            vTaskDelay(pdMS_TO_TICKS(200));
        });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F3, []() {
            //            if (Analog.generator_is_active) {
            //                switch (Analog.selected_sensor) {
            //                case VOLTAGE_SENSOR: ::switch_sensing_chanel(SHUNT_SENSOR); break;
            //
            //                case SHUNT_SENSOR:
            //                    ::switch_sensing_chanel(CLAMP_SENSOR);
            //                    ::buzzer_enable();
            //                    break;
            //
            //                case CLAMP_SENSOR:
            //                    ::switch_sensing_chanel(VOLTAGE_SENSOR);
            //                    ::buzzer_disable();
            //                    break;
            //                }
            //            }
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
        calibration_G0->SetEventCallback(Page::Event::Entrance, []() { calibration_semiauto(CALIBRATOR_GO_NEXT_STEP); });
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
    void InitializeClampMeter() noexcept
    {   // todo: implement
        clampMeter.SetCalculationCompletionCallback([this]() { *zClamp = clampMeter.GetClampResistance(); });
    }
    void InitializeTasks() noexcept
    {
        StartDisplayMeasurementsTask();
        //        StartMeasurementsTask();
        StartAnalogTask();
    }

    void ConnectPeripherals()
    {
        // connect(adc, clampMeter);
        // peripherals.at("adc").setDataArrivalCallback([this](new_value){clampMeter.InsertValue(new_adc_value);});
    }

  private:
    friend class ClampMeterInTaskHandler<DrawerT>;

    ClampMeterAnalogImplementation       clampMeter;
    std::shared_ptr<MenuModel<Keyboard>> model;
    MenuModelDrawer<DrawerT, Keyboard>   drawer;

    // todo: to be deleted from here
    std::shared_ptr<std::array<float, 1>> mybuffer;
    PageDataT                             vOverall;
    PageDataT                             vShunt;
    PageDataT                             zClamp;
    PageDataT                             sensorMag;
    PageDataT                             sensorPhi;
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
