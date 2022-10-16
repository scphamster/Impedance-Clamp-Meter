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

#include "signal_conditioning.h"
#include "system_init.h"
// test
#include "timer.hpp"
#include "mcp23016_driver_fast.hpp"
#include "button.hpp"
#include "keyboard_fast.hpp"

[[noreturn]] void tasks_setup2();
extern "C" void   ClampMeterMeasurementsTaskWrapper(void *);
extern "C" void   ClampMeterDisplayMeasurementsTaskWrapper(void *);

template<typename Drawer, typename Reader>
class ClampMeterInTaskHandler;

template<typename Drawer, typename Sensor, KeyboardC Keyboard = Keyboard<MCP23016_driver, TimerFreeRTOS, MCP23016Button>>
class ClampMeter : private Sensor {
  public:
    using Item          = MenuModelPageItem<Keyboard>;
    using Model         = MenuModel<Keyboard>;
    using ModelItemData = std::shared_ptr<MenuModelPageItemData>;

    ClampMeter(std::unique_ptr<Drawer> &&display_to_be_used, std::unique_ptr<Keyboard> &&new_keyboard)
      : mutex{ std::make_shared<Mutex>() }
      , timer{ pdMS_TO_TICKS(10) }
      , model{ std::make_shared<Model>(mutex) }
      , drawer{ mutex,
                std::forward<decltype(display_to_be_used)>(display_to_be_used),
                std::forward<decltype(new_keyboard)>(new_keyboard) }
      , vOverall{ std::make_shared<MenuModelPageItemData>(static_cast<float>(0)) }
      , iOverallI{ std::make_shared<MenuModelPageItemData>(static_cast<float>(0)) }
      , zClamp{ std::make_shared<MenuModelPageItemData>(static_cast<float>(0)) }
    {
        auto vout_info = std::make_shared<Item>(model);
        vout_info->SetName("V out");
        vout_info->SetData(vOverall);
        vout_info->SetIndex(0);

        auto iout_info = std::make_shared<Item>(model);
        iout_info->SetName("I out");
        iout_info->SetData(iOverallI);
        iout_info->SetIndex(1);

        auto zout_info = std::make_shared<Item>(model);
        zout_info->SetName("Z out");
        zout_info->SetData(zClamp);
        zout_info->SetIndex(2);

        auto measurements_page = std::make_shared<Item>(model);
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

//        auto calibration_page0 = std::make_shared<Item>(model);
//        auto calibration_page0 = std::make_shared<Item>(model);
//        auto calibration_page0 = std::make_shared<Item>(model);
//        auto calibration_page0 = std::make_shared<Item>(model);
//        auto calibration_page0 = std::make_shared<Item>(model);
        auto calibration_page0 = std::make_shared<Item>(model);
//        calibration_page0->SetName
        auto start_calibration = std::make_shared<Item>(model);
        start_calibration->SetIndex(0);
        start_calibration->SetName("press ENTER to start");

        auto calibration_page = std::make_shared<Item>(model);
        calibration_page->SetIndex(1);
        calibration_page->SetName("Calibration");
        calibration_page->InsertChild(start_calibration);

        auto main_page = std::make_shared<Item>(model);
        main_page->SetName("Main");
        main_page->InsertChild(measurements_page);
        main_page->InsertChild(calibration_page);

        model->SetTopLevelItem(main_page);
        drawer.SetModel(model);

        StartDisplayMeasurementsTask();
        StartMeasurementsTask();
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
                    4,
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

  protected:
    virtual void DisplayMeasurementsTask()
    {
        drawer.DrawerTask();
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    virtual void MeasurementsTask()
    {
        if (Analog.generator_is_active) {
            dsp_integrating_filter();

            if (clamp_measurements_result.new_data_is_ready) {
                clamp_measurements_result.new_data_is_ready = false;
                *vOverall                                   = clamp_measurements_result.V_ovrl;
                *iOverallI                                  = clamp_measurements_result.I_ovrl_I_norm;
                *zClamp                                     = clamp_measurements_result.Z_clamp;
            }

            if ((Calibrator.is_calibrating) && (Calibrator.new_data_is_ready)) {
                calibration_display_measured_data();
                Calibrator.new_data_is_ready = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

  private:
    friend class ClampMeterInTaskHandler<Drawer, Sensor>;

    std::shared_ptr<Mutex>               mutex;
    TimerFreeRTOS                        timer;
    std::shared_ptr<MenuModel<Keyboard>> model;
    MenuModelDrawer<Drawer, Keyboard>    drawer;

    ModelItemData vOverall;
    ModelItemData iOverallI;
    ModelItemData zClamp;
};

template<typename Drawer, typename Reader>
class ClampMeterInTaskHandler {
  public:
    ClampMeterInTaskHandler(ClampMeter<Drawer, Reader> &instance)
      : true_instance{ instance }
    { }

    void DisplayMeasurementsTask() { true_instance.DisplayMeasurementsTask(); }
    void MeasurementsTask() { true_instance.MeasurementsTask(); }

  protected:
  private:
    ClampMeter<Drawer, Reader> &true_instance;
};
