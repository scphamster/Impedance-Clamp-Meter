#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include "voltage_sensor_calculator.hpp"
#include "clamp_sensor_calculator.hpp"
#include "shunt_sensor_calculator.hpp"

class ClampMeterCalculator {
  public:


  protected:
  private:
    VoltageSensorCalculator voltageSensorCalculator;
    ShuntSensorCalculator   shuntSensorCalculator;
    ClampMeterAnalogImplementation clampSensorCalculator;
};
