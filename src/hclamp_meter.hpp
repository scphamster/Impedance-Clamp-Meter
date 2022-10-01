#pragma once
#include <deque>
#include <memory>
#include <mutex>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "mutex/semaphore.hpp"

#include "ILI9486_public.h"
#include "clamp_meter_drawer/clamp_meter_drawing.hpp"

void tasks_setup2();
extern "C" void ClampMeterMeasurementsTaskWrapper(void *);
extern "C" void ClampMeterDisplayMeasurementsTaskWrapper(void *);

template<typename Drawer, typename Sensor>
class ClampMeter : private Sensor {
  public:
    ClampMeter(int initial_val)
      : someValue{ std::make_shared<int>(initial_val) }
    //      : display{ display_to_be_used }
    //      , sensor{ std::make_unique<Sensor>() }
    { }

    void StartMeasurementsTask() volatile
    {


        xTaskCreate(ClampMeterMeasurementsTaskWrapper,
                    "measure",
                    300,
                    std::remove_volatile_t<void *const>(this),
                    2,
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

    void DisplayMeasurementsTask()
    {
        TFT_cursor_set(100, 100);
        TFT_text_color_set(COLOR_RED, COLOR_BLACK);
        {
            std::lock_guard<Mutex> lock{ mutex };
            TFT_print_number_f(*someValue, 3, 2);
        }
        //        auto mutex_taken = mutex.lock(pdMS_TO_TICKS(100));
        //        if (mutex_taken) {
        //        mutex.unlock();
        //        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    void MeasurementsTask()
    {
        TFT_Clear(COLOR_BLACK);
        {
            std::lock_guard<Mutex> lock{ mutex };
            (*someValue)++;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

  protected:
    ClampMeter(const ClampMeter &other)
      //      : sensor{ std::make_unique<Sensor>(*other.sensor) }
      //      , display{ other.display }
      : someValue{ other.someValue }
      , mutex{ other.mutex }
    { }

    ClampMeter &operator=(const ClampMeter &rhs)
    {
        //        sensor    = std::make_unique<Sensor>(*rhs.sensor);
        //        display   = rhs.display;
        someValue = rhs.someValue;
        mutex     = rhs.mutex;

        return *this;
    }

  private:
    //    std::unique_ptr<Sensor> sensor;
    //    std::shared_ptr<Drawer> display;
    std::shared_ptr<int> someValue;
    Mutex                mutex;
};

template<typename Drawer, typename Reader>
class ClampMeterInTaskHandler : private ClampMeter<Drawer, Reader> {
  public:
    ClampMeterInTaskHandler(const ClampMeter<Drawer, Reader> &instance)
      : ClampMeter<Drawer, Reader>{ instance }
    { }

    void DisplayMeasurementsTask() { ClampMeter<Drawer, Reader>::DisplayMeasurementsTask(); }
    void MeasurementsTask() { ClampMeter<Drawer, Reader>::MeasurementsTask(); }

  protected:
  private:
};
