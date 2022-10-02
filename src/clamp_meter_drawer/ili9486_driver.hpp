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

#include "ILI9486_config.h"

class ILI9486Driver {
  public:
    using PixelNumT   = int;
    using Color       = uint16_t;
    using ScreenSizeT = int;

    class Point {
      public:
        ScreenSizeT x;
        ScreenSizeT y;
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
    void                  WriteCommand(Byte cmd) const noexcept;
    void                  WriteDataDouble(uint16_t data) const noexcept;
    void                  WriteCommandDouble(uint16_t cmd) const noexcept;

    template<Orientation orientation>
    constexpr void SetOrientation() noexcept;

    void SetPartial(const Point, const Point) const noexcept;

    void WriteParameter(Byte cmd, int n_bytes, Byte *data) const noexcept;
    void DrawPixel(const Point, const Color) const noexcept;
    void PutPixel(const Color color) const noexcept;

    void PutPixel(const Color color, int n) const noexcept;

    void PrintChar(const Point point, Byte data, int size) const noexcept;

    ScreenSizeT m_screen_width, m_screen_height;
    uint16_t    pixelColour, backColour;

  protected:
  private:
    static bool isInitialized;
};

