/*
 * menu.c
 *
 * Created: 30.10.2021 15:18:43
 *  Author: malygosstationar
 */

#include "asf.h"
#include "menu.h"
#include "menu_calibration.h"
#include "MCP23016.h"
#include "LCD1608.h"
#include "system_init.h"
#include "keyboard.h"
#include "DSP_functions.h"
#include "signal_conditioning.h"

#ifdef __cplusplus
extern "C" {
#endif

Menu_t Menu;

void menu_display_value_page(bool reprint_all);
void menu_display_globalconfig_page(bool reprint_all);
void menu_display_functions_init(void);
extern char *gcvtf(float, int, char *);

void menu_set_inputbox_f(uint8_t column, uint8_t line, float32_t datamin,
                         float32_t datamax, float32_t *managed_value, uint8_t occupied_chars,
                         uint8_t digits)
{
	uint8_t idx;

	Menu.inputbox_line = line;
	Menu.inputbox_column = column;
	Menu.inputbox_val_ptr = managed_value;
	Menu.input_box_is_enabled = true;
	Menu.inputbox_occupied_chars = occupied_chars;
	Menu.inputbox_digits = digits;
	Menu.inputbox_valmax = datamax;
	Menu.inputbox_valmin = datamin;

	for (idx = 0; idx < occupied_chars; idx++)
		Menu.inputbox_cleaning_data[idx] = ' ';

	Menu.inputbox_cleaning_data[idx + 1] = '\0';

}

void menu_manage_inputbox_f(float32_t add_val)
{
	static char str[20];

	LCD_cursor_setpos(Menu.inputbox_column, Menu.inputbox_line);
	LCD_write(Menu.inputbox_cleaning_data);
	LCD_cursor_setpos(Menu.inputbox_column, Menu.inputbox_line);

	float32_t new_val = add_val + *Menu.inputbox_val_ptr;

	if (new_val > Menu.inputbox_valmax)
		new_val = Menu.inputbox_valmax;
	else if (new_val < Menu.inputbox_valmin)
		new_val = Menu.inputbox_valmin;

	*Menu.inputbox_val_ptr = new_val;

	gcvtf(*Menu.inputbox_val_ptr, Menu.inputbox_digits, str);
	LCD_write(str);
}

void menu_manager_clamp_pos_calibration(void)
{
	calibration_clamp_position(kbrddd);
}

void menu_manager_calibration(void)
{
	switch (kbrddd) {
	case KEY_ENCSW: {
		calibration_semiauto(CALIBRATOR_GO_NEXT_STEP);
	}
	break;

	case KEY_BACK: {
		calibration_semiauto(CALIBRATOR_GO_PREV_STEP);
	}
	break;

	case KEY_ENCL: {
		if (Calibrator.cal_phase != CALIBRATION_GET_VOUT_MEASURED_EXT)
			return;

		float32_t add_val;

		if (Keyboard.enc_superHSM)
			add_val = -1;
		else if (Keyboard.enc_fastmode)
			add_val = -0.1f;
		else
			add_val = -0.01;

		menu_manage_inputbox_f(add_val);
	}
	break;

	case KEY_ENCR: {
		if (Calibrator.cal_phase != CALIBRATION_GET_VOUT_MEASURED_EXT)
			return;

		float32_t add_val;

		if (Keyboard.enc_superHSM)
			add_val = 1;
		else if (Keyboard.enc_fastmode)
			add_val = 0.1f;
		else
			add_val = 0.01;

		menu_manage_inputbox_f(add_val);
	}
	break;
	}
}

void menu_manager_config(void)
{
	static uint16_t amplitude = 0;
	static bool cursor_enabled = false;

	switch (kbrddd) {
	case KEY_BACK: {
		if (cursor_enabled) {
			LCD_cursor_disable();
			cursor_enabled = false;

			LCD_cursor_setpos(11, 2);
			LCD_write("     ");
			LCD_cursor_setpos(11, 2);
			LCD_write_number(Analog.generator_amplitude);
		} else
			menu_LCD_change_display_page(MENU_MEASUREMENT);
	}
	break;

	case KEY_F3: {
		if (!cursor_enabled) {
			LCD_cursor_setpos(11, 2);
			LCD_cursor_enable();
			cursor_enabled = true;

			amplitude = Analog.generator_amplitude;
		}
	}
	break;

	case KEY_ENCL: {
		if (cursor_enabled) {
			if (amplitude > 0) {
				amplitude--;

				LCD_cursor_setpos(11, 2);
				LCD_write("     ");
				LCD_cursor_setpos(11, 2);
				LCD_write_number(amplitude);
			}
		}
	}
	break;

	case KEY_ENCR: {
		if (cursor_enabled) {
			if (amplitude < DACC_MAX_AMPLITUDE) {
				amplitude++;

				LCD_cursor_setpos(11, 2);
				LCD_write("     ");
				LCD_cursor_setpos(11, 2);
				LCD_write_number(amplitude);
			}
		}
	}
	break;

	case KEY_ENCSW: {
		if (cursor_enabled) {
			Analog.generator_amplitude = amplitude;
			LCD_cursor_disable();
			cursor_enabled = false;

			LCD_cursor_setpos(11, 2);
			LCD_write("     ");
			LCD_cursor_setpos(11, 2);
			LCD_write_number(Analog.generator_amplitude);
		}
	}
	break;

	case KEY_LEFT: {
		measurement_start();
	}
	break;

	case KEY_RIGHT: {
		measurement_stop();
	}
	}
}

void menu_manager_main(void)
{
	switch (kbrddd) {
	case KEY_MENU: {
		menu_LCD_change_display_page(MENU_CALIBRATION);
		calibration_semiauto(CALIBRATOR_GO_NEXT_STEP);

		if (Analog.generator_is_active)
			measurement_stop();
	}
	break;

	case KEY_F1:
		measurement_start();
		break;

	case KEY_F2:
		measurement_stop();
		break;

	case KEY_F3: {
		measurement_stop();
		menu_LCD_change_display_page(MENU_CLAMP_CALIBRATION);
		calibration_clamp_position(0);
	}
	break;

	case KEY_RIGHT: {

	}
	break;

	case KEY_ENCSW: {
		if (Analog.generator_is_active) {
			switch (Analog.selected_sensor) {
			case VOLTAGE_SENSOR:
				switch_sensing_chanel(SHUNT_SENSOR);
				break;

			case SHUNT_SENSOR:
				switch_sensing_chanel(CLAMP_SENSOR);
				buzzer_enable();
				break;

			case CLAMP_SENSOR:
				switch_sensing_chanel(VOLTAGE_SENSOR);
				buzzer_disable();
				break;
			}
		}
	}
	break;

	case KEY_ENCL: {
		/*if (clamp_measurements_result.pos_ch == REF_CH0)
			return;
		else if (clamp_measurements_result.pos_ch == REF_CH2) {
			MCP3462_set_mux(REF_CH0, REF_CH1);
			clamp_measurements_result.pos_ch = REF_CH0;
			clamp_measurements_result.neg_ch = REF_CH1;
		} else if (clamp_measurements_result.pos_ch == REF_CH4) {
			MCP3462_set_mux(REF_CH2, REF_CH3);
			clamp_measurements_result.pos_ch = REF_CH2;
			clamp_measurements_result.neg_ch = REF_CH3;
		}*/
		Analog.AGC_on = false;
		decrease_gain();
	}
	break;

	case KEY_ENCR: {
		/*if (clamp_measurements_result.pos_ch == REF_CH4)
			return;
		else if (clamp_measurements_result.pos_ch == REF_CH2) {
			MCP3462_set_mux(REF_CH4, REF_CH5);
			clamp_measurements_result.pos_ch = REF_CH4;
			clamp_measurements_result.neg_ch = REF_CH5;
		} else if (clamp_measurements_result.pos_ch == REF_CH0) {
			MCP3462_set_mux(REF_CH2, REF_CH3);
			clamp_measurements_result.pos_ch = REF_CH2;
			clamp_measurements_result.neg_ch = REF_CH3;
		}*/
		Analog.AGC_on = false;
		increase_gain();
	}
	break;
	}
}

void manage_keyboard(void)
{
	if (kbrddd) {
		switch (Menu.current_page) {
		case MENU_MEASUREMENT: {
			menu_manager_main();
		}
		break;

		case MENU_CONFIG: {
			menu_manager_config();
		}
		break;

		case MENU_CALIBRATION: {
			menu_manager_calibration();
		}
		break;

		case MENU_CLAMP_CALIBRATION: {
			menu_manager_clamp_pos_calibration();
		}
		break;
		}
		
		kbrddd = 0;

		if (!flash_test_runing);

		menu_refresh_page();
	}
}

void menu_display_value_page(bool reprint_all)
{
	if (reprint_all) {
		LCD_clear();

		LCD_cursor_setpos(1, 1);
		LCD_write("Sensor:");

		LCD_cursor_setpos(1, 2);
		LCD_write("I:");

		LCD_cursor_setpos(1, 3);
		LCD_write("Q:");

		LCD_cursor_setpos(1, 4);
		LCD_write("Phi:");
	}

	LCD_cursor_setpos(8, 1);

	if (Analog.selected_sensor == CLAMP_SENSOR)
		LCD_write("CLAMP");
	else if (Analog.selected_sensor == SHUNT_SENSOR)
		LCD_write("SHUNT");
	else
		LCD_write("VOUT ");

	LCD_cursor_setpos(14, 1);

	if (Analog.generator_is_active)
		LCD_write("!ACTV!");
	else
		LCD_write("NOACTV");

	LCD_cursor_setpos(3, 3);
	LCD_write("              ");
	LCD_cursor_setpos(5, 4);
	LCD_write("           ");
	LCD_cursor_setpos(3, 2);
	LCD_write("           ");
	LCD_cursor_setpos(3, 2);


	if (Analog.selected_sensor == CLAMP_SENSOR) {
		LCD_write_float2(clamp_measurements_result.R_clamp, 8);
		LCD_cursor_setpos(3, 3);
		LCD_write_float2(clamp_measurements_result.X_clamp, 8);
		LCD_cursor_setpos(5, 4);
		LCD_write_float2(clamp_measurements_result.Z_clamp_phi, 8);
	} else if (Analog.selected_sensor == SHUNT_SENSOR) {
		LCD_write_float2(clamp_measurements_result.R_ovrl, 8);
		LCD_cursor_setpos(3, 3);
		LCD_write_float2(clamp_measurements_result.X_ovrl, 8);
		LCD_cursor_setpos(5, 4);
		LCD_write_float2(clamp_measurements_result.Z_ovrl_phi, 8);
	} else {
		LCD_write_float2(clamp_measurements_result.V_ovrl_I, 8);
		LCD_cursor_setpos(3, 3);
		LCD_write_float2(clamp_measurements_result.V_ovrl_Q, 8);
		LCD_cursor_setpos(5, 4);
		LCD_write_float2(clamp_measurements_result.V_ovrl_phi, 8);
	}
}

void menu_display_globalconfig_page(bool reprint_all)
{
	if (reprint_all) {
		LCD_clear();
		LCD_cursor_setpos(1, 1);
		LCD_write("GlobalConfig:");

		LCD_cursor_setpos(1, 2);
		LCD_write("Mode:");

		LCD_cursor_setpos(1, 3);
		LCD_write("Amplitude:");
	}

	LCD_cursor_setpos(11, 2);
	LCD_write("       ");
	LCD_cursor_setpos(11, 2);

	if (Analog.mes_mode == MEASUREMENT_MODE_MANUAL)
		LCD_write("Manual");
	else if (Analog.mes_mode == MEASUREMENT_MODE_1)
		LCD_write("nodef");


	LCD_cursor_setpos(11, 3);
	LCD_write("    ");
	LCD_cursor_setpos(11, 3);
	LCD_write_number(Analog.generator_amplitude);
	LCD_write("V");


}
void menu_refresh_page(void)
{
	if (Clamp_calibrator.is_calibrating)
		calibration_clamp_pos_display_data();
	else {
		if (Menu.current_page > MENU_PAGES) {
			Menu.current_page = 0;
			Menu.alarm = true;
		}

		(*Menu.display_menu[Menu.current_page])(false);
	}
}

void menu_LCD_change_display_page(menu_page_t page)
{
	LCD_clear();

	if (page > MENU_PAGES) {
		page = 0;
		Menu.alarm = true;
	}

	Menu.current_page = page;
	(*Menu.display_menu[Menu.current_page])(true);
}

void menu_display_functions_init(void)
{
	Menu.display_menu[MENU_MEASUREMENT] = menu_display_value_page;
	Menu.display_menu[MENU_CONFIG] = menu_display_globalconfig_page;
	Menu.display_menu[MENU_CALIBRATION] = menu_manager_calibration;
	Menu.display_menu[MENU_CLAMP_CALIBRATION] =
	  calibration_clamp_pos_display_data;
}

void menu_init(void)
{
	menu_display_functions_init();
}

#ifdef __cplusplus
}
#endif