/*
 * TFT_Privte.h
 *
 *  Created on: Jan 23, 2019
 *      Author: abood
 */

#ifndef ILI9486_PRIVATE_H_
#define ILI9486_PRIVATE_H_
#include "asf.h"
/*********************************************************************
 *
 * Display control pins functions definition
 *
 ***********************************************************************/

#if 0
#define TFT_CTRL_CS_EN   TFT_CTRL_PORT &= ~CS_PIN   // chip select active
#define TFT_CTRL_CS_DIS  TFT_CTRL_PORT |= CS_PIN    // chip select disabled
#define TFT_CTRL_CMD     TFT_CTRL_PORT &= ~RS_PIN   // RESET		LO=Command
#define TFT_CTRL_DATA    TFT_CTRL_PORT |= RS_PIN    //			HI=Parameter
#define TFT_CTRL_NOREAD  TFT_CTRL_PORT |= RD_PIN    // READ		HI=disabled
#define TFT_CTRL_READ    TFT_CTRL_PORT &= ~RD_PIN   //			LO=enabled
#define TFT_CTRL_NOWRITE TFT_CTRL_PORT |= WR_PIN    // WRITE		HI=disabled
#define TFT_CTRL_WRITE   TFT_CTRL_PORT &= ~WR_PIN   //			LO=enabled
#define ILI_WR_STROBE                                                                                                    \
    TFT_CTRL_PORT &= ~WR_PIN;                                                                                            \
    TFT_CTRL_PORT |= WR_PIN                          // strobe to write/read data to/from LCD
#define TFT_CTRL_RESET   TFT_CTRL_PORT &= ~RST_PIN   // reset
#define TFT_CTRL_UNRESET TFT_CTRL_PORT |= RST_PIN    // get out from reset
#endif

/*********************************************************************
 *
 *  SCREEN SIZE
 *
 **********************************************************************/

#define TFT_WIDTH   320
#define TFT_HEIGHT  480
#define FONT_HEIGHT 24
#define FONT_WIDTH  14
#define FONT_SQUISH 3

#ifdef __cplusplus
extern "C" {
#endif

void               TFT_GPIO_Init(void);
static inline void Write_Byte(uint8_t data);
static inline void TFT_Write_Data_Byte(uint8_t data);
static inline void TFT_Write_Cmd_Byte(uint8_t cmd);
static inline void TFT_Wrte_Data_Word(uint16_t data);
static inline void TFT_Write_Cmd_Word(uint16_t data);
void               TFT_put_pixel(unsigned int data);
void               TFT_put_N_pixels(unsigned int color, unsigned long int n);
void               TFT_SetAddrWindow(int16_t x_beg, int16_t y_beg, int16_t x_end, int16_t y_end);
void TFT_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
void TFT_Print_Char(uint16_t x, uint16_t y, char data, uint8_t mode, uint8_t size);

extern uint16_t tft_W;
extern uint16_t tft_H;
extern int16_t  cursor_x, cursor_y, char_color;
extern uint16_t PIXEL_COLOUR, BACK_COLOUR;

#define PORTRAIT      0x48
#define LANDSCAPE     0x28
#define PORTRAIT_REV  0x98
#define LANDSCAPE_REV 0xF4

// SCREEN DRIVER COMMANDS
#define TFT_NOP     0x00
#define TFT_SWRST   0x01
#define TFT_CASET   0x2A
#define TFT_PASET   0x2B
#define TFT_RAMWR   0x2C
#define TFT_RAMRD   0x2E
#define TFT_MADCTL  0x36
#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_SS  0x02
#define TFT_MAD_GS  0x01
#define TFT_MAD_RGB 0x00
#define TFT_INVOFF  0x20
#define TFT_INVON   0x21

#ifdef __cplusplus
}
#endif

#endif /* ILI9486_PRIVATE_H_ */
