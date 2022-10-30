#include "clamp_meter_driver.hpp"

extern "C" void
ClampMeterDriverVoltageSensorTask(void *param)
{
    auto clamp_sensor = static_cast<ClampMeterDriver *>(param);
    clamp_sensor->VoltageSensorTask();
}

extern "C" void
ClampMeterDriverShuntSensorTask(void *param)
{
    auto clamp_sensor = static_cast<ClampMeterDriver *>(param);
    clamp_sensor->ShuntSensorTask();
}

extern "C" void
ClampMeterDriverClampSensorTask(void *param)
{
    auto clamp_sensor = static_cast<ClampMeterDriver *>(param);
    clamp_sensor->ClampSensorTask();
}