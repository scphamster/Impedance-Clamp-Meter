#include "clamp_meter_driver.hpp"

std::array<std::pair<float,float>, 21> calibration_data;
bool flash_restore_result;
bool flash_save_result;
int error_counter;

void ClampMeterDriver::CalculateAppliedVoltage() noexcept
{
    data.appliedVoltage = data.voltageSensorData.GetValue() - data.shuntSensorData.GetValue();
//    data.AppliedVoltage = std::abs(data.appliedVoltage);
}