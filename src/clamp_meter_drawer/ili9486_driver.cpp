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
#include "ili9486_driver.hpp"
#include <cstdint>
#include "ili9486_fonts.h"
#include "ILI9486_config.h"

extern "C" char *gcvtf(float, int, char *);

class ILI9486Driver {
  public:
    enum class Pin {
        ChipSelect,
        Command,
        Data,
        Write,
        Read,
        Reset
    };

    void InitGPIO() const noexcept;
    template<Pin, bool to_be_set>
    constexpr inline void SetPin() const noexcept;
    void                  SetDataL(uint8_t data) const noexcept;
    void                  SetDataH(uint8_t data) const noexcept;
    void                  StrobeWritePin() const noexcept;

  protected:
  private:
    // pins config
};

template<ILI9486Driver::Pin pin, bool to_be_set>
constexpr inline void
ILI9486Driver::SetPin() const noexcept
{
    if constexpr (pin == Pin::ChipSelect) {
        if constexpr (to_be_set) {
            pio_clear(TFT_CS_PIO, TFT_CS_PIN);
        }
        else {
            pio_set(TFT_CS_PIO, TFT_CS_PIN);
        };
    }
    else if constexpr (pin == Pin::Data) {
        pio_set(TFT_CMD_DATA_PIO, TFT_CMD_DATA_PIN);
    }
    else if constexpr (pin == Pin::Command) {
        pio_clear(TFT_CMD_DATA_PIO, TFT_CMD_DATA_PIN);
    }
    else if constexpr (pin == Pin::Write) {
        if constexpr (to_be_set) {
            pio_clear(TFT_WR_PIO, TFT_WR_PIN);
        }
        else {
            pio_set(TFT_WR_PIO, TFT_WR_PIN);
        };
    }
    else if constexpr (pin == Pin::Reset) {
        if constexpr (to_be_set) {
            pio_clear(TFT_RESET_PIO, TFT_RESET_PIN);
        }
        else {
            pio_set(TFT_RESET_PIO, TFT_RESET_PIN);
        };
    }
    else if constexpr (pin == Pin::Read) { }
}

inline void
ILI9486Driver::InitGPIO() const noexcept
{
    Matrix *p_matrix;
    p_matrix = MATRIX;

    // configure control pins
    pio_set_output(PIOD, 0x20008600ul, 0x20008600ul, 0, 0);
    pio_set_output(TFT_CS_PIO, TFT_CS_PIN, TFT_CS_PIN, 0, 0);

    // configure data pins
    p_matrix->CCFG_SYSIO |= CCFG_SYSIO_SYSIO10 | CCFG_SYSIO_SYSIO11;

    pio_set_output(PIOA, 0xF0000040ul, 0, 0, 0);
    pio_set_output(PIOB, 0x4C00ul, 0, 0, 0);
    pio_set_output(PIOD, 0x1FEul, 0, 0, 0);
}

inline void
ILI9486Driver::SetDataL(uint8_t data) const noexcept
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

inline void
ILI9486Driver::SetDataH(uint8_t data) const noexcept
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

inline void
ILI9486Driver::StrobeWritePin() const noexcept
{
    SetPin<Pin::Write, true>();
    SetPin<Pin::Write, false>();
}

class DisplayDrawer::Impl : private ILI9486Driver {
  public:
    void Init() noexcept;
    void Reset() const noexcept;
    void Clear(const Color color) const noexcept;

    template<Orientation>
    constexpr void SetOrientation() noexcept;
    void           SetCursor(const Point) noexcept;
    void           SetTextColor(const Color pixelcolor, const Color backcolor) noexcept;

    void FillScreen(const Color) const noexcept;
    void DrawPoint(const Point, const Color) const noexcept;
    void DrawLine(Point, const Point, const Color) const noexcept;
    void DrawVLine(const Point, int h, const Color) const noexcept;
    void DrawHLine(const Point, ScreenSizeT w, const Color) const noexcept;
    void DrawCircle(const Point, ScreenSizeT r, const Color) const noexcept;
    void DrawFiledCircle(const Point, ScreenSizeT r, const Color) const noexcept;
    void DrawRectangle(const Point, ScreenSizeT w, ScreenSizeT h, const Color) const noexcept;
    void DrawFiledRectangle(Point, ScreenSizeT w, ScreenSizeT h, const Color) const noexcept;
    void Print(const char *string, uint8_t size) noexcept;
    void Print(int number, uint8_t size) noexcept;
    void Print(float number, uint8_t n_digits, uint8_t size) noexcept;

