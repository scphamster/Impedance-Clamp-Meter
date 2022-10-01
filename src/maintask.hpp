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
#include "cmaintask.h"

void tasks_setup(void *);
void simple_setup();

extern "C" void task1wrapper(void *);
extern "C" void testclasstask2(void *);

template<typename dummyT>
class TestClass {
  public:
    TestClass(int testval)
      : someValue{ std::make_shared<int>(testval) }
    {
        testarr = new char[5];
        for (auto idx = 0; idx < sizeof(testarr) - 1; idx++) {
            testarr[idx] = 0xa5;
        }

        testarr[sizeof(testarr) - 1] = '\0';
    }

    ~TestClass() { }

    void StartTask() volatile
    {
        xTaskCreate(task1wrapper, "inclasstask", 300, std::remove_volatile_t<void * const>(this), 3, NULL);
        xTaskCreate(testclasstask2, "inclasstask2", 300, std::remove_volatile_t<void * const>(this), 3, NULL);
    }

    void taskfunc()
    {   // to be called in while loop repeatedly
        {
            std::lock_guard<Mutex> lock{ mutex };
            (*someValue)++;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    void taskfunc2()
    {
        TFT_cursor_set(100, 100);
        TFT_text_color_set(COLOR_GREEN, COLOR_BLACK);
        {
            std::lock_guard<Mutex> lock{ mutex };
            TFT_print_number_f(*someValue, 3, 2);
        }

        vTaskDelay(pdMS_TO_TICKS(400));
    }

  protected:
  private:
    std::shared_ptr<int> someValue;
    Mutex                mutex;
    bool                 mymutex = false;
    char                *testarr;
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