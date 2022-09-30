#include "maintask.hpp"

#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <asf.h>
#include "system_init.h"
#include "DSP_functions.h"
#include "MCP23016.h"
#include "signal_conditioning.h"
#include "ILI9486_public.h"
#include "menu_ili9486_kbrd_mngr.h"
#include "menu_ili9486.h"

#include "hclamp_meter.hpp"
#include "clamp_sensor.hpp"

#include "FreeRTOS.h"
#include "task.h"

using Drawer             = ClampMeterDrawer<ILIDrawer>;
using Clamp              = ClampMeter<Drawer, ClampSensor>;
using ClampMeterFreeRTOS = ClampMeterInTaskHandler<Drawer, ClampSensor>;

void
tasks_setup()
{
    TestClass<int> tc{ 1 };
    tc.StartTask();

//    auto clamp_meter = Clamp{56};
//
////    clamp_meter.DisplayMeasurementsTask();
//
////    clamp_meter.StartDisplayMeasurementsTask();
//    clamp_meter.StartMeasurementsTask();

    vTaskStartScheduler();

    while (true) {

    }
}

void
ClampMeterMeasurementsTaskWrapper(void *ClampMeterInstance)
{
    auto clmp = static_cast<Clamp *>(ClampMeterInstance);
//    auto clamp_meter = ClampMeterFreeRTOS{ *clmp };

    while (true) {
        clmp->MeasurementsTask();
    }
}

void
ClampMeterDisplayMeasurementsTaskWrapper(void *ClampMeterInstance)
{
    auto clmp = static_cast<Clamp *>(ClampMeterInstance);
//    auto clamp_meter = ClampMeterFreeRTOS{ *clmp };

    while (true) {
        clmp->DisplayMeasurementsTask();
    }
}

void
testclasstask1(void *param)
{
    auto testclass_p      = static_cast<TestClass<int> *>(param);
    auto task_implementer = TestClassTaskImpl<int>{ *testclass_p };

    while (true) {
        task_implementer.DoWork();
    }
}

void
testclasstask2(void *param)
{
    auto testclass_p      = static_cast<TestClass<int> *>(param);
    auto task_implementer = TestClassTaskImpl<int>{ *testclass_p };

    while (true) {
        task_implementer.DoWork2();
    }
}