  protected:
    void WriteData(Byte data) const noexcept;
    void WriteCommand(Byte cmd) const noexcept;
    void WriteDataDouble(uint16_t data) const noexcept;
    void WriteCommandDouble(uint16_t cmd) const noexcept;

    void SetPartial(const Point, const Point) const noexcept;

    void WriteParameter(Byte cmd, int n_bytes, Byte *data) const noexcept;
    void DrawPixel(const Point, const Color) const noexcept;
    void PutPixel(const Color) const noexcept;
    void PutPixel(const Color, uint64_t n) const noexcept;

    void PrintChar(const Point point, Byte data, int size) const noexcept;
    void DrawFiledCircleHelper(const Point, ScreenSizeT r, uint8_t cornername, int delta, const Color) const noexcept;

  private:
    ScreenSizeT m_screen_width, m_screen_height;
    Point     cursorPosition;
    uint16_t  pixelColour, backColour;
};

inline void
DisplayDrawer::Impl::SetCursor(const Point point) noexcept
{
    cursorPosition = point;
}

void
DisplayDrawer::Impl::Reset() const noexcept
{
    SetPin<Pin::ChipSelect, false>();
    // TFT_CTRL_NOREAD;
    SetPin<Pin::Write, false>();

    SetPin<Pin::Reset, false>();

    delay_ms(50);
    SetPin<Pin::Reset, true>();
    delay_ms(100);

    SetPin<Pin::Reset, false>();
    delay_ms(100);
}

inline void
DisplayDrawer::Impl::Clear(const Color color) const noexcept
{
    FillScreen(color);
}

inline void
DisplayDrawer::Impl::WriteData(uint8_t data) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Data, true>();

    SetDataL(data);
    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

inline void
DisplayDrawer::Impl::WriteCommand(uint8_t cmd) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Command, true>();

    SetDataL(cmd);
    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

inline void
DisplayDrawer::Impl::WriteDataDouble(uint16_t data) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Data, true>();

    SetDataL(data >> 8);
    StrobeWritePin();

    SetDataL(data & 0xff);
    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

inline void
DisplayDrawer::Impl::WriteCommandDouble(uint16_t cmd) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Command, true>();

    SetDataL(cmd >> 8);
    StrobeWritePin();
    SetDataL(cmd & 0xff);
    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

inline void
DisplayDrawer::Impl::PutPixel(const Color color) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Data, true>();

    SetDataL(color & 0xff);
    SetDataH((color >> 8) & 0xff);

    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

inline void
DisplayDrawer::Impl::PutPixel(const Color color, uint64_t n) const noexcept
{
    WriteCommand(TFT_RAMWR);

    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Data, true>();

    SetDataL(color & 0xff);
    SetDataH((color >> 8) & 0xff);

    while (n) {
        StrobeWritePin();
        n--;
    }

    SetPin<Pin::ChipSelect, false>();
}

inline void
DisplayDrawer::Impl::WriteParameter(Byte cmd, int n_bytes, Byte *data) const noexcept
{
    WriteCommand(cmd);

    while (n_bytes--) {
        WriteData(*data++);
    }
}

inline void
DisplayDrawer::Impl::SetPartial(const Point point_beg, const Point point_end) const noexcept
{
    uint8_t caset_data[4] = { static_cast<uint8_t>(point_beg.x >> 8),
                              static_cast<uint8_t>(point_beg.x),
                              static_cast<uint8_t>(point_end.x >> 8),
                              static_cast<uint8_t>(point_end.x) };

    uint8_t paset_data[4] = { static_cast<uint8_t>(point_beg.y >> 8),
                              static_cast<uint8_t>(point_beg.y),
                              static_cast<uint8_t>(point_end.y >> 8),
                              static_cast<uint8_t>(point_end.y) };

    WriteParameter(TFT_CASET, 4, caset_data);
    WriteParameter(TFT_PASET, 4, paset_data);
}

