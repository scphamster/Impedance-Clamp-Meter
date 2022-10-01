/**
 * @authors: abdelrhamn werby scphamster
 */

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
//#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif
void TFT_Init			(void);
void TFT_test_pio		(void);
void TFT_Reset			(void);
void TFT_Clear			(uint16_t color);
void TFT_Set_Rotation	(uint8_t rotation);

//void TFT_Write_Data_Byte(uint8_t data);
//void TFT_Write_Cmd_Byte(uint8_t cmd);

void TFT_Draw_Line(int16_t x0, int16_t y0,int16_t x1, int16_t y1,uint16_t color);
void TFT_Draw_Circle(int16_t x0, int16_t y0, int16_t r,uint16_t color);
void TFT_DrawRect(int16_t x, int16_t y,  int16_t w, int16_t h,  uint16_t color);
void TFT_DrawFastVLine(int16_t x, int16_t y,int16_t h, uint16_t color);
void TFT_DrawFastHLine(int16_t x, int16_t y,int16_t w, uint16_t color);
void TFT_frect_print(int16_t x, int16_t y, int16_t w, int16_t h,uint16_t color);
void TFT_fillScreen(uint16_t color);
void TFT_fillCircle(int16_t x0, int16_t y0, int16_t r,uint16_t color);
void TFT_Draw_Point(int16_t x0, int16_t y0 ,uint16_t color);

void TFT_cursor_set				(int16_t x, int16_t y);
void TFT_text_color_set			(uint16_t pixelcolor, uint16_t backcolor);
void TFT_print_str				(const char *string , uint8_t TFT_STRING_MODE, uint8_t size);
void TFT_print_number			(uint16_t  Number, uint8_t TFT_STRING_MODE, uint8_t size);
void TFT_print_number_f			(float value, uint8_t n_digits, uint8_t size);

#define TFT_STR_M_NOBACKGR	0x01
#define TFT_STR_M_BACKGR	0x00

typedef uint16_t color_t;
#define COLOR_BLACK       0x0000      /*   0,   0,   0 */
#define COLOR_NAVY        0x000F      /*   0,   0, 128 */
#define COLOR_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define COLOR_DDGREEN 0xc0
#define COLOR_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define COLOR_MAROON      0x7800      /* 128,   0,   0 */
#define COLOR_PURPLE      0x780F      /* 128,   0, 128 */
#define COLOR_OLIVE       0x7BE0      /* 128, 128,   0 */
#define COLOR_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define COLOR_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define COLOR_BLUE        0x001F      /*   0,   0, 255 */
#define COLOR_GREEN       0x07E0      /*   0, 255,   0 */
#define COLOR_CYAN        0x07FF      /*   0, 255, 255 */
#define COLOR_RED         0xF800      /* 255,   0,   0 */
#define COLOR_MAGENTA     0xF81F      /* 255,   0, 255 */
#define COLOR_YELLOW      0xFFE0      /* 255, 255,   0 */
#define COLOR_WHITE       0xFFFF      /* 255, 255, 255 */
#define COLOR_ORANGE      0xFD20      /* 255, 165,   0 */
#define COLOR_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define COLOR_PINK        0xF81F
#define COLOR_DARKDARKGREY 0x514A		//10,10,10

#define COLOR_REDYELLOW		60580
#define COLOR_LIGHTMUD		42023
#define COLOR_GREY			10565

//screen orientations
#define TFT_PORTRAIT       0
#define TFT_LANDSCAPE      1
#define TFT_PORTRAIT_REV   2
#define TFT_LANDSCAPE_REV  3

#ifdef __cplusplus
}
#endif
