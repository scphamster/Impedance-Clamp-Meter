/*
 * menu_ili9486.h
 *
 * Created: 13.11.2021 16:25:22
 *  Author: malygosstationar
 */

#ifndef MENU_ILI9486_H_
#define MENU_ILI9486_H_

//#include "ILI9486_public.h"

#define SELECTED_LINE_BACKGR_COLOR    COLOR_DARKDARKGREY
#define NOTSELECTED_LINE_BACKGR_COLOR COLOR_BLACK

#ifdef __cplusplus
extern "C" {
#endif

void display_print_page(void);
void display_reset_vars(bool if_reprint_all);
void display_init(void);
void display_show_top_bar(void);
void display_refresh(void);
void display_show_bot_bar(void);

#ifdef __cplusplus
}
#endif

#endif /* MENU_ILI9486_H_ */