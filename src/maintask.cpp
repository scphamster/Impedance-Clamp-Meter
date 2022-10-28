#ifdef min

#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif
#include <type_traits>
#include <asf.h>

#include "system_init.h"
#include "DSP_functions.h"
#include "MCP23016.h"
#include "signal_conditioning.h"
#include "ILI9486_public.h"
#include "menu_ili9486_kbrd_mngr.h"
#include "menu_ili9486.h"
#include "tasks_controller.hpp"

//#include "clamp_sensor.hpp"
#include "FreeRTOS.h"

#include "task.h"
#include "maintask.hpp"
portPOINTER_SIZE_TYPE p_one;
portPOINTER_SIZE_TYPE p_two;

TestClass<int> gtc{5};

void
tasks_setup(void *)
{
    auto tc = TestClass<int>{ 4 };
    tc.StartTask();
//        void task1wrapper(void *);
//        void testclasstask2(void *);

//        xTaskCreate(task1wrapper, "inclasstask", 300, &gtc, 2, NULL);
//        xTaskCreate(testclasstask2, "inclasstask2", 300, &gtc, 2, NULL);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(500));

    }
}

void
simple_setup(void)
{
    static volatile auto tc = TestClass<int>{ 4 };
    tc.StartTask();

    vTaskStartScheduler();

    while (true) {
//        tc.taskfunc();
    }
}

extern "C" void
task1wrapper(void *param)
{
    auto testclass_p      = static_cast<TestClass<int> *>(param);
//    auto task_implementer = TestClassTaskImpl<int>{ *testclass_p };

    while (true) {
        testclass_p->taskfunc();
//            gtc.taskfunc();
        //        task_implementer.DoWork();
    }
}

extern "C" void
testclasstask2(void *param)
{
    auto testclass_p      = static_cast<TestClass<int> *>(param);
//    auto task_implementer = TestClassTaskImpl<int>{ *testclass_p };

    while (true) {
        testclass_p->taskfunc2();
//gtc.taskfunc2();
        //        task_implementer.DoWork2();
    }
}
