
#include "asf.h"
#include <stdlib.h>
#include "ili9486_fonts.h"
#include "ili9486_config.h"
#include "ili9486_public.h"
#include "ili9486_private.h"
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t font_width, font_height;
uint8_t font_offset;
uint8_t *font_addr;
uint16_t tft_W;
uint16_t tft_H;
int16_t   _width, _height, cursor_x, cursor_y , char_color ;
uint16_t   PIXEL_COLOUR, BACK_COLOUR;
uint8_t    textsize, rotation;

extern char * gcvtf(float,int,char *);

static inline void tft_cs_enable(void)
{
	pio_clear(TFT_CS_PIO, TFT_CS_PIN);
}

static inline void tft_cs_disable(void)
{
	pio_set(TFT_CS_PIO, TFT_CS_PIN);
}

static inline void tft_cmd_set(void)
{
	pio_clear(TFT_CMD_DATA_PIO, TFT_CMD_DATA_PIN);
}

static inline void tft_data_set(void)
{
	pio_set(TFT_CMD_DATA_PIO, TFT_CMD_DATA_PIN);
}

static inline void tft_nowrite_set(void)
{
	pio_set(TFT_WR_PIO, TFT_WR_PIN);
}

static inline void tft_write_set(void)
{
	pio_clear(TFT_WR_PIO, TFT_WR_PIN);
}

static inline void tft_reset_set(void)
{
	pio_clear(TFT_RESET_PIO, TFT_RESET_PIN);
}

static inline void tft_noreset_set(void)
{
	pio_set(TFT_RESET_PIO, TFT_RESET_PIN);
}

static inline void __attribute__ ((hot)) 
tft_write_strobe(void) 
{
	tft_write_set();
	//delay_us(10);
	tft_nowrite_set();
}

static inline void __attribute__ ((hot)) 
tft_data_port1(uint8_t data)
{
	uint32_t data_a = 0;
	uint32_t data_d = 0;
	
	pio_enable_output_write(PIOA, 0xD0000040ul);
	pio_enable_output_write(PIOD, 0x1E0ul);
	
	data_d |= (((data & (1 << 0)) && 1) << 8);
	
	data_a |= (((data & (1 << 1)) && 1) << 28);
	data_a |= (((data & (1 << 2)) && 1) << 30);
	data_a |= (((data & (1 << 3)) && 1) << 6);
	
	data_d |= (((data & (1 << 4)) && 1) << 7);
	
	data_a |= (((data & (1 << 5)) && 1) << 31);

	data_d |= (((data & (1 << 6)) && 1) << 5);
	data_d |= (((data & (1 << 7)) && 1) << 6);
	
	pio_sync_output_write(PIOA, data_a);
	pio_sync_output_write(PIOD, data_d);
	
	pio_disable_output_write(PIOA, 0xD0000040ul);
	pio_disable_output_write(PIOD, 0x1E0ul);
}

static inline void __attribute__ ((hot)) 
tft_data_port2(uint8_t data)
{
	uint32_t data_a = 0;
	uint32_t data_b = 0;
	uint32_t data_d = 0;
	
	pio_enable_output_write(PIOA, (1 << 29));
	pio_enable_output_write(PIOB, 0x4C00ul);
	pio_enable_output_write(PIOD, 0x1Eul);
	
	data_b |= (((data & (1 << 0)) && 1) << 11);
	data_b |= (((data & (1 << 1)) && 1) << 14);
	data_b |= (((data & (1 << 3)) && 1) << 10);
	
	data_d |= (((data & (1 << 2)) && 1) << 1);
	data_d |= (((data & (1 << 5)) && 1) << 2);
	data_d |= (((data & (1 << 6)) && 1) << 4);
	data_d |= (((data & (1 << 7)) && 1) << 3);
	
	data_a |= (((data & (1 << 4)) && 1) << 29);
	
	pio_sync_output_write(PIOA, data_a);
	pio_sync_output_write(PIOB, data_b);
	pio_sync_output_write(PIOD, data_d);
	
	pio_disable_output_write(PIOA, (1 << 29));
	pio_disable_output_write(PIOB, 0x4C00ul);
	pio_disable_output_write(PIOD, 0x1Eul);
}

static inline void tft_write_byte(uint8_t data)
{
	tft_data_port1(data);
	tft_write_strobe();
}

