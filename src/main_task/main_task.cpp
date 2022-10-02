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

using Drawer             = ClampMeterDrawer<DisplayDrawer>;
using Clamp              = ClampMeter<Drawer, ClampSensor>;
using ClampMeterFreeRTOS = ClampMeterInTaskHandler<Drawer, ClampSensor>;

[[noreturn]] void
tasks_setup2()
{
    static volatile auto clamp_meter = Clamp{ 56, std::make_shared<Drawer>() };

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
