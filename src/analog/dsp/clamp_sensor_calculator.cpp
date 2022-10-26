#include "clamp_sensor_calculator.hpp"

extern "C" void
ClampSensorTaskWrapper(void *param)
{
    auto clamp_sensor = static_cast<ClampMeterAnalogImplementation *>(param);
    clamp_sensor->Task();
}