void
TFT_GPIO_Init(void)
{
	Matrix *p_matrix;
	p_matrix = MATRIX;
	
	//configure control pins
	pio_set_output(PIOD, 0x20008600ul, 0x20008600ul, 0, 0);
	pio_set_output(TFT_CS_PIO, TFT_CS_PIN, TFT_CS_PIN, 0, 0);

	//configure data pins
	p_matrix->CCFG_SYSIO |= CCFG_SYSIO_SYSIO10 | CCFG_SYSIO_SYSIO11;
	
	pio_set_output(PIOA, 0xF0000040ul, 0, 0, 0);
	pio_set_output(PIOB, 0x4C00ul, 0, 0, 0);
	pio_set_output(PIOD, 0x1FEul, 0, 0, 0);
}

void
TFT_Init(void)
{
	TFT_GPIO_Init();
	TFT_Reset();

	TFT_Write_Cmd_Byte(0x11);	// Sleep out, also SW reset
	delay_ms(60);

	TFT_Write_Cmd_Byte(0x3A);	//interface pixel format
	TFT_Write_Data_Byte(0x55);	//16bit RGB & 16bit CPU

	TFT_Write_Cmd_Byte(0xC2);	// Power Control 3 Normal mode
	TFT_Write_Data_Byte(0x44);

	TFT_Write_Cmd_Byte(0xC5);	//vcom control
	TFT_Write_Data_Byte(0x00);
	TFT_Write_Data_Byte(0x00);
	TFT_Write_Data_Byte(0x00);
	TFT_Write_Data_Byte(0x00);

	TFT_Write_Cmd_Byte(0xE0);	//Positive gamma CTRL
	TFT_Write_Data_Byte(0x0F);
	TFT_Write_Data_Byte(0x1F);
	TFT_Write_Data_Byte(0x1C);
	TFT_Write_Data_Byte(0x0C);
	TFT_Write_Data_Byte(0x0F);
	TFT_Write_Data_Byte(0x08);
	TFT_Write_Data_Byte(0x48);
	TFT_Write_Data_Byte(0x98);
	TFT_Write_Data_Byte(0x37);
	TFT_Write_Data_Byte(0x0A);
	TFT_Write_Data_Byte(0x13);
	TFT_Write_Data_Byte(0x04);
	TFT_Write_Data_Byte(0x11);
	TFT_Write_Data_Byte(0x0D);
	TFT_Write_Data_Byte(0x00);

	TFT_Write_Cmd_Byte(0xE1);	//Negative gamma CTRL
	TFT_Write_Data_Byte(0x0F);
	TFT_Write_Data_Byte(0x32);
	TFT_Write_Data_Byte(0x2E);
	TFT_Write_Data_Byte(0x0B);
	TFT_Write_Data_Byte(0x0D);
	TFT_Write_Data_Byte(0x05);
	TFT_Write_Data_Byte(0x47);
	TFT_Write_Data_Byte(0x75);
	TFT_Write_Data_Byte(0x37);
	TFT_Write_Data_Byte(0x06);
	TFT_Write_Data_Byte(0x10);
	TFT_Write_Data_Byte(0x03);
	TFT_Write_Data_Byte(0x24);
	TFT_Write_Data_Byte(0x20);
	TFT_Write_Data_Byte(0x00);

	TFT_Write_Cmd_Byte(0x20);	//display inversion OFF

	TFT_Write_Cmd_Byte(0x36);	//memory access CTRL
	TFT_Write_Data_Byte(0b01001001);	//MY MX MV ML BGR MH X X

	TFT_Write_Cmd_Byte(0x29);	// display on
	delay_ms(60);

	TFT_Set_Rotation(TFT_PORTRAIT);
}

#if 0
uint16_t
TFT_read_data_byte(uint8_t address)
{
	TFT_Write_Data_Byte(address);
	tft_data_set();
	TFT_DATA_PORT_DIR1 = 0x00;
	TFT_DATA_PORT_DIR2 = 0x00;
	tft_write_strobe();
	uint8_t lsb = PINA;
	uint8_t msb = PINB;

	TFT_DATA_PORT_DIR1 = 0xff;
	TFT_DATA_PORT_DIR2 = 0xff;

	return lsb | (msb << 8);
}
#endif

