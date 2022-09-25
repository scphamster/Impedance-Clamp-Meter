/*
 * menu_calibration.h
 *
 * Created: 05.11.2021 18:11:02
 *  Author: malygosstationar
 */ 

#ifndef MENU_CALIBRATION_H_
#define MENU_CALIBRATION_H_

#define CALIBRATION_CLAMP_POS_Z0	10e3
#define CALIBRATION_CLAMP_POS_Z1	100e3
#define CALIBRATION_CLAMP_POS_Z2	470e3
#define CALIBRATION_CLAMP_POS_Z3	1e6

#define CALIBRATION_G0_R	6.73e3
#define CALIBRATION_G1_R	55.4e3
#define CALIBRATION_G2_R	55.4e3
#define CALIBRATION_G3_R	465e3
#define CALIBRATION_G4_R	465e3

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CALIBRATION_HELLO_PAGE,
	CALIBRATION_START,
	CALIBRATION_VOUT_MSG_SHOWN,
	CALIBRATION_VOUT,
	CALIBRATION_GET_VOUT_MEASURED_EXT,
	CALIBRATION_WARNING_G0,
	CALIBRATION_SET_G0R,
	CALIBRATION_MEASURE_VOUT_G0,
	CALIBRATION_SHUNT_G0,
	CALIBRATION_CLAMP_G0,
	CALIBRATION_SET_G1R,
	CALIBRATION_MEASURE_VOUT_G1,
	CALIBRATION_SHUNT_G1,
	CALIBRATION_CLAMP_G1,
	CALIBRATION_SET_G2R,
	CALIBRATION_MEASURE_VOUT_G2,
	CALIBRATION_SHUNT_G2,
	CALIBRATION_CLAMP_G2,
	CALIBRATION_SET_G3R,
	CALIBRATION_MEASURE_VOUT_G3,
	CALIBRATION_SHUNT_G3,
	CALIBRATION_CLAMP_G3,
	CALIBRATION_SET_G4R,
	CALIBRATION_MEASURE_VOUT_G4,
	CALIBRATION_SHUNT_G4,
	CALIBRATION_CLAMP_G4,
	CALIBRATION_CONFIRM
}calibration_phase_type_t;

typedef enum {
	CALIBRATOR_START,
	CALIBRATOR_GO_NEXT_STEP,
	CALIBRATOR_GO_PREV_STEP,
	CALIBRATOR_TERMINATE
}calibrator_action_type_t;

typedef enum {
	CLAMP_CAL_START,
	CLAMP_CAL_HELLOPAGE,
	CLAMP_CAL_CONNECT_REFERENSE,
	CLAMP_CAL_VOUT,
	CLAMP_CAL_SHUNT,
	CLAMP_CAL_CLAMP,
	CLAMP_CAL_TERMINATION
}clamp_calibrator_phase_type_t;

typedef struct {
	clamp_calibrator_phase_type_t phase;
	float32_t reference_resistance;
	float32_t I_clamp_orig;
	float32_t I_clamp_delta;
	float32_t I_clamp_delta_calc;
	float32_t V_applied_1;
	float32_t V_applied_2;
	float32_t normalize_coeff;
	float32_t position_gain;
	bool is_calibrated;
	bool is_calibrating;
}Clamp_calibrator_t;

extern Clamp_calibrator_t Clamp_calibrator;

static float32_t test_resistances[] = {
	6.8e3,
	55e3,
	55e3,
	470e3,
	470e3
};

void calibration_clamp_pos_display_data(void);
void calibration_clamp_position(uint16_t key);
void calibration_semiauto(calibrator_action_type_t action);
void calibration_display_measured_data	(void);

#ifdef __cplusplus
}
#endif

#endif /* MENU_CALIBRATION_H_ */
