/*
 * menu_ili9486.c
 *
 * Created: 13.11.2021 18:57:09
 *  Author: malygosstationar
 */

#include "asf.h"
#include "menu_types_ili9486.h"
#include "menu_ili9486_kbrd_mngr.h"
#include "DSP_functions.h"
#include "menu_ili9486.h"
#include "signal_conditioning.h"
#include "menu.h"

#ifdef __cplusplus
extern "C" {
#endif

MMMenu_t                MMMenu;
page_params_Measure_t   page_params_Measure;
page_params_TopHeader_t page_params_TopHeader;
page_params_BotHeader_t page_params_BotHeader;
page_params_Menu_t      page_params_Menu;

void        show_page_measurement_all(void);
static void main_struct_init(void);
static void page_measure_all_init(void);
static void top_header_init(void);
static void bot_header_init(void);

void
display_init(void)
{
    main_struct_init();
    page_measure_all_init();
    top_header_init();
    bot_header_init();

    MMMenu.if_reprint_all = true;
    MMMenu.reprint        = REPRINT_ALL;

    show_page_measurement_all();

    MMMenu.if_reprint_all = true;
    MMMenu.reprint        = REPRINT_ALL;
    display_show_top_bar();

    MMMenu.if_reprint_all = true;
    display_show_bot_bar();
}

void
display_reset_vars(bool if_reprint_all)
{
    MMMenu.seltd_line      = 0;
    MMMenu.line_to_reprint = 0;
    MMMenu.line_is_seltd   = false;
    MMMenu.val_is_seltd    = false;
    MMMenu.if_reprint_all  = if_reprint_all;
}

static void
main_struct_init(void)
{
    MMMenu.dy_std          = 30;
    MMMenu.y_std_first_row = 100;
    MMMenu.window.lft_x    = 0;
    MMMenu.window.top_y    = PAGE_TOP_BAR_HEIGHT;
    MMMenu.window.width    = tft_W;
    MMMenu.window.height   = PAGE_USABLE_HEIGHT;
    MMMenu.bot_header_msg  = NULL;
}

static void
top_header_init(void)
{
    page_params_TopHeader.output_enabled_bk_color         = COLOR_RED;
    page_params_TopHeader.output_disabled_bk_color        = COLOR_GREEN;
    page_params_TopHeader.output_enabled_main_text_color  = COLOR_WHITE;
    page_params_TopHeader.output_disabled_main_text_color = COLOR_GREY;
    page_params_TopHeader.info_text_color                 = COLOR_WHITE;
    page_params_TopHeader.output_status_text_xy.x         = 60;
    page_params_TopHeader.output_status_text_xy.y         = 0;
    page_params_TopHeader.info_text_xy.x                  = 0;
    page_params_TopHeader.info_text_xy.y                  = 40;
    page_params_TopHeader.header_coordinates.lft_x        = 0;
    page_params_TopHeader.header_coordinates.width        = tft_W;
    page_params_TopHeader.header_coordinates.top_y        = 0;
    page_params_TopHeader.header_coordinates.height       = PAGE_TOP_BAR_HEIGHT;
}

static void
bot_header_init(void)
{
    page_params_BotHeader.normal_bk_color           = COLOR_DARKCYAN;
    page_params_BotHeader.error_bk_color            = COLOR_REDYELLOW;
    page_params_BotHeader.text_color                = COLOR_WHITE;
    page_params_BotHeader.header_coordinates.lft_x  = 0;
    page_params_BotHeader.header_coordinates.top_y  = tft_H - PAGE_BOT_BAR_HEIGHT;
    page_params_BotHeader.header_coordinates.width  = tft_W;
    page_params_BotHeader.header_coordinates.height = PAGE_BOT_BAR_HEIGHT;
    page_params_BotHeader.msg_coordinates.x         = page_params_BotHeader.header_coordinates.lft_x;
    page_params_BotHeader.msg_coordinates.y         = page_params_BotHeader.header_coordinates.top_y + 5;
}

static void
page_measure_all_init(void)
{
    page_params_Measure.value_color        = COLOR_GREEN;
    page_params_Measure.main_text_color    = COLOR_WHITE;
    page_params_Measure.special_text_color = COLOR_RED;
    page_params_Measure.y_first_row        = 80;
    page_params_Measure.x_val              = 140;
    page_params_Measure.value_precision    = 8;

    MMMenu.Header[MENU_MEASURE].is_defined = false;
    MMMenu.menu_printer[MENU_MEASURE]      = show_page_measurement_all;
}

static inline void
clear_screen(void)
{
    TFT_frect_print(MMMenu.window.lft_x, MMMenu.window.top_y, MMMenu.window.width, MMMenu.window.height, COLOR_BLACK);
}

static inline void
clear_line(uint16_t y, uint16_t bk_color)
{
    TFT_frect_print(MMMenu.window.lft_x, y, MMMenu.window.width, FONT_HEIGHT, bk_color);
}

uint16_t
set_cursor_in_line(uint8_t line, uint16_t y_std_first_row, uint16_t *bk_color)
{
    uint16_t y;

    y = y_std_first_row + MMMenu.dy_std * line;
    TFT_cursor_set(0, y);

    if ((MMMenu.line_is_seltd) && (line == MMMenu.seltd_line))
        *bk_color = SELECTED_LINE_BACKGR_COLOR;
    else
        *bk_color = NOTSELECTED_LINE_BACKGR_COLOR;

    clear_line(y, *bk_color);
    TFT_text_color_set(COLOR_WHITE, *bk_color);

    return y;
}

static inline void
set_cursor(uint8_t line_num, color_t *bk_color, uint8_t font_size, bool only_value, bool clear)
{
    uint16_t y;
    uint16_t x;
    uint16_t clear_width;

    switch (MMMenu.current_menu) {
    case MENU_MEASURE:
        y = page_params_Measure.y_first_row;
        x = page_params_Measure.x_val;
        break;

    default: y = MMMenu.y_std_first_row; x = 0;
    }

    y += line_num * MMMenu.dy_std;

    if (!only_value)
        x = MMMenu.window.lft_x;

    clear_width = MMMenu.window.width - x - MMMenu.window.lft_x;

    if ((MMMenu.line_is_seltd) && (line_num == MMMenu.seltd_line))
        *bk_color = SELECTED_LINE_BACKGR_COLOR;
    else
        *bk_color = NOTSELECTED_LINE_BACKGR_COLOR;

    if (clear)
        TFT_frect_print(x, y, clear_width, font_size * FONT_HEIGHT, *bk_color);

    TFT_cursor_set(x, y);

    MMMenu.y_current_line = y;
}

static inline void
print_value_f(float32_t value, uint8_t value_digits, uint8_t line_num, uint16_t font_size)
{
    color_t bk_color;
    color_t value_color;

    switch (MMMenu.current_menu) {
    case MENU_MEASURE: value_color = page_params_Measure.value_color; break;

    default: value_color = COLOR_YELLOW;
    }

    set_cursor(line_num, &bk_color, font_size, true, true);
    TFT_text_color_set(value_color, bk_color);

    TFT_print_number_f(value, value_digits, font_size);
}

static inline void
print_line_text(const char *str, uint8_t line_num, uint8_t font_size)
{
    color_t text_color;
    color_t bk_color;

    switch (MMMenu.current_menu) {
    case MENU_MEASURE: text_color = page_params_Measure.main_text_color; break;

    default: text_color = COLOR_YELLOW;
    }

    if ((MMMenu.line_is_seltd) && (line_num == MMMenu.seltd_line))
        bk_color = SELECTED_LINE_BACKGR_COLOR;
    else
        bk_color = NOTSELECTED_LINE_BACKGR_COLOR;

    TFT_cursor_set(MMMenu.window.lft_x, MMMenu.y_current_line);
    TFT_text_color_set(text_color, bk_color);

    TFT_print_str(str, TFT_STR_M_BACKGR, font_size);
}

void
display_show_top_bar(void)
{
    color_t color;

    if ((MMMenu.if_reprint_all) && (MMMenu.reprint == REPRINT_ALL)) {
        if (Analog.generator_is_active)
            color = page_params_TopHeader.output_enabled_bk_color;
        else
            color = page_params_TopHeader.output_disabled_bk_color;

        TFT_frect_print(page_params_TopHeader.header_coordinates.lft_x,
                        page_params_TopHeader.header_coordinates.top_y,
                        page_params_TopHeader.header_coordinates.width,
                        page_params_TopHeader.header_coordinates.height,
                        color);
    }

    TFT_cursor_set(page_params_TopHeader.output_status_text_xy.x, page_params_TopHeader.output_status_text_xy.y);

    if (Analog.generator_is_active) {
        TFT_text_color_set(page_params_TopHeader.output_enabled_main_text_color,
                           page_params_TopHeader.output_enabled_bk_color);
        TFT_print_str("!ACTIVE!", TFT_STR_M_BACKGR, 2);

        TFT_cursor_set(page_params_TopHeader.info_text_xy.x, page_params_TopHeader.info_text_xy.y);
        TFT_text_color_set(page_params_TopHeader.info_text_color, page_params_TopHeader.output_enabled_bk_color);

        switch (Analog.selected_sensor) {
        case VOLTAGE_SENSOR: TFT_print_str("Sensor: Vout ", TFT_STR_M_BACKGR, 1); break;

        case SHUNT_SENSOR: TFT_print_str("Sensor: Shunt", TFT_STR_M_BACKGR, 1); break;

        case CLAMP_SENSOR: TFT_print_str("Sensor: Clamp", TFT_STR_M_BACKGR, 1); break;
        }
    }
    else {
        TFT_text_color_set(page_params_TopHeader.output_disabled_main_text_color,
                           page_params_TopHeader.output_disabled_bk_color);
        TFT_print_str("Disabled", TFT_STR_M_BACKGR, 2);
    }
}

void
display_show_bot_bar(void)
{
    color_t color;

    if (MMMenu.if_reprint_all)
        TFT_frect_print(page_params_BotHeader.header_coordinates.lft_x,
                        page_params_BotHeader.header_coordinates.top_y,
                        page_params_BotHeader.header_coordinates.width,
                        page_params_BotHeader.header_coordinates.height,
                        page_params_BotHeader.normal_bk_color);

    TFT_cursor_set(page_params_BotHeader.msg_coordinates.x, page_params_BotHeader.msg_coordinates.y);
    TFT_text_color_set(page_params_BotHeader.text_color, page_params_BotHeader.normal_bk_color);

    if (MMMenu.bot_header_msg != NULL)
        TFT_print_str(MMMenu.bot_header_msg, TFT_STR_M_BACKGR, 1);
}

void
show_page_menu(void)
{ }

void
show_page_measurement_all(void)
{
    if (MMMenu.if_reprint_all)
        MMMenu.line_to_reprint = 0;

    switch (MMMenu.line_to_reprint) {
    case MENU_MEASUREMENT_ITEM_OVRL_Z:
        print_value_f(clamp_measurements_result.Z_ovrl, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_OVRL_Z, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Overall Z:", MENU_MEASUREMENT_ITEM_OVRL_Z, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_OVRL_R:
        print_value_f(clamp_measurements_result.R_ovrl, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_OVRL_R, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Overall R:", MENU_MEASUREMENT_ITEM_OVRL_R, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_OVRL_X:
        print_value_f(clamp_measurements_result.X_ovrl, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_OVRL_X, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Overall X:", MENU_MEASUREMENT_ITEM_OVRL_X, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_OVRL_PHI:
        print_value_f(clamp_measurements_result.Z_ovrl_phi, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_OVRL_PHI, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Overall phi:", MENU_MEASUREMENT_ITEM_OVRL_PHI, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_CLAMP_Z:
        print_value_f(clamp_measurements_result.Z_clamp, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_CLAMP_Z, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Clamp Z:", MENU_MEASUREMENT_ITEM_CLAMP_Z, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_CLAMP_R:
        print_value_f(clamp_measurements_result.R_clamp, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_CLAMP_R, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Clamp R:", MENU_MEASUREMENT_ITEM_CLAMP_R, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_CLAMP_X:
        print_value_f(clamp_measurements_result.X_clamp, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_CLAMP_X, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Clamp X:", MENU_MEASUREMENT_ITEM_CLAMP_X, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_CLAMP_I_I:
        print_value_f(clamp_measurements_result.I_clamp_I, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_CLAMP_I_I, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Clamp I_i:", MENU_MEASUREMENT_ITEM_CLAMP_I_I, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_CLAMP_I_Q:
        print_value_f(clamp_measurements_result.I_clamp_Q, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_CLAMP_I_Q, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Clamp I_q:", MENU_MEASUREMENT_ITEM_CLAMP_I_Q, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_CLAMP_PHI:
        print_value_f(clamp_measurements_result.Z_clamp_phi, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_CLAMP_PHI, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("Clamp phi:", MENU_MEASUREMENT_ITEM_CLAMP_PHI, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_VAPPLIED_MAG:
        print_value_f(clamp_measurements_result.V_applied, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_VAPPLIED_MAG, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("V applied:", MENU_MEASUREMENT_ITEM_VAPPLIED_MAG, 1);

        if (!MMMenu.if_reprint_all)
            break;

    case MENU_MEASUREMENT_ITEM_VOUT_MAG:
        print_value_f(clamp_measurements_result.V_ovrl, page_params_Measure.value_precision, MENU_MEASUREMENT_ITEM_VOUT_MAG, 1);

        if (MMMenu.reprint == REPRINT_ALL)
            print_line_text("V gen:", MENU_MEASUREMENT_ITEM_VOUT_MAG, 1);

        if (!MMMenu.if_reprint_all)
            break;

    default: return;
    }
}

void
display_print_page(void)
{
    if ((MMMenu.if_reprint_all) && (MMMenu.reprint == REPRINT_ALL)) {
        clear_screen();
        uint8_t i = 0;

        if (MMMenu.Header[MMMenu.current_menu].is_defined) {
            TFT_cursor_set(MMMenu.Header[MMMenu.current_menu].x, MMMenu.Header[MMMenu.current_menu].y);
            TFT_text_color_set(MMMenu.Header[MMMenu.current_menu].char_color,
                               MMMenu.Header[MMMenu.current_menu].background_color);
            TFT_print_str(MMMenu.Header[MMMenu.current_menu].header_string,
                          TFT_STR_M_BACKGR,
                          MMMenu.Header[MMMenu.current_menu].size);
        }
    }

    (*MMMenu.menu_printer[MMMenu.current_menu])();

    MMMenu.if_reprint_all = false;
}

void
display_refresh(void)
{
    MMMenu.if_reprint_all = true;
    MMMenu.reprint        = REPRINT_ONLYVALUE;

    display_print_page();
    display_show_top_bar();
    display_show_bot_bar();
}

#ifdef __cplusplus
}
#endif