/*
 * menu_ili9486_kbrd_mngr.c
 *
 * Created: 13.11.2021 17:54:07
 *  Author: malygosstationar
 */ 

#include "asf.h"
#include "menu_ili9486_kbrd_mngr.h"
#include "menu_ili9486.h"
#include "menu_types_ili9486.h"
#include "menu.h"
#include "keyboard.h"
#include "signal_conditioning.h"
#include "system_init.h"

#ifdef __cplusplus
extern "C" {
#endif

//km stands for keyboard manager
void km_page_measurement(void);

void
km_page_measurement(void)
{
	switch(Keyboard.keys) {
	case KEY_MENU: {
		menu_LCD_change_display_page(MENU_CALIBRATION);
		calibration_semiauto(CALIBRATOR_GO_NEXT_STEP);

		if (Analog.generator_is_active)
			measurement_stop();
			
		display_print_page();
	}
	break;

	case KEY_F1:
		if (!Analog.generator_is_active)
			measurement_start();
			
		MMMenu.if_reprint_all = true;
		MMMenu.reprint = REPRINT_ALL;
		display_show_top_bar();
		
		const static char bot_bar_msg [] = "Push Encoder if data stable";
		MMMenu.bot_header_msg = bot_bar_msg;
		MMMenu.if_reprint_all = true;
		display_show_bot_bar();
		break;
		
		
	case KEY_F2:
		measurement_stop();
		
		MMMenu.if_reprint_all = true;
		MMMenu.reprint = REPRINT_ALL;
		
		display_show_top_bar();
		break;
		
	case KEY_F3: {
		if (Analog.generator_is_active)
			measurement_stop();
		menu_LCD_change_display_page(MENU_CLAMP_CALIBRATION);
		calibration_clamp_position(0);
		
		display_print_page();
	}
	break;

	case KEY_RIGHT: {
		display_print_page();
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
		
		display_print_page();
	}
	break;

	case KEY_ENCL: {
		Analog.AGC_on = false;
		decrease_gain();
		display_print_page();
	}
	break;

	case KEY_ENCR: {
		Analog.AGC_on = false;
		increase_gain();
		display_print_page();
	}
	break;
	
	}
}

void
kbrd_manager(void)
{
	if (!Keyboard.keys)
		return;
	
	switch (MMMenu.current_menu) {
		case MENU_MEASURE:
		km_page_measurement();
		break;
		
		default: break;
	}
	
	kbrddd = 0;
	Keyboard.keys = 0;
}

#ifdef __cplusplus
}
#endif