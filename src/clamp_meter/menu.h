/*
 * menu.h
 *
 * Created: 30.10.2021 15:18:59
 *  Author: malygosstationar
 */


#ifndef MENU_H_
#define MENU_H_

#include "arm_math.h"

#define MENU_PAGES	4

#define strgze(str)	#str
#define conv_to_str(str) strgze(str)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MENU_MEASUREMENT,
	MENU_CONFIG,
	MENU_CALIBRATION,
	MENU_CLAMP_CALIBRATION
} menu_page_t;

typedef void display_function_t(bool);

typedef struct {
	menu_page_t current_page;
	display_function_t *display_menu[MENU_PAGES];

	bool input_box_is_enabled;
	uint8_t inputbox_line;
	uint8_t inputbox_column;
	float32_t *inputbox_val_ptr;
	uint8_t inputbox_occupied_chars;
	uint8_t inputbox_digits;
	float32_t inputbox_valmax;
	float32_t inputbox_valmin;
	char inputbox_cleaning_data[21];

	bool alarm;
} Menu_t;

extern Menu_t Menu;

void menu_set_inputbox_f	(uint8_t column,
                             uint8_t line,
                             float32_t datamin,
                             float32_t datamax,
                             float32_t *managed_value,
                             uint8_t occupied_chars,
                             uint8_t digits);
void menu_manage_inputbox_f(float32_t add_val);
void manage_keyboard(void);
void menu_refresh_page(void);
void menu_LCD_change_display_page(menu_page_t page);
void menu_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MENU_H_ */