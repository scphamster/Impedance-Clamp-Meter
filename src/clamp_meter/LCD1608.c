/*
 * LCD1608.c
 *
 * Created: 29.09.2021 12:33:18
 *  Author: malygosstationar
 */
#include <asf.h>
#include "arm_math.h"
#include "stdio.h"
#include "LCD1608.h"
#include "twi.h"
#include "twi_pdc.h"

#define LCD_PIN_RS			0
#define LCD_PIN_RW			1
#define LCD_PIN_E			2
#define LCD_PIN_BACKLIGHT	3

#define LCD_CMD_CLEAR	0x01
#define LCD_CMD_HOME	0x02
#define LCD_MAX_STRLEN	50

#define TEST_TWI_PDC	

#ifdef __cplusplus
extern "C" {
#endif

void LCD_send_cmd(uint8_t cmd);
extern char * gcvtf(float,int,char *);


#ifdef TEST_TWI_PDC
void LCD_send_cmd(uint8_t cmd)
{
	static uint8_t buffer[5];
	
	buffer[0] = (1 << LCD_PIN_BACKLIGHT);
	buffer[1] =	buffer[0] | (1 << LCD_PIN_E) | (cmd & 0xF0);
	buffer[2] = buffer[1] & (~(1 << LCD_PIN_E));
	buffer[3] = buffer[0] | ((cmd << 4) & 0xF0) | (1 << LCD_PIN_E);
	buffer[4] = buffer[3] & (~(1 << LCD_PIN_E));

	twiPdc_write(buffer, 5, LCD_ADDR);
}

void LCD_send_character(uint8_t data)
{
	static uint8_t buffer[6];
	
	buffer[0] = (1 << LCD_PIN_BACKLIGHT) | (1 << LCD_PIN_RS);
	buffer[1] =	buffer[0] | (1 << LCD_PIN_E) | (data & 0xF0);
	buffer[2] = buffer[1] & (~(1 << LCD_PIN_E));
	buffer[3] = buffer[0] | ((data << 4) & 0xF0) | (1 << LCD_PIN_E);
	buffer[4] = buffer[3] & (~(1 << LCD_PIN_E));
	buffer[5] = buffer[4] & (~(1 << LCD_PIN_RS));

	twiPdc_write(buffer, 6, LCD_ADDR);
}

#else

void LCD_send_cmd(uint8_t cmd)
{
	static uint8_t buffer[8];
	static twi_packet_t write_packet;
	
	buffer[0] = (1 << LCD_PIN_BACKLIGHT);
	buffer[1] = (cmd & 0xF0) | (1 << LCD_PIN_BACKLIGHT);
	buffer[2] = buffer[0] | (1 << LCD_PIN_E);
	buffer[3] = buffer[1];
	buffer[4] = ((cmd << 4) & 0xF0) | (1 << LCD_PIN_BACKLIGHT);
	buffer[5] = buffer[4] | (1 << LCD_PIN_E);
	buffer[6] = buffer[4];
	buffer[7] = (1 << LCD_PIN_BACKLIGHT);
	
	write_packet.addr_length = 0;
	write_packet.length = 8;
	write_packet.buffer = (void *) &buffer;
	write_packet.chip = LCD_ADDR;

	twi_master_write(TWI0, &write_packet);
}

void LCD_send_character(uint8_t data)
{
	static uint8_t buffer[8];
	static twi_packet_t write_packet;
	
	buffer[0] = (1 << LCD_PIN_BACKLIGHT);
	buffer[1] = (data & 0xF0) | (1 << LCD_PIN_BACKLIGHT) | (1 << LCD_PIN_RS);
	buffer[2] = buffer[1] | (1 << LCD_PIN_E);
	buffer[3] = buffer[1];
	buffer[4] = ((data << 4) & 0xF0) | (1 << LCD_PIN_BACKLIGHT) | (1 << LCD_PIN_RS);
	buffer[5] = buffer[4] | (1 << LCD_PIN_E);
	buffer[6] = buffer[4];
	buffer[7] = (1 << LCD_PIN_BACKLIGHT);
	
	write_packet.addr_length = 0;
	write_packet.length = 8;
	write_packet.buffer = &buffer;
	write_packet.chip = LCD_ADDR;

	twi_master_write(TWI0, &write_packet);
}

#endif

void LCD_clear(void)
{
	LCD_send_cmd(LCD_CMD_CLEAR);
}

void LCD_display_off(void)
{
	LCD_send_cmd(0x08);
}

void LCD_display_on(void)
{
	LCD_send_cmd(0x0C);
}

void LCD_cursor_enable(void)
{
	LCD_send_cmd(0x0F);
}

void LCD_cursor_disable(void)
{
	LCD_send_cmd(0x0C);
}

void LCD_home(void)
{
	LCD_send_cmd(0x02);
}

void set_display_databus(void)
{
	LCD_send_cmd(0x28);		//4bit 2lines 5*8
}

void set_cursor_movement_direction(void)
{
	LCD_send_cmd(0x06);		//increment, disp shift off
}

void LCD_init(void)
{
	LCD_display_off();
	LCD_home();
	set_display_databus();
	set_cursor_movement_direction();
	LCD_display_on();
	LCD_clear();
}

void LCD_write(const char *data)
{
	while (*data) {
		LCD_send_character(*data);
		data++;
	}
}

void LCD_write_number(int32_t number)
{
	bool negval_flag = false;
	static uint8_t str[LCD_MAX_STRLEN];
	uint8_t char_numA = 0;
	uint8_t char_numB = 0;
	uint8_t stringlen = 0;
	uint8_t char_buff = 0;

	if (number < 0) {
		negval_flag = true;
		str[char_numA] = '-';
		
		char_numA++;
		stringlen++;
		
		number *= -1;
	}

	do {
		str[char_numA] = number % 10 + '0';
		number /= 10;
		
		char_numA++;
		char_numB++;
		stringlen++;
	} while ((number > 0) && (char_numA < LCD_MAX_STRLEN));

	if (negval_flag) {
		char_numA = 1;
	} else {
		char_numA = 0;
		char_numB--;
	}

	while(char_numA < char_numB) {
		char_buff = str[char_numA];
		str[char_numA] = str[char_numB];
		str[char_numB] = char_buff;
		
		char_numA++;
		char_numB--;
	}
	
	str[stringlen] = '\0';
	
	LCD_write(str);
}

void LCD_write_number_f(float32_t number)
{
	static char str[LCD_MAX_STRLEN];
	
	gcvtf(number, 3, str);
	LCD_write(str);
}

void LCD_write_float2(float32_t number, uint8_t digits)
{
	static char str[LCD_MAX_STRLEN];
	
	gcvtf(number, digits, str);
	LCD_write(str);
}


void LCD_cursor_setpos(uint8_t column, uint8_t line)
{
	uint8_t pos = 0;

	if (line == 1)
		pos = 0x80;
	else if (line == 2) 
		pos = 0xC0;
	else if (line == 3)
		pos = 0x80 + 20;
	else if (line == 4)
		pos = 0xC0 + 20;
		

	pos = pos - 1 + column;
	LCD_send_cmd(pos);
}

#ifdef __cplusplus
}
#endif