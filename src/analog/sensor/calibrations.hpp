#pragma once

#define AMPLITUDE_TOO_HIGH_LIMIT     3000000UL
#define AMPLITUDE_TOO_LOW_LIMIT      100000UL
#define AMPLITUDE_HIGH_COUNTER_LIMIT 50
#define AMPLITUDE_LOW_COUNTER_LIMIT  500

#define SHUNT_SENSOR_MAX_GAIN         4
#define CLAMP_SENSOR_MAX_GAIN         4
#define CLAMP_SENSOR_MAX_OVERALL_GAIN 9
#define SHUNT_SENSOR_MAX_OVERALL_GAIN 9

#define SHUNT_SENSOR_GAIN_COEFFS_NUM 5
#define CLAMP_SENSOR_GAIN_COEFFS_NUM 5
#define COEFFS_OVERALL_NUM           SHUNT_SENSOR_GAIN_COEFFS_NUM + CLAMP_SENSOR_GAIN_COEFFS_NUM + 1
#define COEFFS_FLASH_PAGES_OCCUPIED  1
#define COEFFS_FLASH_START_ADDR      (IFLASH_ADDR + IFLASH_SIZE - IFLASH_PAGE_SIZE * COEFFS_FLASH_PAGES_OCCUPIED)

#define CALIBRATION_EXTERNAL_VOLTAGE_STARTVAL 37.15f

#define CALIBRATION_DATA_NOT_ERASED_MARKER 0xA5