void
TFT_test_pio(void)
{
	while(1){
		tft_data_port1(0xff);
		tft_data_port2(0x00);
		
		tft_cs_enable();
		tft_cmd_set();
		tft_write_set();
		tft_reset_set();
		
		tft_cs_disable();
		tft_data_set();
		tft_nowrite_set();
		tft_noreset_set();


		tft_data_port1(0x00);
		tft_data_port2(0xff);
		
		tft_cs_enable();
		tft_cmd_set();
		tft_write_set();
		tft_reset_set();
		
		tft_cs_disable();
		tft_data_set();
		tft_nowrite_set();
		tft_noreset_set();
		
		tft_data_port2(0x00);
	}
}

void
TFT_Reset(void)
{
	tft_cs_disable();
	//TFT_CTRL_NOREAD;
	tft_nowrite_set();
	
	tft_noreset_set();

	delay_ms(50);
	tft_reset_set();
	delay_ms(100);

	tft_noreset_set();
	delay_ms(100);
}

void
TFT_Write_Data_Byte(uint8_t data)
{
	tft_cs_enable();
	tft_data_set();

	tft_write_byte(data);

	tft_cs_disable();
}

static inline void
TFT_Write_Cmd_Byte(uint8_t cmd)
{
	tft_cs_enable();
	tft_cmd_set();
	tft_write_byte(cmd);
	tft_cs_disable();
}


static inline void
TFT_Write_Data_Word(uint16_t data)
{
	tft_cs_enable();
	tft_data_set();

	tft_write_byte(data >> 8 );
	tft_write_byte(data & 0xff);

	tft_cs_disable();
}

static inline void
TFT_Write_Cmd_Word(uint16_t cmd)
{
	/* CS_ACTIVE */
	tft_cs_enable();

	/* CD/RS COMMAND */
	tft_cmd_set();

	tft_write_byte(cmd >> 8 );
	tft_write_byte(cmd & 0xff );

	/* CS_IDLE */
	tft_cs_disable();
}


static inline void
TFT_Write_Cmd_Data(uint16_t cmd,
                   uint16_t data)
{
	TFT_Write_Cmd_Byte(cmd);
	TFT_Write_Data_Word(data);
}

static inline void
TFT_Write_Data_Cmd(uint16_t cmd,
                   uint16_t data)
{
	TFT_Write_Cmd_Byte(cmd);
	TFT_Write_Data_Word(data);
}


static inline void
TFT_Write_Cmd_ParamN(uint8_t cmd,
                     int8_t N,
                     uint8_t *block)
{
	TFT_Write_Cmd_Byte(cmd);

	while (N--) {
		uint8_t Z = *block++;
		TFT_Write_Data_Byte(Z);;
	}
}

static inline void
TFT_Write_Cmd_Param4(uint8_t cmd,
                     uint8_t d1,
                     uint8_t d2,
                     uint8_t d3,
                     uint8_t d4)
{
	uint8_t d[4];
	d[0] = d1, d[1] = d2, d[2] = d3, d[3] = d4;
	TFT_Write_Cmd_ParamN(cmd, 4, d);
}


void
TFT_Clear(uint16_t color)
{
	TFT_fillScreen(color);
}

void
TFT_Scroll_Vertical(int16_t top,
                    int16_t scrollines,
                    int16_t offset)
{
	int16_t bfa = tft_H - top - scrollines;  // bottom fixed area
	int16_t vsp;
	//int16_t sea = top;
	vsp = top + offset; // vertical start position

	if (offset < 0) {
		vsp += scrollines;     //keep in unsigned range
	}

	//sea = top + scrollines - 1;



	uint8_t d[6];           // for multi-byte parameters

	d[0] = top >> 8;        //TFA
	d[1] = top;
	d[2] = scrollines >> 8; //VSA
	d[3] = scrollines;
	d[4] = bfa >> 8;        //BFA
	d[5] = bfa;
	TFT_Write_Cmd_ParamN(0x33, 6, d);

	d[0] = vsp >> 8;        //VSP
	d[1] = vsp;
	TFT_Write_Cmd_ParamN(0x37, 2, d);

}