void
DisplayDrawer::Impl::PrintChar(const Point point, Byte data, int size) const noexcept
{
    if ((point.x > m_screen_width - FONT_WIDTH * size) || (point.y > m_screen_height - FONT_HEIGHT * size))
        return;

    uint8_t byte_row, bit_row, column, temp;
    uint8_t bit_row_counter   = 0;
    bool    char_color_is_set = false;
    uint8_t counter           = 0;
    uint8_t data_from_memory[FONT_ONE_CHAR_BYTES];

    SetPartial(point, Point{ point.x + FONT_WIDTH * size - 1, point.y + FONT_HEIGHT * size - 1 });
    WriteCommand(TFT_RAMWR);

    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Data, true>();

    SetDataL(0xff & backColour);
    SetDataH((backColour >> 8) && 0Xff);

    for (; counter < FONT_ONE_CHAR_BYTES; counter++)
        data_from_memory[counter] = Consolas14x24[(data - ' ') * FONT_ONE_CHAR_BYTES + counter];

    for (byte_row = 0; byte_row < 3; byte_row++) {
        for (bit_row = 0; bit_row < 8; bit_row++) {
            for (column = 0; column < FONT_WIDTH; column++) {
                temp = data_from_memory[column * 3 + byte_row];

                if (temp & (1 << bit_row)) {
                    if (char_color_is_set) {
                        StrobeWritePin();

                        if (2 == size)
                            StrobeWritePin();
                    }
                    else {
                        char_color_is_set = true;
                        SetDataL(0xff & pixelColour);
                        SetDataH((pixelColour >> 8) & 0xff);
                        StrobeWritePin();

                        if (2 == size)
                            StrobeWritePin();
                    }
                }
                else {
                    if (char_color_is_set) {
                        char_color_is_set = false;
                        SetDataL(0xff & backColour);
                        SetDataH((backColour >> 8) & 0xff);
                        StrobeWritePin();

                        if (2 == size)
                            StrobeWritePin();
                    }
                    else {
                        StrobeWritePin();

                        if (2 == size)
                            StrobeWritePin();
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

    SetPin<Pin::ChipSelect, false>();
}

void
DisplayDrawer::Impl::DrawFiledCircleHelper(const Point point,
                                           ScreenSizeT r,
                                           uint8_t     cornername,
                                           int     delta,
                                           const Color color) const noexcept
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
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        if (cornername & 0x1) {
            DrawVLine(Point{ point.x + x, point.y - y }, 2 * y + 1 + delta, color);
            DrawVLine(Point{ point.x + y, point.y - x }, 2 * x + 1 + delta, color);
        }

        if (cornername & 0x2) {
            DrawVLine(Point{ point.x - x, point.y - y }, 2 * y + 1 + delta, color);
            DrawVLine(Point{ point.x - y, point.y - x }, 2 * x + 1 + delta, color);
        }
    }
}

template<DisplayDrawer::Orientation orientation>
constexpr void
DisplayDrawer::Impl::SetOrientation() noexcept
{
    WriteCommand(TFT_MADCTL);

    if constexpr (orientation == DisplayDrawer::Orientation::Portrait) {
        WriteData(0x48);   // 0b 0100 1000;
        m_screen_width  = TFT_WIDTH;
        m_screen_height = TFT_HEIGHT;
    }

    if constexpr (orientation == DisplayDrawer::Orientation::Landscape) {   // 0b 0010 1000;
        WriteData(0x28);
        m_screen_width  = TFT_HEIGHT;
        m_screen_height = TFT_WIDTH;
    }

    if constexpr (orientation == DisplayDrawer::Orientation::ReversePortrait) {
        WriteData(0x98);   // 0b 1001 1000;
        m_screen_width  = TFT_WIDTH;
        m_screen_height = TFT_HEIGHT;
    }

    if constexpr (orientation == DisplayDrawer::Orientation::ReverseLandscape) {
        WriteData(0xF8);
        m_screen_width  = TFT_HEIGHT;
        m_screen_height = TFT_WIDTH;
    }
}

inline void
DisplayDrawer::Impl::SetTextColor(Color pixelcolor, Color backcolor) noexcept
{
    pixelColour = pixelcolor;
    backColour  = backcolor;
}

inline void
DisplayDrawer::Impl::FillScreen(const Color color) const noexcept
{
    DrawFiledRectangle(Point{ 0, 0 }, m_screen_width, m_screen_height, color);
}

inline void
DisplayDrawer::Impl::DrawPoint(const DisplayDrawer::Point point, const DisplayDrawer::Color color) const noexcept
{
    ScreenSizeT r = 1;

    DrawVLine(Point{ point.x, point.y - r }, 2 * r + 1, color);
    DrawFiledCircleHelper(point, r, 3, 0, color);
}

inline void
DisplayDrawer::Impl::DrawLine(Point point_beg, const Point point_end, const DisplayDrawer::Color color) const noexcept
{
    int16_t i;
    int16_t dx, dy;
    int16_t sx, sy;
    int16_t E;

    /* distance between two points */
    if (point_end.x > point_beg.x)
        dx = point_end.x - point_beg.x;
    else
        dx = point_beg.x - point_end.x;

    if (point_end.y > point_beg.y)
        dy = point_end.y - point_beg.y;
    else
        dy = point_beg.y - point_end.y;

    /* direction of two point */

    if (point_end.x > point_beg.x)
        sx = 1;
    else
        sx = -1;

    if (point_end.y > point_beg.y)
        sy = 1;
    else
        sy = -1;

    if (point_beg.y == point_end.y) {
        if (point_end.x > point_beg.x)
            DrawHLine(point_beg, point_end.x - point_beg.x + 1, color);
        else
            DrawHLine(Point{ point_end.x, point_beg.y }, point_beg.x - point_end.x + 1, color);
        return;
    }
    else if (point_beg.x == point_end.x) {
        if (point_end.y > point_beg.y)
            DrawVLine(point_beg, point_end.y - point_beg.y + 1, color);
        else
            DrawVLine(point_beg, point_beg.y - point_end.y + 1, color);

        return;
    }

    /* inclination < 1 */
    if (dx > dy) {
        E = -dx;

        for (i = 0; i <= dx; i++) {
            DrawPixel(point_beg, color);
            point_beg.x += sx;
            E += 2 * dy;

            if (E >= 0) {
                point_beg.y += sy;
                E -= 2 * dx;
            }
        }

        /* inclination >= 1 */
    }
    else {
        E = -dy;

        for (i = 0; i <= dy; i++) {
            DrawPixel(point_beg, color);
            point_beg.y += sy;
            E += 2 * dx;

            if (E >= 0) {
                point_beg.x += sx;
                E -= 2 * dy;
            }
        }
    }
}

inline void
DisplayDrawer::Impl::DrawPixel(const Point point, const Color color) const noexcept
{
    if (point.x < 0 || point.y < 0 || point.x >= m_screen_width || point.y >= m_screen_height)
        return;

    uint8_t caset_data[4] = { static_cast<uint8_t>(point.x >> 8),
                              static_cast<uint8_t>(point.x),
                              static_cast<uint8_t>(point.x >> 8),
                              static_cast<uint8_t>(point.x) };

    uint8_t paset_data[4] = { static_cast<uint8_t>(point.y >> 8),
                              static_cast<uint8_t>(point.y),
                              static_cast<uint8_t>(point.y >> 8),
                              static_cast<uint8_t>(point.y) };

    WriteParameter(TFT_CASET, 4, caset_data);
    WriteParameter(TFT_PASET, 4, paset_data);

    WriteCommand(TFT_RAMWR);
    PutPixel(color);
}

void
DisplayDrawer::Impl::DrawVLine(const Point point, ScreenSizeT h, const DisplayDrawer::Color color) const noexcept
{
    if ((point.x >= m_screen_width) || (point.y >= m_screen_height || h < 1))
        return;

    if ((point.y + h - 1) >= m_screen_height)
        h = m_screen_height - point.y;

    DrawFiledRectangle(point, 1, h, color);
}

void
DisplayDrawer::Impl::DrawHLine(const Point point, ScreenSizeT w, DisplayDrawer::Color color) const noexcept
{
    if ((point.x >= m_screen_width) || (point.y >= m_screen_height || w < 1))
        return;

    if ((point.x + w - 1) >= m_screen_width)
        w = m_screen_width - point.x;

    DrawFiledRectangle(point, w, 1, color);
}
void
DisplayDrawer::Impl::DrawCircle(const Point point, ScreenSizeT r, DisplayDrawer::Color color) const noexcept
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    DrawPixel(Point{ point.x, point.y + r }, color);
    DrawPixel(Point{ point.x, point.y - r }, color);
    DrawPixel(Point{ point.x + r, point.y }, color);
    DrawPixel(Point{ point.x - r, point.y }, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        DrawPixel(Point{ point.x + x, point.y + y }, color);
        DrawPixel(Point{ point.x - x, point.y + y }, color);
        DrawPixel(Point{ point.x + x, point.y - y }, color);
        DrawPixel(Point{ point.x - x, point.y - y }, color);
        DrawPixel(Point{ point.x + y, point.y + x }, color);
        DrawPixel(Point{ point.x - y, point.y + x }, color);
        DrawPixel(Point{ point.x + y, point.y - x }, color);
        DrawPixel(Point{ point.x - y, point.y - x }, color);
    }
}

void
DisplayDrawer::Impl::DrawFiledCircle(const Point point, ScreenSizeT r, DisplayDrawer::Color color) const noexcept
{
    DrawVLine(Point{ point.x, point.y - r }, 2 * r + 1, color);
    DrawFiledCircleHelper(Point{ point.x, point.y }, r, 3, 0, color);
}
inline void
DisplayDrawer::Impl::DrawRectangle(const Point          point,
                                   ScreenSizeT          w,
                                   ScreenSizeT          h,
                                   DisplayDrawer::Color color) const noexcept
{
    DrawHLine(point, w, color);
    DrawHLine(Point{ point.x, point.y + h - 1 }, w, color);
    DrawVLine(point, h, color);
    DrawVLine(Point{ point.x + w - 1, point.y }, h, color);
}
void
DisplayDrawer::Impl::DrawFiledRectangle(Point                point,
                                        ScreenSizeT          w,
                                        ScreenSizeT          h,
                                        DisplayDrawer::Color color) const noexcept
{
    int16_t  end;
    uint32_t n_pixels;

    if (w < 0) {
        w = -w;
        point.x -= w;
    }

    end = point.x + w;

    if (point.x < 0)
        point.x = 0;

    if (end > m_screen_width)
        end = m_screen_width;

    w = end - point.x;

    if (h < 0) {
        h = -h;
        point.y -= h;
    }

    end = point.y + h;

    if (point.y < 0)
        point.y = 0;

    if (end > m_screen_height)
        end = m_screen_height;

    h        = end - point.y;
    n_pixels = (uint32_t)h * w;

    SetPartial(point, Point{ point.x + w - 1, point.y + h - 1 });
    PutPixel(color, n_pixels);
}
void
DisplayDrawer::Impl::Print(const char *string, uint8_t size) noexcept
{
    uint8_t i      = 0;
    uint8_t font_w = (FONT_WIDTH - FONT_SQUISH) * size;
    uint8_t font_h = (FONT_HEIGHT - FONT_SQUISH) * size;

    while ((*(string + i) != '\0') && (*string)) {
        if (*(string + i) == '\n') {
            cursorPosition.x += font_h;
            cursorPosition.x = 0;
            string++;
        }

        if (cursorPosition.x > m_screen_width - font_w) {
            cursorPosition.x = 0;
            cursorPosition.y += font_h;
        }

        if (cursorPosition.y > m_screen_height - font_h)
            cursorPosition.y = cursorPosition.x = 0;

        PrintChar(Point{ cursorPosition }, *(string + i), size);
        cursorPosition.x += font_w;
        i++;
    }
}
void
DisplayDrawer::Impl::Print(int number, uint8_t size) noexcept
{
    int temp = 1;

    if (number <= 0) {
        PrintChar(cursorPosition, '0', size);
        cursorPosition.x += 14 * size;
    }
    else {
        while (number != 0) {
            uint8_t remainder = number % 10;
            number            = number / 10;
            temp              = temp * 10 + remainder;
        }

        while (temp != 1) {
            uint8_t remainder = temp % 10;
            temp              = temp / 10;
            PrintChar(cursorPosition, remainder + 48, size);
            cursorPosition.x += 14 * size;
        }
    }
}
void
DisplayDrawer::Impl::Print(float number, uint8_t n_digits, uint8_t size) noexcept
{
    static char str[20];

    //    snprintf(str, sizeof(str), "%f", value);

    gcvtf(number, n_digits, str);

    Print(str, size);
}
void
DisplayDrawer::Impl::Init() noexcept
{
    InitGPIO();
    Reset();

    WriteCommand(0x11);   // Sleep out, also SW reset
    delay_ms(60);

    WriteCommand(0x3A);   // interface pixel format
    WriteData(0x55);      // 16bit RGB & 16bit CPU

    WriteCommand(0xC2);   // Power Control 3 Normal mode
    WriteData(0x44);

    WriteCommand(0xC5);   // vcom control
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);
    WriteData(0x00);

    WriteCommand(0xE0);   // Positive gamma CTRL
    WriteData(0x0F);
    WriteData(0x1F);
    WriteData(0x1C);
    WriteData(0x0C);
    WriteData(0x0F);
    WriteData(0x08);
    WriteData(0x48);
    WriteData(0x98);
    WriteData(0x37);
    WriteData(0x0A);
    WriteData(0x13);
    WriteData(0x04);
    WriteData(0x11);
    WriteData(0x0D);
    WriteData(0x00);

    WriteCommand(0xE1);   // Negative gamma CTRL
    WriteData(0x0F);
    WriteData(0x32);
    WriteData(0x2E);
    WriteData(0x0B);
    WriteData(0x0D);
    WriteData(0x05);
    WriteData(0x47);
    WriteData(0x75);
    WriteData(0x37);
    WriteData(0x06);
    WriteData(0x10);
    WriteData(0x03);
    WriteData(0x24);
    WriteData(0x20);
    WriteData(0x00);

    WriteCommand(0x20);   // display inversion OFF

    WriteCommand(0x36);      // memory access CTRL
    WriteData(0b01001001);   // MY MX MV ML BGR MH X X

    WriteCommand(0x29);   // display on
    delay_ms(60);

    SetOrientation<Orientation::Portrait>();
}

DisplayDrawer::DisplayDrawer()
  : impl{ std::make_unique<Impl>() }
{
    impl->Init();
    impl->Clear(COLOR_BLACK);
}

DisplayDrawer::DisplayDrawer(const DisplayDrawer &other)
  : impl{ std::make_unique<Impl>(*other.impl) }
{ }

DisplayDrawer &
DisplayDrawer::operator=(const DisplayDrawer &rhs)
{
    impl = std::make_unique<Impl>(*rhs.impl);
    return *this;
}

DisplayDrawer::~DisplayDrawer()                           = default;
DisplayDrawer &DisplayDrawer::operator=(DisplayDrawer &&) = default;
DisplayDrawer::DisplayDrawer(DisplayDrawer &&)            = default;

inline void
DisplayDrawer::Clear(const Color color) const noexcept
{
    impl->Clear(color);
}

template<DisplayDrawer::Orientation orientation>
constexpr void
DisplayDrawer::SetOrientation() const noexcept
{
    impl->SetOrientation<orientation>();
}

void
DisplayDrawer::SetTextColor(const Color pixelcolor, const Color backcolor) const noexcept
{
    impl->SetTextColor(pixelcolor, backcolor);
}
void
DisplayDrawer::SetCursor(const Point point) const noexcept
{
    impl->SetCursor(point);
}
void
DisplayDrawer::FillScreen(DisplayDrawer::Color color) const noexcept
{
    impl->FillScreen(color);
}
void
DisplayDrawer::DrawPoint(const DisplayDrawer::Point p, const DisplayDrawer::Color color) const noexcept
{
    impl->DrawPoint(p, color);
}
void
DisplayDrawer::DrawLine(Point point_beg, const Point point_end, const DisplayDrawer::Color color) const noexcept
{
    impl->DrawLine(point_beg, point_end, color);
}
void
DisplayDrawer::DrawVLine(const Point point, const ScreenSizeT h, const DisplayDrawer::Color color) const noexcept
{
    impl->DrawVLine(point, h, color);
}
void
DisplayDrawer::DrawHLine(const Point point, const ScreenSizeT w, const DisplayDrawer::Color color) const noexcept
{
    impl->DrawHLine(point, w, color);
}
void
DisplayDrawer::DrawCircle(const Point point, const ScreenSizeT r, const DisplayDrawer::Color color) const noexcept
{
    impl->DrawCircle(point, r, color);
}
void
DisplayDrawer::DrawFiledCircle(const Point point, const ScreenSizeT r, const DisplayDrawer::Color color) const noexcept
{
    impl->DrawFiledCircle(point, r, color);
}
void
DisplayDrawer::DrawRectangle(const Point                point,
                             const ScreenSizeT          w,
                             const ScreenSizeT          h,
                             const DisplayDrawer::Color color) const noexcept
{
    impl->DrawRectangle(point, w, h, color);
}
void
DisplayDrawer::DrawFiledRectangle(const Point                point,
                                  const ScreenSizeT          w,
                                  const ScreenSizeT          h,
                                  const DisplayDrawer::Color color) const noexcept
{
    impl->DrawFiledRectangle(point, w, h, color);
}
void
DisplayDrawer::Print(const char *string, const uint8_t fontsize) const noexcept
{
    impl->Print(string, fontsize);
}
void
DisplayDrawer::Print(int number, const uint8_t fontsize) const noexcept
{
    impl->Print(number, fontsize);
}
void
DisplayDrawer::Print(float number, uint8_t digits, const uint8_t fontsize) const noexcept
{
    impl->Print(number, digits, fontsize);
}
