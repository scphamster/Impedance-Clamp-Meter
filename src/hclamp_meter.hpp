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

template<typename Drawer, typename Sensor>
class ClampMeter : private Sensor {
  public:
    ClampMeter(int initial_val)
      : someValue{ std::make_shared<int>(initial_val) }
    //      : display{ display_to_be_used }
    //      , sensor{ std::make_unique<Sensor>() }
    { }

    void StartMeasurementsTask()
    {
        void ClampMeterMeasurementsTaskWrapper(void *);

        xTaskCreate(ClampMeterMeasurementsTaskWrapper, "measure", 300, this, 2, nullptr);
    }

    void StartDisplayMeasurementsTask()
    {
        void ClampMeterDisplayMeasurementsTaskWrapper(void *);

        xTaskCreate(ClampMeterDisplayMeasurementsTaskWrapper, "display", 300, this, 2, nullptr);
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
    static void DisplayMeasurementsTaskWrapper(void *param);

    void MeasurementsTask()
    {
        TFT_Clear(COLOR_BLACK);
        vTaskDelay(pdMS_TO_TICKS(500));

        TFT_Clear(COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(500));
        //test
        TFT_print_number_f(*someValue, 3, 2);
        {
            std::lock_guard<Mutex> lock{ mutex };
            (*someValue)++;
        }
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

template<typename dummyT>
class TestClass {
  public:
    TestClass(int testval)
      : someValue{ std::make_shared<int>(testval) }
    { }

    void StartTask()
    {
        void testclasstask1(void *);
        void testclasstask2(void *);

        xTaskCreate(testclasstask1, "inclasstask", 300, this, 2, NULL);
        xTaskCreate(testclasstask2, "inclasstask2", 300, this, 3, NULL);
    }

  protected:
    void taskfunc()
    {   // to be called in while loop repeatedly
        TFT_Clear(COLOR_BLACK);
        vTaskDelay(pdMS_TO_TICKS(500));

        TFT_Clear(COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(500));

        {
            std::lock_guard<Mutex> lock{ mutex };
            (*someValue)++;
        }

        //        auto mutex_taken = mutex.lock(pdMS_TO_TICKS(100));
        //        if (mutex_taken) {
        //            mutex.unlock();
        //        }
    }

    void taskfunc2()
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

  private:
    std::shared_ptr<int> someValue;
    Mutex                mutex;
};

template<typename T>
class TestClassTaskImpl : public TestClass<T> {
  public:
    TestClassTaskImpl(const TestClass<T> &srce)
      : TestClass<T>{ std::forward<const TestClass<T>>(srce) }
    { }

    TestClassTaskImpl(TestClass<T> &&srce)
      : TestClass<T>{ std::forward<TestClass<T>>(srce) }
    { }

    void DoWork() { TestClass<T>::taskfunc(); }

    void DoWork2() { TestClass<T>::taskfunc2(); }
};