void
TFT_SetAddrWindow (int16_t x_beg,
                   int16_t y_beg,
                   int16_t x_end,
                   int16_t y_end)
{
	TFT_Write_Cmd_Param4(TFT_CASET, x_beg >> 8, x_beg, x_end >> 8, x_end);
	TFT_Write_Cmd_Param4(TFT_PASET, y_beg >> 8, y_beg, y_end >> 8, y_end);
}

void
TFT_Set_Rotation(uint8_t rotation)
{
#define RGB 1
#define BGR 0
	//ob MY MX MV ML	BGR MH 0 0
	TFT_Write_Cmd_Byte(TFT_MADCTL);

	if (rotation == 0) {
		TFT_Write_Data_Byte(0x48);		//0b 0100 1000;
		tft_W = TFT_WIDTH;
		tft_H = TFT_HEIGHT;
	}

	if (rotation == 1) {				//0b 0010 1000;
		TFT_Write_Data_Byte(0x28);
		tft_W = TFT_HEIGHT;
		tft_H = TFT_WIDTH;
	}

	if (rotation == 2) {
		TFT_Write_Data_Byte(0x98);		//0b 1001 1000;
		tft_W = TFT_WIDTH;
		tft_H = TFT_HEIGHT;
	}

	if (rotation == 3) {
		TFT_Write_Data_Byte(0xF8);
		tft_W = TFT_HEIGHT;
		tft_H = TFT_WIDTH;
	}

}

void
TFT_text_color_set(uint16_t pixelcolor,
                   uint16_t backcolour)
{
	PIXEL_COLOUR = pixelcolor;
	BACK_COLOUR	= backcolour;
}

void
TFT_cursor_set(int16_t x,
               int16_t y)
{
	cursor_x = x;
	cursor_y = y;
}


void
TFT_put_pixel(unsigned int color)
{

	tft_cs_enable();
	tft_data_set();

	tft_data_port1(0xff & color);
	tft_data_port2((color >> 8) & 0xff);

	tft_write_strobe();

	tft_cs_disable();
}

void
TFT_put_N_pixels(unsigned int color,
                 uint32_t n)
{
	TFT_Write_Cmd_Byte(TFT_RAMWR);

	tft_cs_enable();
	tft_data_set();

	tft_data_port1(0xff & color);
	tft_data_port2((color >> 8) & 0xff);

	while (n) {
		tft_write_strobe();
		n--;
	}

	tft_cs_disable();
}

void
drawPixel(uint16_t x,
          uint16_t y,
          uint16_t color)
{
	if (x < 0 || y < 0 || x >= tft_W || y >= tft_H)
		return;

	TFT_Write_Cmd_Param4(TFT_CASET, x >> 8, x, x >> 8, x);
	TFT_Write_Cmd_Param4(TFT_PASET, y >> 8, y, y >> 8, y);
	TFT_Write_Cmd_Byte(TFT_RAMWR);
	TFT_put_pixel(color);
	//TFT_Write_Cmd_Data(TFT_RAMWR,color);
}


void
TFT_fillScreen(uint16_t color)
{
	TFT_frect_print(0, 0, tft_W, tft_H, color);
}

void
TFT_frect_print(int16_t x,
                int16_t y,
                int16_t w, int16_t h,
                uint16_t color)
{
	int16_t end;
	uint32_t n_pixels;

	if (w < 0) {
		w = -w;
		x -= w;
	}

	end = x + w;

	if (x < 0) x = 0;

	if (end > tft_W) end = tft_W;

	w = end - x;

	if (h < 0) {
		h = -h;
		y -= h;
	}

	end = y + h;

	if (y < 0)
		y = 0;

	if (end > tft_H)
		end = tft_H;

	h = end - y;
	n_pixels = (uint32_t)h * w;
	TFT_SetAddrWindow(x, y, x + w - 1, y + h - 1);
	TFT_put_N_pixels(color, n_pixels);
}


