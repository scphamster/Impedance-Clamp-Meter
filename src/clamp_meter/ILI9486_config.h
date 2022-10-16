#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define TFT_WIDTH   320
#define TFT_HEIGHT  480
#define FONT_HEIGHT 24
#define FONT_WIDTH  14
#define FONT_SQUISH 3

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

#define TFT_STR_M_NOBACKGR 0x01
#define TFT_STR_M_BACKGR   0x00

#define COLOR_BLACK        0x0000 /*   0,   0,   0 */
#define COLOR_NAVY         0x000F /*   0,   0, 128 */
#define COLOR_DARKGREEN    0x03E0 /*   0, 128,   0 */
#define COLOR_DDGREEN      0xc0
#define COLOR_DARKCYAN     0x03EF /*   0, 128, 128 */
#define COLOR_MAROON       0x7800 /* 128,   0,   0 */
#define COLOR_PURPLE       0x780F /* 128,   0, 128 */
#define COLOR_OLIVE        0x7BE0 /* 128, 128,   0 */
#define COLOR_LIGHTGREY    0xC618 /* 192, 192, 192 */
#define COLOR_DARKGREY     0x7BEF /* 128, 128, 128 */
#define COLOR_BLUE         0x001F /*   0,   0, 255 */
#define COLOR_GREEN        0x07E0 /*   0, 255,   0 */
#define COLOR_CYAN         0x07FF /*   0, 255, 255 */
#define COLOR_RED          0xF800 /* 255,   0,   0 */
#define COLOR_MAGENTA      0xF81F /* 255,   0, 255 */
#define COLOR_YELLOW       0xFFE0 /* 255, 255,   0 */
#define COLOR_WHITE        0xFFFF /* 255, 255, 255 */
#define COLOR_ORANGE       0xFD20 /* 255, 165,   0 */
#define COLOR_GREENYELLOW  0xAFE5 /* 173, 255,  47 */
#define COLOR_PINK         0xF81F
#define COLOR_DARKDARKGREY 0x514A   // 10,10,10

#define COLOR_REDYELLOW 60580
#define COLOR_LIGHTMUD  42023
#define COLOR_GREY      10565

// screen orientations
#define TFT_PORTRAIT      0
#define TFT_LANDSCAPE     1
#define TFT_PORTRAIT_REV  2
#define TFT_LANDSCAPE_REV 3


#define TFT_CS_PIO       PIOA
#define TFT_CS_PIN       (1 << 0)
#define TFT_CMD_DATA_PIO PIOD
#define TFT_CMD_DATA_PIN (1 << 29)
#define TFT_WR_PIO       PIOD
#define TFT_WR_PIN       (1 << 9)
#define TFT_RESET_PIO    PIOD
#define TFT_RESET_PIN    (1 << 15)
#ifdef __cplusplus
}
#endif
