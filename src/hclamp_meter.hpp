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

// test
#include "timer.hpp"
#include "mcp23016_driver_fast.hpp"
#include "button.hpp"
#include "keyboard_fast.hpp"

[[noreturn]] void tasks_setup2();
extern "C" void   ClampMeterMeasurementsTaskWrapper(void *);
extern "C" void   ClampMeterDisplayMeasurementsTaskWrapper(void *);

template<typename Drawer, typename Sensor, KeyboardC Keyboard = Keyboard<MCP23016_driver, TimerFreeRTOS, MCP23016Button>>
class ClampMeter : private Sensor {
  public:
    ClampMeter(std::shared_ptr<Drawer> display_to_be_used, std::unique_ptr<Keyboard> &&new_keyboard)
      : display{ display_to_be_used }
      , keyboard{ std::move(new_keyboard) }
      , timer{ pdMS_TO_TICKS(10) }
    //      , sensor{ std::make_unique<Sensor>() }
    {
#ifdef DEBUG
        display->SetTextColor(COLOR_RED, COLOR_BLACK);
        display->SetCursor({ 0, 400 });
        display->Print("debug", 1);
#endif

        keyboard->SetButtonEventCallback(2, Keyboard::ButtonEvent::Release, [this]() {
            display->SetCursor({ 0, 0 });
            display->SetTextColor(COLOR_RED, COLOR_BLACK);
            display->Print("dupa invoked 2", 1);
        });
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

    virtual void DisplayMeasurementsTask();

  protected:
    virtual void MeasurementsTask()
    {
        //        display->FillScreen(COLOR_BLACK);

        {
            std::lock_guard<Mutex> lock{ mutex };
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ClampMeter(const ClampMeter &other)
      : display{ other.display }
      , keyboard{ std::make_unique<Keyboard>(*other.keyboard) }
      , mutex{ other.mutex }
      , timer{ other.timer }
    { }

    ClampMeter &operator=(const ClampMeter &rhs)
    {
        display  = rhs.display;
        keyboard = std::make_unique<Keyboard>(*rhs.keyboard);
        mutex    = rhs.mutex;
        timer    = rhs.timer;

        return *this;
    }

  private:
    //    std::unique_ptr<Sensor> sensor;
    std::shared_ptr<Drawer>   display;
    std::unique_ptr<Keyboard> keyboard;
    Mutex                     mutex;
    TimerFreeRTOS             timer;
};

template<typename Drawer, typename Sensor, KeyboardC Keyboard>
void
ClampMeter<Drawer, Sensor, Keyboard>::DisplayMeasurementsTask()
{
    //    display->SetTextColor(COLOR_BLACK, COLOR_BLACK);
    //
    //    auto ticks_beg = xTaskGetTickCount();
    //    auto ticks_end = ticks_beg + 10;
    //
    //    auto iterations = uint32_t{};
    //
    //    while (xTaskGetTickCount() != ticks_end) {
    //        display->Print(1.0, 1, 2);
    //        iterations++;
    //    }
    //
    //    display->SetTextColor(COLOR_GREEN, COLOR_BLACK);
    //    display->SetCursor({ 100, 100 });
    //    display->Print(iterations, 2);

    vTaskDelay(pdMS_TO_TICKS(100));

    //        display->FillScreen(COLOR_GREEN);
    //        display->FillScreen(COLOR_RED);

    //                vTaskDelay(pdMS_TO_TICKS(100));
}

template<typename Drawer, typename Reader>
class ClampMeterInTaskHandler : private ClampMeter<Drawer, Reader> {
  public:
    ClampMeterInTaskHandler(const ClampMeter<Drawer, Reader> &instance)
      : ClampMeter<Drawer, Reader>{ instance }
    { }

    void DisplayMeasurementsTask() override { ClampMeter<Drawer, Reader>::DisplayMeasurementsTask(); }
    void MeasurementsTask() override { ClampMeter<Drawer, Reader>::MeasurementsTask(); }

  protected:
  private:
};
