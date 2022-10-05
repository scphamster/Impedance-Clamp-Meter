#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include "hclamp_meter.hpp"
#include "main_task.hpp"
#include "clamp_sensor.hpp"
#include "task.h"
#include "display_drawer.hpp"
#include "ili9486_driver.hpp"
#include "FreeRTOS/include/timers.h"

using Drawer             = DisplayDrawer<ILI9486Driver>;
using Clamp              = ClampMeter<Drawer, ClampSensor>;
using ClampMeterFreeRTOS = ClampMeterInTaskHandler<Drawer, ClampSensor>;

bool ILI9486Driver::isInitialized = false;


[[noreturn]] void
tasks_setup2()
{
    static volatile auto clamp_meter =
      Clamp{ 56, std::make_shared<DisplayDrawer<ILI9486Driver>>(std::make_shared<ILI9486Driver>()) };

    //    clamp_meter.StartMeasurementsTask();
    clamp_meter.StartDisplayMeasurementsTask();

    vTaskStartScheduler();

    while (true) { }
}

extern "C" void
ClampMeterMeasurementsTaskWrapper(void *ClampMeterInstance)
{
    auto clmp        = static_cast<Clamp *>(ClampMeterInstance);
    auto clamp_meter = ClampMeterFreeRTOS{ *clmp };

    while (true) {
        //        clmp->MeasurementsTask();
        clamp_meter.MeasurementsTask();
    }
}

extern "C" void
ClampMeterDisplayMeasurementsTaskWrapper(void *ClampMeterInstance)
{
    auto clmp        = static_cast<Clamp *>(ClampMeterInstance);
    auto clamp_meter = ClampMeterFreeRTOS{ *clmp };

    while (true) {
        //        clmp->DisplayMeasurementsTask();
        clamp_meter.DisplayMeasurementsTask();
    }
}
