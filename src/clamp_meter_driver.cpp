#include "clamp_meter_driver.hpp"

extern "C" void
ClampMeterDriverMainTask(void *param)
{
    auto clamp_sensor = static_cast<ClampMeterDriver *>(param);
    clamp_sensor->MainTask();
}
