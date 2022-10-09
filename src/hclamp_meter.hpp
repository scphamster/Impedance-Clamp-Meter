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
    ClampMeter(std::unique_ptr<Drawer> &&display_to_be_used, std::unique_ptr<Keyboard> &&new_keyboard)
      : mutex{ std::make_shared<Mutex>() }
      , timer{ pdMS_TO_TICKS(10) }
      , model{ std::forward<decltype(new_keyboard)>(new_keyboard), mutex }
      , drawer{ mutex, std::forward<decltype(display_to_be_used)>(display_to_be_used) }
    {
#ifdef DEBUG
//        display->SetTextColor(COLOR_RED, COLOR_BLACK);
//        display->SetCursor({ 0, 400 });
//        display->Print("debug", 1);
#endif


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
                    300,
                    std::remove_volatile_t<void *const>(this),
                    3,
                    nullptr);
    }

  protected:
    virtual void DisplayMeasurementsTask();
    virtual void MeasurementsTask() { vTaskDelay(pdMS_TO_TICKS(500)); }

    ClampMeter(const ClampMeter &other)
      : mutex{ other.mutex }
      , timer{ other.timer }
    { }

    ClampMeter &operator=(const ClampMeter &rhs)
    {
        mutex = rhs.mutex;
        timer = rhs.timer;

        return *this;
    }

  private:
    friend class ClampMeterInTaskHandler<Drawer, Sensor>;

    std::shared_ptr<Mutex>            mutex;
    TimerFreeRTOS                     timer;
    MenuModel<Keyboard>               model;
    MenuModelDrawer<Drawer, Keyboard> drawer;
};

template<typename Drawer, typename Sensor, KeyboardC Keyboard>
void
ClampMeter<Drawer, Sensor, Keyboard>::DisplayMeasurementsTask()
{
    drawer.DrawerTask();

    vTaskDelay(pdMS_TO_TICKS(500));
}

template<typename Drawer, typename Reader>
class ClampMeterInTaskHandler {
  public:
    //    ClampMeterInTaskHandler(const ClampMeter<Drawer, Reader> &instance)
    //      : ClampMeter<Drawer, Reader>{ instance }
    //    { }
    //
    //    void DisplayMeasurementsTask() override { ClampMeter<Drawer, Reader>::DisplayMeasurementsTask(); }
    //    void MeasurementsTask() override { ClampMeter<Drawer, Reader>::MeasurementsTask(); }

    ClampMeterInTaskHandler(ClampMeter<Drawer, Reader> &instance)
      : true_instance{ instance }
    { }

    void DisplayMeasurementsTask() { true_instance.DisplayMeasurementsTask(); }
    void MeasurementsTask() { true_instance.MeasurementsTask(); }

  protected:
  private:
    ClampMeter<Drawer, Reader> &true_instance;
};
