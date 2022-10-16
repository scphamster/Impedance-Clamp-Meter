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

#include <cstdint>
#include <memory>
#include "compiler.h"

#define TFT_CS_PIO       PIOA
#define TFT_CMD_DATA_PIO PIOD
#define TFT_WR_PIO       PIOD
#define TFT_RESET_PIO    PIOD

class ILI9486Driver {
  public:
    using PixelNumT   = int;
    using ColorT      = uint16_t;
    using ScreenSizeT = int;

    class Point {
      public:
        ScreenSizeT x;
        ScreenSizeT y;
    };

    enum class Command : Byte {
        nop     = 0x00,
        mad_rgb = 0x00,
        mad_gs  = 0x01,
        swrst   = 0x01,
        mad_ss  = 0x02,
        mad_mh  = 0x04,
        mad_bgr = 0x08,
        mad_ml  = 0x10,
        mad_mv  = 0x20,
        invoff  = 0x20,
        invon   = 0x21,
        caset   = 0x2A,
        paset   = 0x2B,
        ramwr   = 0x2C,
        ramrd   = 0x2E,
        madctl  = 0x36,
        mad_mx  = 0x40,
        mad_my  = 0x80,
    };

    enum class Orientation {
        Portrait = 0,
        Landscape,
        ReversePortrait,
        ReverseLandscape
    };

    enum class Pin {
        ChipSelect,
        Command,
        Data,
        Write,
        Read,
        Reset
    };

    void Init() noexcept;
    void InitGPIO() const noexcept;
    template<Pin, bool to_be_set>
    constexpr inline void SetPin() const noexcept;
    void                  SetDataL(uint8_t data) const noexcept;
    void                  SetDataH(uint8_t data) const noexcept;
    void                  StrobeWritePin() const noexcept;
    void                  Reset() const noexcept;
    void                  WriteData(Byte data) const noexcept;
    void                  WriteCommand(Command cmd) const noexcept;
    void                  WriteDataDouble(uint16_t data) const noexcept;
    void                  WriteCommandDouble(uint16_t cmd) const noexcept;
    void                  WriteParameter(Command cmd, int n_bytes, Byte *data) const noexcept;
    template<Orientation orientation>
    constexpr void SetOrientation() noexcept;
    void           SetPartial(const Point, const Point) const noexcept;
    void           DrawPixel(const Point, const ColorT) const noexcept;
    void           PutPixel(const ColorT color) const noexcept;
    void           PutPixel(const ColorT color, int n) const noexcept;
    void           PrintChar(const Point at_point, Byte data, int size) const noexcept;

    ScreenSizeT screen_width, screen_height;
    ColorT      pixelColour, backColour;

  protected:
  private:
    static bool isInitialized;
    auto constexpr static font_width          = 14;
    auto constexpr static font_height         = 24;
    auto constexpr static font_one_char_bytes = 42;

    auto constexpr static tft_width  = 320;
    auto constexpr static tft_height = 480;

    auto constexpr static font_squish   = 3;
    auto constexpr static portrait      = 0x48;
    auto constexpr static landscape     = 0x28;
    auto constexpr static portrait_rev  = 0x98;
    auto constexpr static landscape_rev = 0xF4;
    // driver commands;
    //    auto constexpr static tft_nop            = 0x00;
    //    auto constexpr static tft_swrst          = 0x01;
    //    auto constexpr static tft_caset          = 0x2A;
    //    auto constexpr static tft_paset          = 0x2B;
    //    auto constexpr static tft_ramwr          = 0x2C;
    //    auto constexpr static tft_ramrd          = 0x2E;
    //    auto constexpr static tft_madctl         = 0x36;
    //    auto constexpr static tft_mad_my         = 0x80;
    //    auto constexpr static tft_mad_mx         = 0x40;
    //    auto constexpr static tft_mad_mv         = 0x20;
    //    auto constexpr static tft_mad_ml         = 0x10;
    //    auto constexpr static tft_mad_bgr        = 0x08;
    //    auto constexpr static tft_mad_mh         = 0x04;
    //    auto constexpr static tft_mad_ss         = 0x02;
    //    auto constexpr static tft_mad_gs         = 0x01;
    //    auto constexpr static tft_mad_rgb        = 0x00;
    //    auto constexpr static tft_invoff         = 0x20;
    //    auto constexpr static tft_invon          = 0x21;
    //    auto constexpr static tft_str_m_nobackgr = 0x01;
    //    auto constexpr static tft_str_m_backgr   = 0x00;

    // orientations;
    auto constexpr static tft_portrait      = 0;
    auto constexpr static tft_landscape     = 1;
    auto constexpr static tft_portrait_rev  = 2;
    auto constexpr static tft_landscape_rev = 3;

    auto constexpr static tft_cs_pin       = (1 << 0);
    auto constexpr static tft_cmd_data_pin = (1 << 29);
    auto constexpr static tft_wr_pin       = (1 << 9);
    auto constexpr static tft_reset_pin    = (1 << 15);
};
