/*
 * LCD1608.h
 *
 * Created: 29.09.2021 12:33:30
 *  Author: malygosstationar
 */ 


#ifndef LCD1608_H_
#define LCD1608_H_
#define LCD_ADDR		0b00100111
#define LCD_TWI_FREQ	100000UL
#include "arm_math.h"
/*
lcd module 1602 is connected via II2 expander PCF8574:
P0 - RS
P1 - RW
P2 - E
P3 - backlight???
P4 - D4
P5 - D5
P6 - D6
P7 - D7
*/
#ifdef __cplusplus
extern "C" {
#endif

void LCD_send_character(uint8_t data);
void LCD_init(void);
void LCD_clear(void);
void LCD_cursor_enable(void);
void LCD_display_off(void);
void LCD_cursor_disable(void);
void LCD_write(const char *data);
void LCD_write_number(int32_t number);
void LCD_write_number_f(float32_t number);
void LCD_write_float2(float32_t number, uint8_t digits);
void LCD_cursor_setpos(uint8_t column, uint8_t line);

#ifdef __cplusplus
}
#endif

#endif /* LCD1608_H_ */