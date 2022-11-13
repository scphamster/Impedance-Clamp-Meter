#include "clamp_meter_driver.hpp"

std::array<std::pair<float,float>, 21> calibration_data;
bool flash_restore_result;
bool flash_save_result;
int error_counter;

void ClampMeterDriver::CalculateAppliedVoltage() noexcept
{
    data.AppliedVoltageI = data.voltageSensorData.GetI() - data.shuntSensorData.GetI();
    data.AppliedVoltageQ = data.voltageSensorData.GetQ() - data.shuntSensorData.GetQ();
    arm_sqrt_f32(data.AppliedVoltageI * data.AppliedVoltageI + data.AppliedVoltageQ * data.AppliedVoltageQ,
                 &data.AppliedVoltage);

    data.AppliedVoltagePhi =
      SynchronousIQCalculator<ValueT>::FindAngle(data.AppliedVoltageI, data.AppliedVoltageQ, data.AppliedVoltage);
}