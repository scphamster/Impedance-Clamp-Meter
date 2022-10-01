#pragma once

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

