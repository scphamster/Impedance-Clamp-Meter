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
#include <stdint.h>
#include <memory>

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

class ILI9486Driver {
  public:
    ILI9486Driver();
    explicit ILI9486Driver(const ILI9486Driver &);
    ILI9486Driver &operator=(const ILI9486Driver &);
    explicit ILI9486Driver(ILI9486Driver &&);
    ILI9486Driver &operator=(ILI9486Driver &&);
    virtual ~ILI9486Driver();

    using PixelNumT   = uint16_t;
    using Color       = uint16_t;
    using ScreenSizeT = int;

    enum class Orientation {
        Portrait = 0,
        Landscape,
        ReversePortrait,
        ReverseLandscape
    };

    class Point {
      public:
        ScreenSizeT x;
        ScreenSizeT y;
    };

    void Clear(Color color) const noexcept;
    template<ILI9486Driver::Orientation orientation>
    constexpr void SetOrientation() const noexcept;
    void           SetCursor(Point) const noexcept;
    void           SetTextColor(Color pixelcolor, Color backcolor) const noexcept;
    void           FillScreen(Color) const noexcept;
    void           DrawPoint(Point p, Color) const noexcept;
    void           DrawLine(Point, Point, Color) const noexcept;
    void           DrawVLine(Point, ScreenSizeT h, Color) const noexcept;
    void           DrawHLine(Point, ScreenSizeT w, Color) const noexcept;
    void           DrawCircle(Point, ScreenSizeT r, Color) const noexcept;
    void           DrawFiledCircle(Point, ScreenSizeT r, Color) const noexcept;
    void           DrawRectangle(Point, ScreenSizeT w, ScreenSizeT h, Color) const noexcept;
    void           DrawFiledRectangle(Point, ScreenSizeT w, ScreenSizeT h, Color) const noexcept;
    void           Print(const char *string, uint8_t fontsize) const noexcept;
    void           Print(uint16_t number, uint8_t fontsize) const noexcept;
    void           Print(float number, uint8_t digits, uint8_t fontsize) const noexcept;

  private:
    class Impl;
    std::unique_ptr<Impl> impl;
};