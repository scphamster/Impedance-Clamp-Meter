#include "sensor.hpp"

extern "C" void
InputTaskWrapper(void *sensor_instance)
{
    static_cast<SensorController *>(sensor_instance)->InputTask();
}