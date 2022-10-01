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
#include "asf.h"
#include "MCP3462.h"
#include "arm_math.h"
#include "DSP_functions.h"
#include "menu_calibration.h"

#define AMPLITUDE_TOO_HIGH_LIMIT	3000000UL
#define AMPLITUDE_TOO_LOW_LIMIT		100000UL
#define AMPLITUDE_HIGH_COUNTER_LIMIT	50
#define AMPLITUDE_LOW_COUNTER_LIMIT		500

#define SHUNT_SENSOR_MAX_GAIN	4
#define CLAMP_SENSOR_MAX_GAIN	4
#define CLAMP_SENSOR_MAX_OVERALL_GAIN	9
#define SHUNT_SENSOR_MAX_OVERALL_GAIN	9

#define SHUNT_SENSOR_GAIN_COEFFS_NUM	5
#define CLAMP_SENSOR_GAIN_COEFFS_NUM	5
#define COEFFS_OVERALL_NUM				SHUNT_SENSOR_GAIN_COEFFS_NUM + \
CLAMP_SENSOR_GAIN_COEFFS_NUM + 1
#define COEFFS_FLASH_PAGES_OCCUPIED		1
#define COEFFS_FLASH_START_ADDR	(IFLASH_ADDR + IFLASH_SIZE - IFLASH_PAGE_SIZE * \
COEFFS_FLASH_PAGES_OCCUPIED)

#define CALIBRATION_EXTERNAL_VOLTAGE_STARTVAL	37.15f


#define CALIBRATION_DATA_NOT_ERASED_MARKER 0xA5

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SHUNT_SENSOR,
	CLAMP_SENSOR,
	VOLTAGE_SENSOR
} sensor_type_t;

typedef enum {
	MEASUREMENT_MODE_MANUAL = 0,
	MEASUREMENT_MODE_1
} measurement_mode_type_t;

extern bool flash_test_runing;

typedef struct {
	bool error_occured			: 1;
	bool is_calibrated			: 1;
	bool AGC_on					: 1;
	bool generator_is_active	: 1;
	bool ampl_is_too_low		: 1;
	bool ampl_is_too_high		: 1;

	uint16_t generator_amplitude;

	sensor_type_t selected_sensor;

	uint16_t ampl_too_low_counter;
	uint16_t ampl_too_high_counter;

	gain_type_t adc_gain;
	uint8_t shunt_sensor_gain;
	uint8_t clamp_sensor_gain;
	uint8_t overall_gain;

	refsel_type_t pos_ch;
	refsel_type_t neg_ch;

	measurement_mode_type_t mes_mode;
} Analog_t;

extern Analog_t Analog;

typedef struct {
	bool is_calibrating;
	
	bool new_data_is_ready;
	
	calibration_phase_type_t cal_phase;
	uint8_t cal_phase_num;

	float32_t externally_measured_vout;
	float32_t vout_ovrl;
	float32_t vout_phi;
	float32_t vout_phi_noload;
	float32_t v_sh;
	float32_t current_phi;

	float32_t sensor_mag;
	float32_t sensor_phi;

	float32_t v_sens_gain;
	float32_t clamp_gains[CLAMP_SENSOR_GAIN_COEFFS_NUM];
	float32_t shunt_gains[SHUNT_SENSOR_GAIN_COEFFS_NUM];
	float32_t clamp_angles[CLAMP_SENSOR_GAIN_COEFFS_NUM];
	float32_t shunt_angles[SHUNT_SENSOR_GAIN_COEFFS_NUM];

	bool error_occured;
} Calibrator_t;

extern Calibrator_t Calibrator;

typedef struct {
	float32_t shunt_gain[SHUNT_SENSOR_GAIN_COEFFS_NUM];
	float32_t clamp_gain[CLAMP_SENSOR_GAIN_COEFFS_NUM];

	float32_t shunt_phi[SHUNT_SENSOR_GAIN_COEFFS_NUM];
	float32_t clamp_phi[CLAMP_SENSOR_GAIN_COEFFS_NUM];

	float32_t v_sens_gain;
	float32_t v_sens_phi;

	uint8_t data_password;
} calibration_data_t;

extern calibration_data_t Cal_data;

static calibration_data_t Reserve_cal_data = {
	//Shunt gains
	53784.875f,
	132666.203f,
	264311.406f,
	8696278.f,
	17165270.f,
	
	//clamp gain
	338750944.f,
	645587648.f,
	955409024.f,
	1.53519985e+010f,
	1.50437837e+010f,

	//shunt phi
	90.9220352f,
	90.9662323f,
	91.0643845f,
	90.829567f,
	90.9678421f,

	//clamp phi
	255.35321f,
	260.974487f,
	262.992676f,
	269.107361f,
	269.591278f,

	//v sensor gain, phi
	60314.3242f,
	161.143814f,

	//reserve calibration data is used flag
	0x00
};

static uint8_t clamp_sensor_gain_preset[CLAMP_SENSOR_MAX_OVERALL_GAIN + 1] = {
	0,
	0, 1, 2, 3, 4,
	4, 4, 4, 4
};

static gain_type_t clamp_sensor_adc_gain_preset[CLAMP_SENSOR_MAX_OVERALL_GAIN
+ 1] = {
	GAIN_1V3,
	GAIN_1, GAIN_1, GAIN_1, GAIN_1, GAIN_1,
	GAIN_2, GAIN_4, GAIN_8, GAIN_16
};

static uint8_t shunt_sensor_gain_preset[SHUNT_SENSOR_MAX_OVERALL_GAIN + 1] = {
	0,
	0, 1, 2, 3, 4,
	4, 4, 4, 4
};

static gain_type_t shunt_sensor_adc_gain_preset[SHUNT_SENSOR_MAX_OVERALL_GAIN
+ 1] = {
	GAIN_1V3,
	GAIN_1, GAIN_1, GAIN_1, GAIN_1, GAIN_1,
	GAIN_2, GAIN_4, GAIN_8, GAIN_16
};

static float32_t adc_gain_coeffs[6] = {
	(1.f) / (3.f), 1, 2, 4, 8, 16
};


void shunt_sensor_set_gain		(uint8_t gain);
void clamp_sensor_set_gain		(uint8_t gain);
void adc_increase_gain			(void);
void adc_decrease_gain			(void);
void measurement_start			(void);
void measurement_stop			(void);
void increase_gain				(void);
void decrease_gain				(void);
void switch_sensing_chanel	(sensor_type_t switch_to_sensor);
bool store_coeffs_to_flash_struct(void);
void recall_coeffs_from_flash_struct(void);

#ifdef __cplusplus
}
#endif
