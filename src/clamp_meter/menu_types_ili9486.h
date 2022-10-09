/*
* menu_types_ili9486.h
*
* Created: 13.11.2021 17:49:14
*  Author: malygosstationar
*/


#ifndef MENU_TYPES_ILI9486_H_
#define MENU_TYPES_ILI9486_H_

#include "ILI9486_public.h"
#include "ILI9486_private.h"

#define SUBSTRACT			0
#define ADD					1

/*Encoder HighSpeedMode delay values*/
#define M_ENC_HSM_ACVTON_TIMDEL	420
#define M_ENC_HSM_ACVTON_VALDEL	4

#define PAGE_TOP_BAR_HEIGHT		70
#define PAGE_BOT_BAR_HEIGHT		29
#define PAGE_USABLE_HEIGHT		tft_H - (PAGE_TOP_BAR_HEIGHT + PAGE_BOT_BAR_HEIGHT)

#define PAGE_BOT_BAR_MSG_LEN	20

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t lft_x, top_y, width, height;
}Page;

typedef struct {
	uint16_t x;
	uint16_t y;
}Point;

typedef void (menu_printer_func_t)(void);

typedef struct {
	bool is_defined;
	uint8_t x;
	uint8_t y;
	char *header_string;
	uint16_t char_color;
	uint16_t background_color;
	uint8_t size;
}menu_header_t;

typedef menu_header_t *menu_header_ptr_t;

typedef enum {
	MENU_MEASURE = 0,
	MENU_CALIBRATION_GLOBAL,
	MENU_CALIBRATION_CLAMP,
	MENU_DISPLAY_CONFIG
}current_menu_t;

typedef enum {
	MENU_MEASUREMENT_ITEM_OVRL_Z = 0,
	MENU_MEASUREMENT_ITEM_OVRL_R,
	MENU_MEASUREMENT_ITEM_OVRL_X,
	MENU_MEASUREMENT_ITEM_OVRL_PHI,
	
	MENU_MEASUREMENT_ITEM_CLAMP_Z,
	MENU_MEASUREMENT_ITEM_CLAMP_R,
	MENU_MEASUREMENT_ITEM_CLAMP_X,
	MENU_MEASUREMENT_ITEM_CLAMP_I_I,
	MENU_MEASUREMENT_ITEM_CLAMP_I_Q,
	MENU_MEASUREMENT_ITEM_CLAMP_PHI,
	
	MENU_MEASUREMENT_ITEM_VAPPLIED_MAG,
	MENU_MEASUREMENT_ITEM_VOUT_MAG
}display_page_Measure_line_t;

typedef enum {
	REPRINT_ONLYVALUE = 0,
	REPRINT_ALL
}reprint_item_t;


typedef struct {
	bool if_reprint_all		: 1;

	bool line_is_seltd		: 1;
	bool val_is_seltd		: 1;

	uint8_t seltd_line		: 4;
	uint8_t line_to_reprint		: 4;
	uint8_t seltd_char		: 4;
	
	uint16_t y_current_line;
	
	uint8_t dy_std;
	uint16_t y_std_first_row;
    Page window;
	
	reprint_item_t reprint;
	current_menu_t current_menu;
	current_menu_t previous_menu;

	menu_printer_func_t *menu_printer[6];
	menu_header_t Header[6];
	
	const char *bot_header_msg;
}MMMenu_t;

extern MMMenu_t MMMenu;

typedef struct {
	color_t		value_color;
	color_t		main_text_color;
	color_t		special_text_color;
	
	uint16_t	y_first_row;
	uint16_t	x_val;
	
	uint8_t		value_precision;
}page_params_Measure_t;

extern page_params_Measure_t page_params_Measure;

typedef struct {
	color_t		output_enabled_bk_color;
	color_t		output_disabled_bk_color;
	color_t		output_enabled_main_text_color;
	color_t		output_disabled_main_text_color;
	color_t		info_text_color;
    Page    header_coordinates;
	Point		output_status_text_xy;
	Point		info_text_xy;
}page_params_TopHeader_t;

extern page_params_TopHeader_t page_params_TopHeader;

typedef struct {
	color_t		normal_bk_color;
	color_t		error_bk_color;
	color_t		text_color;
    Page    header_coordinates;
	Point		msg_coordinates;
}page_params_BotHeader_t;

extern page_params_BotHeader_t page_params_BotHeader;

typedef struct {
	uint16_t x_val;
	uint16_t y_first_row;
}page_params_Menu_t;

extern page_params_Menu_t page_params_Menu;

#ifdef __cplusplus
}
#endif

#endif /* MENU_TYPES_ILI9486_H_ */