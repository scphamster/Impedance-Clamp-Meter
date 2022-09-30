/*
 * menu_calibration.c
 *
 * Created: 05.11.2021 18:10:46
 *  Author: malygosstationar
 */

#include "asf.h"
#include "signal_conditioning.h"
#include "MCP3462.h"
#include "DSP_functions.h"
#include "system_init.h"
#include "system.h"
#include "LCD1608.h"
#include "menu.h"
#include "menu_calibration.h"
#include "keyboard.h"

#ifdef __cplusplus
extern "C" {
#endif

Clamp_calibrator_t Clamp_calibrator;

void calibration_terminate(void);
void calibration_start(void);
void calibration_pause(void);
void calibration_reset_variables(void);
void calibration_save(void);

float32_t calibration_clamp_pos_refsel(void)
{
	if (Dsp.Z_clamp < CALIBRATION_CLAMP_POS_Z0) {
		return CALIBRATION_CLAMP_POS_Z0;
	} else if (Dsp.Z_clamp < CALIBRATION_CLAMP_POS_Z1) {
		return CALIBRATION_CLAMP_POS_Z0;
	} else if (Dsp.Z_clamp < CALIBRATION_CLAMP_POS_Z2) {
		return CALIBRATION_CLAMP_POS_Z1;
	} else if (Dsp.Z_clamp < CALIBRATION_CLAMP_POS_Z3) {
		return CALIBRATION_CLAMP_POS_Z2;
	} else {
		return CALIBRATION_CLAMP_POS_Z3;
	}
}

void calibration_clamp_pos_display_data(void)
{
	LCD_cursor_setpos(7, 2);
	LCD_write("              ");
	LCD_cursor_setpos(7, 2);

	switch (Clamp_calibrator.phase) {
	case CLAMP_CAL_VOUT:
		LCD_write_float2(Dsp.V_ovrl, 5);
		break;

	case CLAMP_CAL_SHUNT:
		LCD_write_float2(Dsp.V_shunt, 5);
		break;

	case CLAMP_CAL_CLAMP:
		LCD_write_float2(Dsp.I_clamp, 5);
		break;
	}
}

void calibration_clamp_pos_terminate(void)
{
	Clamp_calibrator.is_calibrated = true;
	Clamp_calibrator.is_calibrating = false;
	Clamp_calibrator.phase = CLAMP_CAL_TERMINATION;

	menu_LCD_change_display_page(MENU_MEASUREMENT);
}

void calibration_clamp_pos_calculate(void)
{
	measurement_stop();
	switch_sensing_chanel(VOLTAGE_SENSOR);
	
	Clamp_calibrator.I_clamp_delta = Dsp.I_clamp *
	                                 Clamp_calibrator.normalize_coeff - Clamp_calibrator.I_clamp_orig;
	Clamp_calibrator.position_gain = Clamp_calibrator.I_clamp_delta_calc /
	                                 Clamp_calibrator.I_clamp_delta;

	LCD_cursor_setpos(1, 1);
	LCD_write("Clamp position cal  ");
	LCD_cursor_setpos(1, 2);
	LCD_write("is complete.        ");
	LCD_cursor_setpos(1, 3);
	LCD_write("PosGain is          ");
	LCD_cursor_setpos(1, 4);
	LCD_write("Press ENC to return ");

	LCD_cursor_setpos(11, 3);
	LCD_write_float2(Clamp_calibrator.position_gain, 6);

	calibration_clamp_pos_terminate();
}

void calibration_clamp_pos_clamp(calibrator_action_type_t action)
{
	if (action == CALIBRATOR_GO_NEXT_STEP) {
		Clamp_calibrator.normalize_coeff = Clamp_calibrator.V_applied_1 
											/ Dsp.V_applied;

		switch_sensing_chanel(CLAMP_SENSOR);

		LCD_cursor_setpos(1, 2);
		LCD_write("CLAMP:              ");
		LCD_cursor_setpos(1, 3);
		LCD_write("PUSH ENC if CLAMP is");
		LCD_cursor_setpos(1, 4);
		LCD_write("stable..............");

		Clamp_calibrator.phase = CLAMP_CAL_CLAMP;
	} else {

	}
}

void calibration_clamp_pos_shunt(calibrator_action_type_t action)
{
	if (action == CALIBRATOR_GO_NEXT_STEP) {
		switch_sensing_chanel(SHUNT_SENSOR);

		LCD_cursor_setpos(1, 2);
		LCD_write("SHUNT:              ");
		LCD_cursor_setpos(1, 3);
		LCD_write("PUSH ENC if SHUNT is");
		LCD_cursor_setpos(1, 4);
		LCD_write("stable..............");
		
		Clamp_calibrator.phase = CLAMP_CAL_SHUNT;
	} else {

	}
}

void calibration_clamp_pos_vout(calibrator_action_type_t action)
{
	if (action == CALIBRATOR_GO_NEXT_STEP) {
		LCD_cursor_setpos(1, 1);
		LCD_write("calibrating clamp...");
		LCD_cursor_setpos(1, 2);
		LCD_write("VOUT:               ");
		LCD_cursor_setpos(1, 3);
		LCD_write("PUSH ENC if VOUT is ");
		LCD_cursor_setpos(1, 4);
		LCD_write("stable..............");

		switch_sensing_chanel(VOLTAGE_SENSOR);
		measurement_start();

		Clamp_calibrator.phase = CLAMP_CAL_VOUT;
	}
}

void calibration_clamp_pos_connect_ref(calibrator_action_type_t action)
{
	if (action == CALIBRATOR_GO_NEXT_STEP) {
		Clamp_calibrator.is_calibrating = true;
		Clamp_calibrator.reference_resistance = calibration_clamp_pos_refsel();
		Clamp_calibrator.I_clamp_orig = Dsp.I_clamp;
		Clamp_calibrator.V_applied_1 = Dsp.V_applied;
		Clamp_calibrator.I_clamp_delta_calc = Clamp_calibrator.V_applied_1 /
		                                      Clamp_calibrator.reference_resistance;

		LCD_cursor_setpos(1, 1);
		LCD_write("                    ");
		LCD_cursor_setpos(1, 1);
		LCD_write("Connect " conv_to_str(Clamp_calibrator.reference_resistance)
		          "Ohm");
		LCD_cursor_setpos(1, 2);
		LCD_write("Reference Resistance");
		LCD_cursor_setpos(1, 3);
		LCD_write("PUSH ENC to proceed ");
		LCD_cursor_setpos(1, 4);
		LCD_write("                    ");

		Clamp_calibrator.phase = CLAMP_CAL_CONNECT_REFERENSE;
	} else {

	}
}

void calibration_clamp_pos_hellopage(void)
{
	LCD_cursor_setpos(1, 1);
	LCD_write("Calibrate           ");
	LCD_cursor_setpos(1, 2);
	LCD_write("Clamp Position Gain?");
	LCD_cursor_setpos(1, 3);
	LCD_write("         PUSH:      ");
	LCD_cursor_setpos(1, 4);
	LCD_write("   L = NO / R = YES ");

	Clamp_calibrator.phase = CLAMP_CAL_HELLOPAGE;
}

void calibration_clamp_position(uint16_t key)
{
	calibrator_action_type_t action;
	
	if (Clamp_calibrator.phase == CLAMP_CAL_TERMINATION)
		Clamp_calibrator.phase = CLAMP_CAL_HELLOPAGE;
	
	if (Clamp_calibrator.phase == CLAMP_CAL_HELLOPAGE) {
		if (key == KEY_LEFT)
			action = CALIBRATOR_GO_PREV_STEP;
		else if (key == KEY_RIGHT)
			action = CALIBRATOR_GO_NEXT_STEP;
	} else {
		if (key == KEY_ENCSW) 
			action = CALIBRATOR_GO_NEXT_STEP;
		else
			action = CALIBRATOR_GO_PREV_STEP;
	}
	
	switch (Clamp_calibrator.phase) {
	case CLAMP_CAL_START:
		calibration_clamp_pos_hellopage();
		break;

	case CLAMP_CAL_HELLOPAGE:
		calibration_clamp_pos_connect_ref(action);
		break;
		
	case CLAMP_CAL_CONNECT_REFERENSE:
		calibration_clamp_pos_vout(action);
		break;

	case CLAMP_CAL_VOUT:
		calibration_clamp_pos_shunt(action);
		break;

	case CLAMP_CAL_SHUNT:
		calibration_clamp_pos_clamp(action);
		break;

	case CLAMP_CAL_CLAMP:
		calibration_clamp_pos_calculate();
		break;
	}
}




void calibration_terminate(void)
{
	Calibrator.cal_phase = CALIBRATION_HELLO_PAGE;
	Calibrator.cal_phase_num = 0;
	Calibrator.new_data_is_ready = false;
	Calibrator.is_calibrating = false;
	menu_LCD_change_display_page(MENU_MEASUREMENT);
}

void calibration_start(void)
{
	Calibrator.is_calibrating = true;

	switch (Calibrator.cal_phase) {
	case CALIBRATION_VOUT: {
		switch_sensing_chanel(VOLTAGE_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(0);
		shunt_sensor_set_gain(0);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_MEASURE_VOUT_G0:
	case CALIBRATION_MEASURE_VOUT_G1:
	case CALIBRATION_MEASURE_VOUT_G2:
	case CALIBRATION_MEASURE_VOUT_G3:
	case CALIBRATION_MEASURE_VOUT_G4: {
		switch_sensing_chanel(VOLTAGE_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(0);
		shunt_sensor_set_gain(0);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_SHUNT_G0: {
		switch_sensing_chanel(SHUNT_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(0);
		shunt_sensor_set_gain(0);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_CLAMP_G0: {
		switch_sensing_chanel(CLAMP_SENSOR);
		MCP3462_set_gain(GAIN_1V3);
		clamp_sensor_set_gain(0);
		shunt_sensor_set_gain(0);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_SHUNT_G1: {
		switch_sensing_chanel(SHUNT_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(1);
		shunt_sensor_set_gain(1);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_CLAMP_G1: {
		switch_sensing_chanel(CLAMP_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(1);
		shunt_sensor_set_gain(1);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_SHUNT_G2: {
		switch_sensing_chanel(SHUNT_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(2);
		shunt_sensor_set_gain(2);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_CLAMP_G2: {
		switch_sensing_chanel(CLAMP_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(2);
		shunt_sensor_set_gain(2);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_SHUNT_G3: {
		switch_sensing_chanel(SHUNT_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(3);
		shunt_sensor_set_gain(3);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_CLAMP_G3: {
		switch_sensing_chanel(CLAMP_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(3);
		shunt_sensor_set_gain(3);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_SHUNT_G4: {
		switch_sensing_chanel(SHUNT_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(4);
		shunt_sensor_set_gain(4);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;

	case CALIBRATION_CLAMP_G4: {
		switch_sensing_chanel(CLAMP_SENSOR);
		MCP3462_set_gain(GAIN_1);
		clamp_sensor_set_gain(4);
		shunt_sensor_set_gain(4);
		Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
		measurement_start();
	}
	break;



	default:
		Calibrator.error_occured = true;
	}

}

void calibration_pause(void)
{
	Calibrator.is_calibrating = false;
	measurement_stop();
}

void calibration_reset_variables(void)
{
	Calibrator.new_data_is_ready = false;

	Calibrator.externally_measured_vout = CALIBRATION_EXTERNAL_VOLTAGE_STARTVAL;
}

void calibration_display_measured_data(void)
{
	float32_t data;

	LCD_cursor_setpos(7, 2);
	LCD_write("         ");
	LCD_cursor_setpos(7, 2);

	if ((Calibrator.cal_phase == CALIBRATION_MEASURE_VOUT_G0) ||
	    (Calibrator.cal_phase == CALIBRATION_MEASURE_VOUT_G1) ||
	    (Calibrator.cal_phase == CALIBRATION_MEASURE_VOUT_G2) ||
	    (Calibrator.cal_phase == CALIBRATION_MEASURE_VOUT_G3) ||
	    (Calibrator.cal_phase == CALIBRATION_MEASURE_VOUT_G4))
		data = Calibrator.sensor_mag / Calibrator.v_sens_gain;
	else
		data = Calibrator.sensor_mag;

	LCD_write_float2(data, 8);

	LCD_cursor_setpos(5, 3);
	LCD_write("                ");
	LCD_cursor_setpos(5, 3);
	LCD_write_float2(Calibrator.sensor_phi, 8);
}

void calibration_save(void)
{
	uint32_t idx;

	for (idx = 0; idx < CLAMP_SENSOR_GAIN_COEFFS_NUM; idx++) {
		Cal_data.clamp_gain[idx] = Calibrator.clamp_gains[idx];
		Cal_data.clamp_phi[idx] = Calibrator.clamp_angles[idx];
	}

	for (idx = 0; idx < SHUNT_SENSOR_GAIN_COEFFS_NUM; idx++) {
		Cal_data.shunt_gain[idx] = Calibrator.shunt_gains[idx];
		Cal_data.shunt_phi[idx] = Calibrator.shunt_angles[idx];
	}

	Cal_data.v_sens_gain = Calibrator.v_sens_gain;
	Cal_data.v_sens_phi = Calibrator.vout_phi_noload;

	Cal_data.data_password = CALIBRATION_DATA_NOT_ERASED_MARKER;

	store_coeffs_to_flash_struct();
	calibration_terminate();
}


void calibration_write_phase_header(uint8_t phase_num)
{
	LCD_cursor_setpos(1, 1);

	LCD_write("calibrating ");
	LCD_write_number(phase_num);
	LCD_write("/5.....");
}


void calibration_semiauto_clamp_gX(calibrator_action_type_t action)
{
	calibration_pause();

	if (action == CALIBRATOR_GO_NEXT_STEP) {
		float32_t current;

		if (Calibrator.cal_phase_num == 1) {
			Calibrator.clamp_gains[Calibrator.cal_phase_num - 1] = Calibrator.sensor_mag
			    * 3 / (Calibrator.v_sh / R_SHUNT);
		} else {
			Calibrator.clamp_gains[Calibrator.cal_phase_num - 1] = Calibrator.sensor_mag
			    / (Calibrator.v_sh / R_SHUNT);
		}

		Calibrator.clamp_angles[Calibrator.cal_phase_num - 1] = Calibrator.sensor_phi
		    - Calibrator.vout_phi;

		LCD_cursor_setpos(1, 1);
		LCD_write("Calibration ");
		LCD_write_number(Calibrator.cal_phase_num);
		LCD_write("/5 DONE");

		Calibrator.cal_phase_num++;

		LCD_cursor_setpos(1, 2);

		if (Calibrator.cal_phase != CALIBRATION_CLAMP_G4)
			LCD_write("Connect             ");

		LCD_cursor_setpos(1, 3);
		LCD_write("Push ENC when ready ");

		LCD_cursor_setpos(1, 4);
		LCD_write("                    ");

		LCD_cursor_setpos(9, 2);

		switch (Calibrator.cal_phase) {
		case CALIBRATION_CLAMP_G0:
			LCD_write(conv_to_str(CALIBRATION_G1_R)" Ohm");
			break;

		case CALIBRATION_CLAMP_G1:
			LCD_write(conv_to_str(CALIBRATION_G2_R)" Ohm");
			break;

		case CALIBRATION_CLAMP_G2:
			LCD_write(conv_to_str(CALIBRATION_G3_R)" Ohm");
			break;

		case CALIBRATION_CLAMP_G3:
			LCD_write(conv_to_str(CALIBRATION_G4_R)" Ohm");
			break;

		case CALIBRATION_CLAMP_G4:
			LCD_cursor_setpos(1, 2);
			LCD_write("Push ENC to confirm ");

			LCD_cursor_setpos(1, 3);
			LCD_write("                    ");

			LCD_cursor_setpos(1, 4);
			LCD_write("                    ");
			break;
		}

		Calibrator.cal_phase++;
	} else
		calibration_terminate();
}

void calibration_semiauto_shunt_gX(calibrator_action_type_t action)
{
	calibration_pause();

	if (action == CALIBRATOR_GO_NEXT_STEP) {
		float32_t current;

		calibration_write_phase_header(Calibrator.cal_phase_num);

		LCD_cursor_setpos(1, 2);
		LCD_write("CLAMP:shready       ");

		LCD_cursor_setpos(1, 3);
		LCD_write("Phi:                ");

		LCD_cursor_setpos(1, 4);
		LCD_write("Push ENC if stable  ");

		Calibrator.shunt_gains[Calibrator.cal_phase_num - 1] =
		  Calibrator.sensor_mag / Calibrator.v_sh;

		Calibrator.shunt_angles[Calibrator.cal_phase_num - 1] =
		  Calibrator.sensor_phi - Calibrator.vout_phi;

		Calibrator.cal_phase++;

		calibration_start();
	} else
		calibration_terminate();
}

void calibration_semiauto_measure_vout_gX(calibrator_action_type_t action)
{
	calibration_pause();

	if (action == CALIBRATOR_GO_NEXT_STEP) {
		float32_t test_resistance;

		calibration_write_phase_header(Calibrator.cal_phase_num);

		LCD_cursor_setpos(1, 2);
		LCD_write("SHUNT:voutready     ");

		LCD_cursor_setpos(1, 3);
		LCD_write("Phi:                ");

		LCD_cursor_setpos(1, 4);
		LCD_write("Push ENC if stable  ");

		Calibrator.vout_ovrl = Calibrator.sensor_mag /
		                       Calibrator.v_sens_gain;

		Calibrator.vout_phi = Calibrator.sensor_phi - Calibrator.vout_phi_noload;

		switch (Calibrator.cal_phase_num) {
		case 1:
			test_resistance = CALIBRATION_G0_R;
			break;

		case 2:
			test_resistance = CALIBRATION_G1_R;
			break;

		case 3:
			test_resistance = CALIBRATION_G2_R;
			break;

		case 4:
			test_resistance = CALIBRATION_G3_R;
			break;

		case 5:
			test_resistance = CALIBRATION_G4_R;
			break;
		}

		Calibrator.v_sh = Calibrator.vout_ovrl * R_SHUNT / (test_resistance +
		                  R_SHUNT);

		Calibrator.cal_phase++;
		calibration_start();
	} else
		calibration_terminate();
}

void calibration_semiauto_set_gX(calibrator_action_type_t action)
{
	if (action == CALIBRATOR_GO_NEXT_STEP) {
		calibration_write_phase_header(Calibrator.cal_phase_num);

		LCD_cursor_setpos(1, 2);
		LCD_write("Vout:               ");

		LCD_cursor_setpos(1, 3);
		LCD_write("Phi:                ");

		LCD_cursor_setpos(1, 4);
		LCD_write("Push ENC if stable  ");

		Calibrator.cal_phase++;

		calibration_start();
	} else
		calibration_terminate();
}


void calibration_semiauto_warning_g0(calibrator_action_type_t action)
{
	if (action == CALIBRATOR_GO_NEXT_STEP) {
		LCD_cursor_setpos(1, 1);
		LCD_write("Connect             ");
		LCD_cursor_setpos(9, 1);
		LCD_write(conv_to_str(CALIBRATION_G0_R)" Ohm");

		LCD_cursor_setpos(1, 2);
		LCD_write("Do not change CLAMP ");

		LCD_cursor_setpos(1, 3);
		LCD_write("position after start");

		LCD_cursor_setpos(1, 4);
		LCD_write("Push ENC to start...");

		Calibrator.cal_phase = CALIBRATION_SET_G0R;
		Calibrator.cal_phase_num = 1;
	} else
		calibration_terminate();
}

void calibration_semiauto_helper_input_vout_mesd(calibrator_action_type_t
    action)
{
	LCD_cursor_disable();

	if (action == CALIBRATOR_GO_NEXT_STEP) {
		LCD_cursor_setpos(1, 1);
		LCD_write("Press ENC to proceed");

		LCD_cursor_setpos(1, 2);
		LCD_write("Press BACK to repeat");

		LCD_cursor_setpos(1, 3);
		LCD_write("measured output     ");

		LCD_cursor_setpos(1, 4);
		LCD_write("voltage:            ");

		Calibrator.v_sens_gain = Calibrator.sensor_mag /
		                         Calibrator.externally_measured_vout;

		Calibrator.vout_phi_noload = Calibrator.sensor_phi;

		LCD_cursor_setpos(9, 4);
		LCD_write_float2(Calibrator.sensor_mag / Calibrator.v_sens_gain, 4);

		Calibrator.cal_phase = CALIBRATION_WARNING_G0;
	} else
		calibration_terminate();

}

void calibration_semiauto_helper_vout(calibrator_action_type_t action)
{
	calibration_pause();

	if (action == CALIBRATOR_GO_NEXT_STEP) {
		LCD_cursor_setpos(1, 1);
		LCD_write("Turn Encoder to set ");

		LCD_cursor_setpos(1, 2);
		LCD_write("externally mes'd    ");

		LCD_cursor_setpos(1, 3);
		LCD_write("output voltage..... ");

		LCD_cursor_setpos(1, 4);
		LCD_write("voltage:");
		LCD_write_float2(Calibrator.externally_measured_vout, 4);
		LCD_write("       ");
		LCD_cursor_setpos(14, 4);
		LCD_cursor_enable();

		menu_set_inputbox_f(9, 4, 1, 70, &Calibrator.externally_measured_vout, 7, 4);

		Calibrator.cal_phase = CALIBRATION_GET_VOUT_MEASURED_EXT;
	} else
		calibration_terminate();
}

void calibration_semiauto_helper_vout_warning_shown(calibrator_action_type_t
    action)
{
	if (action == CALIBRATOR_GO_NEXT_STEP) {
		LCD_cursor_setpos(1, 1);
		LCD_write("Calibrating VOUT... ");

		LCD_cursor_setpos(1, 2);
		LCD_write("Data:               ");

		LCD_cursor_setpos(1, 3);
		LCD_write("Phi:                ");

		LCD_cursor_setpos(1, 4);
		LCD_write("Push ENC if stable  ");

		Calibrator.cal_phase = CALIBRATION_VOUT;
		calibration_start();
	} else
		calibration_terminate();
}

void calibration_semiauto_helper_start(calibrator_action_type_t action)
{
	if (action == CALIBRATOR_GO_NEXT_STEP) {
		LCD_cursor_setpos(1, 1);
		LCD_write("Discon. TNC output  ");

		LCD_cursor_setpos(1, 2);
		LCD_write("Carefully measure:  ");

		LCD_cursor_setpos(1, 3);
		LCD_write("Center pin<->Shield.");

		LCD_cursor_setpos(1, 4);
		LCD_write("ENC to enable output");

		Calibrator.cal_phase = CALIBRATION_VOUT_MSG_SHOWN;
	} else
		calibration_terminate();
}

void calibration_semiauto_helper_hellopage(void)
{
	calibration_reset_variables();

	LCD_cursor_setpos(1, 1);
	LCD_write("Semiautomatic       ");

	LCD_cursor_setpos(1, 2);
	LCD_write("calibration         ");

	LCD_cursor_setpos(1, 3);
	LCD_write("procedure           ");

	LCD_cursor_setpos(1, 4);
	LCD_write("Push ENC to start   ");

	Calibrator.cal_phase = CALIBRATION_START;
}

void calibration_semiauto(calibrator_action_type_t action)
{
	switch (Calibrator.cal_phase) {
	case CALIBRATION_HELLO_PAGE:
		calibration_semiauto_helper_hellopage();
		break;

	case CALIBRATION_START:
		calibration_semiauto_helper_start(action);
		break;

	case CALIBRATION_VOUT_MSG_SHOWN:
		calibration_semiauto_helper_vout_warning_shown(action);
		break;

	case CALIBRATION_VOUT:
		calibration_semiauto_helper_vout(action);
		break;

	case CALIBRATION_GET_VOUT_MEASURED_EXT:
		calibration_semiauto_helper_input_vout_mesd(action);
		break;

	case CALIBRATION_WARNING_G0:
		calibration_semiauto_warning_g0(action);
		break;

	case CALIBRATION_SET_G0R:
	case CALIBRATION_SET_G1R:
	case CALIBRATION_SET_G2R:
	case CALIBRATION_SET_G3R:
	case CALIBRATION_SET_G4R:
		calibration_semiauto_set_gX(action);
		break;

	case CALIBRATION_MEASURE_VOUT_G0:
	case CALIBRATION_MEASURE_VOUT_G1:
	case CALIBRATION_MEASURE_VOUT_G2:
	case CALIBRATION_MEASURE_VOUT_G3:
	case CALIBRATION_MEASURE_VOUT_G4:
		calibration_semiauto_measure_vout_gX(action);
		break;

	case CALIBRATION_SHUNT_G0:
	case CALIBRATION_SHUNT_G1:
	case CALIBRATION_SHUNT_G2:
	case CALIBRATION_SHUNT_G3:
	case CALIBRATION_SHUNT_G4:
		calibration_semiauto_shunt_gX(action);
		break;

	case CALIBRATION_CLAMP_G0:
	case CALIBRATION_CLAMP_G1:
	case CALIBRATION_CLAMP_G2:
	case CALIBRATION_CLAMP_G3:
	case CALIBRATION_CLAMP_G4:
		calibration_semiauto_clamp_gX(action);
		break;

	case CALIBRATION_CONFIRM:
		calibration_save();
		break;
	}
}

#ifdef __cplusplus
}
#endif