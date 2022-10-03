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

[[noreturn]] void tasks_setup2();
extern "C" void   ClampMeterMeasurementsTaskWrapper(void *);
extern "C" void   ClampMeterDisplayMeasurementsTaskWrapper(void *);

template<typename Drawer, typename Sensor, typename Keyboard = void>
class ClampMeter : private Sensor {
  public:
    ClampMeter(int initial_val, std::shared_ptr<Drawer> display_to_be_used)
      : someValue{ std::make_shared<int>(initial_val) }
      , display{ display_to_be_used }
    //      , sensor{ std::make_unique<Sensor>() }
    { }

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

    ClampMeter(ClampMeter &&other)          = default;
    ClampMeter &operator=(ClampMeter &&rhs) = default;

  protected:
    virtual void DisplayMeasurementsTask()
    {
        display->SetTextColor(COLOR_BLACK, COLOR_BLACK);

        auto ticks_beg = xTaskGetTickCount();
        auto ticks_end = ticks_beg + 10;

        auto iterations = uint32_t{};

        while(xTaskGetTickCount() != ticks_end) {
            display->Print(1.0, 1, 2);
            iterations++;
        }

        display->SetTextColor(COLOR_GREEN, COLOR_BLACK);
        display->SetCursor({ 100, 100 });
        display->Print(iterations, 2);
        //        display->FillScreen(COLOR_GREEN);
        //        display->FillScreen(COLOR_RED);

        //                vTaskDelay(pdMS_TO_TICKS(100));
    }
    virtual void MeasurementsTask()
    {
        //        display->FillScreen(COLOR_BLACK);

        {
            std::lock_guard<Mutex> lock{ mutex };
            (*someValue)++;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ClampMeter(const ClampMeter &other)
      //      : sensor{ std::make_unique<Sensor>(*other.sensor) }
      : someValue{ other.someValue }
      , display{ other.display }
      , mutex{ other.mutex }
    { }

    ClampMeter &operator=(const ClampMeter &rhs)
    {
        //        sensor    = std::make_unique<Sensor>(*rhs.sensor);
        display   = rhs.display;
        someValue = rhs.someValue;
        mutex     = rhs.mutex;

        return *this;
    }

  private:
    //    std::unique_ptr<Sensor> sensor;
    std::shared_ptr<Drawer> display;
    std::shared_ptr<Keyboard> keyboard;
    std::shared_ptr<int>    someValue;
    Mutex                   mutex;
};

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