void
TFT_Draw_Line(int16_t x1,
              int16_t y1,
              int16_t x2,
              int16_t y2,
              uint16_t color)
{

	int16_t i;
	int16_t dx, dy;
	int16_t sx, sy;
	int16_t E;

	/* distance between two points */
	if (x2 > x1)
		dx = x2 - x1 ;
	else
		dx = x1 - x2 ;

	if (y2 > y1)
		dy = y2 - y1 ;
	else
		dy = y1 - y2 ;



	/* direction of two point */

	if (x2 > x1)
		sx = 1 ;
	else
		sx = -1 ;

	if (y2 > y1)
		sy = 1 ;
	else
		sy = -1;


	if (y1 == y2) {
		if (x2 > x1)
			TFT_DrawFastHLine(x1, y1, x2 - x1 + 1, color);
		else
			TFT_DrawFastHLine(x2, y1, x1 - x2 + 1, color);

		return;
	} else if (x1 == x2) {
		if (y2 > y1)
			TFT_DrawFastVLine(x1, y1, y2 - y1 + 1, color);
		else
			TFT_DrawFastVLine(x1, y2, y1 - y2 + 1, color);

		return;
	}

	/* inclination < 1 */
	if ( dx > dy ) {
		E = -dx;

		for ( i = 0 ; i <= dx ; i++ ) {
			drawPixel( x1, y1, color );
			x1 += sx;
			E += 2 * dy;

			if ( E >= 0 ) {
				y1 += sy;
				E -= 2 * dx;
			}
		}

		/* inclination >= 1 */
	} else {
		E = -dy;

		for ( i = 0 ; i <= dy ; i++ ) {
			drawPixel( x1, y1, color );
			y1 += sy;
			E += 2 * dx;

			if ( E >= 0 ) {
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}

void
TFT_Draw_Circle(int16_t x0,
                int16_t y0,
                int16_t r,
                uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	drawPixel(x0  , y0 + r, color);
	drawPixel(x0  , y0 - r, color);
	drawPixel(x0 + r, y0  , color);
	drawPixel(x0 - r, y0  , color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x;

		drawPixel(x0 + x, y0 + y, color);
		drawPixel(x0 - x, y0 + y, color);
		drawPixel(x0 + x, y0 - y, color);
		drawPixel(x0 - x, y0 - y, color);
		drawPixel(x0 + y, y0 + x, color);
		drawPixel(x0 - y, y0 + x, color);
		drawPixel(x0 + y, y0 - x, color);
		drawPixel(x0 - y, y0 - x, color);
	}
}

void
TFT_fillCircle(int16_t x0,
               int16_t y0,
               int16_t r,
               uint16_t color)
{
	TFT_DrawFastVLine(x0, y0 - r, 2 * r + 1, color);
	TFT_fillCircleHelper(x0, y0, r, 3, 0, color);
}

void
TFT_Draw_Point(int16_t x0,
               int16_t y0 ,
               uint16_t color)
{
	int16_t r = 1 ;
	TFT_DrawFastVLine(x0, y0 - r, 2 * r + 1, color);
	TFT_fillCircleHelper(x0, y0, r, 3, 0, color);
}

void
TFT_fillCircleHelper(int16_t x0,
                     int16_t y0,
                     int16_t r,
                     uint8_t cornername,
                     int16_t delta,
                     uint16_t color)
{

	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f     += ddF_y;
		}

		x++;
		ddF_x += 2;
		f     += ddF_x;

		if (cornername & 0x1) {
			TFT_DrawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
			TFT_DrawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
		}

		if (cornername & 0x2) {
			TFT_DrawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
			TFT_DrawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
		}
	}
}

void
TFT_DrawRect(int16_t x,
             int16_t y,
             int16_t w,
             int16_t h,
             uint16_t color)
{
	TFT_DrawFastHLine(x, y, w, color);
	TFT_DrawFastHLine(x, y + h - 1, w, color);
	TFT_DrawFastVLine(x, y, h, color);
	TFT_DrawFastVLine(x + w - 1, y, h, color);
}

void
TFT_DrawFastVLine(int16_t x,
                  int16_t y,
                  int16_t h,
                  uint16_t color)
{
	if ((x >= tft_W) || (y >= tft_H || h < 1))
		return;

	if ((y + h - 1) >= tft_H)
		h = tft_H - y;

	TFT_frect_print(x, y, 1, h, color);
}

void
TFT_DrawFastHLine(int16_t x,
                  int16_t y,
                  int16_t w,
                  uint16_t color)
{
	if ((x >= tft_W) || (y >= tft_H || w < 1))
		return;

	if ((x + w - 1) >= tft_W)
		w = tft_W - x;

	TFT_frect_print(x, y, w, 1, color);
}

void
TFT_Print_Char(uint16_t x,
               uint16_t y,
               char data ,
               uint8_t mode,
               uint8_t size)
{
	if ((x > tft_W - FONT_WIDTH * size) || (y > tft_H - FONT_HEIGHT * size))
		return;

	uint8_t byte_row, bit_row, column, temp;
	uint8_t bit_row_counter = 0;
	bool char_color_is_set = false;
	uint8_t counter = 0;
	uint8_t data_from_memory[FONT_ONE_CHAR_BYTES];

	TFT_SetAddrWindow(x, y, x + FONT_WIDTH * size - 1,
	                  y + FONT_HEIGHT * size - 1);
	TFT_Write_Cmd_Byte(TFT_RAMWR);

	tft_cs_enable();
	tft_data_set();
	tft_data_port1(0xff & BACK_COLOUR);
	tft_data_port2((BACK_COLOUR >> 8) & 0xff);

	for (; counter < FONT_ONE_CHAR_BYTES; counter++)
		data_from_memory[counter] = Consolas14x24[(data - ' ') * FONT_ONE_CHAR_BYTES + counter];

	for (byte_row = 0; byte_row < 3; byte_row++) {
		for (bit_row = 0; bit_row < 8; bit_row++) {
			for (column = 0; column < FONT_WIDTH; column++) {
				temp = data_from_memory[column * 3 + byte_row];

				if (temp & (1 << bit_row)) {
					if (char_color_is_set) {
						tft_write_strobe();

						if (2 == size)
							tft_write_strobe();
					} else {
						char_color_is_set = true;
						tft_data_port1(0xff & PIXEL_COLOUR);
						tft_data_port2((PIXEL_COLOUR >> 8) & 0xff);
						tft_write_strobe();

						if (2 == size)
							tft_write_strobe();
					}
				} else {
					if (char_color_is_set) {
						char_color_is_set = false;
						tft_data_port1(0xff & BACK_COLOUR);
						tft_data_port2((BACK_COLOUR >> 8) & 0xff);
						tft_write_strobe();

						if (2 == size)
							tft_write_strobe();
					} else {
						tft_write_strobe();

						if (2 == size)
							tft_write_strobe();
					}
				}
			}

			bit_row_counter++;

			if (bit_row_counter == size)
				bit_row_counter = 0;
			else
				bit_row--;
		}
	}

	tft_cs_disable();
}

void
TFT_print_str(const char *string,
              uint8_t string_mode,
              uint8_t size)
{
	uint8_t i = 0;
	uint8_t font_w = (FONT_WIDTH - FONT_SQUISH) * size;
	uint8_t font_h = (FONT_HEIGHT - FONT_SQUISH) * size;

	while ((*(string + i) != '\0') && (*string)) {
		if (*(string + i) == '\n') {
			cursor_y += font_h;
			cursor_x = 0;
			string++;
		}

		if (cursor_x > tft_W - font_w) {
			cursor_x = 0;
			cursor_y += font_h;
		}

		if (cursor_y > tft_H - font_h)
			cursor_y = cursor_x = 0;

		TFT_Print_Char(cursor_x, cursor_y, *(string + i), string_mode, size);
		cursor_x += font_w;
		i++;
	}
}

void
TFT_print_number(uint16_t  number,
                 uint8_t TFT_STRING_MODE,
                 uint8_t size)
{

	int16_t temp = 1;

	if (number <= 0) {
		TFT_Print_Char(cursor_x, cursor_y, '0', TFT_STRING_MODE, size);
		cursor_x += 14 * size;
	} else {
		while (number != 0) {
			uint8_t remainder = number % 10;
			number = number / 10;
			temp = temp * 10 + remainder;
		}

		while (temp != 1) {
			uint8_t remainder = temp % 10;
			temp = temp / 10;
			TFT_Print_Char(cursor_x, cursor_y, remainder + 48, TFT_STRING_MODE, size);
			cursor_x += 14 * size;

		}
	}


}

void
TFT_print_number_f(float32_t value, uint8_t n_digits, uint8_t size)
{
	static char str[20];

//    snprintf(str, sizeof(str), "%f", value);

    gcvtf(value, n_digits, str);

	TFT_print_str(str, TFT_STR_M_BACKGR, size);
}

#ifdef __cplusplus
}
#endif