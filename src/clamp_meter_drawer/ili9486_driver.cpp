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
// #include "ILI9486_public.h"
// #include "ILI9486_private.h"
#include <cstdint>
#include "ili9486_fonts.h"
#include "ILI9486_config.h"

extern "C" char *gcvtf(float, int, char *);

class ILIDrawerIO {
  public:
    enum class Pin {
        ChipSelect,
        Command,
        Data,
        Write,
        Read,
        Reset
    };

    void           InitGPIO();
    constexpr void SetPin(Pin, bool to_be_set);
    void           SetData1(uint8_t data);
    void           SetData2(uint8_t data);
    void           StrobeWritePin();

  protected:
  private:
    // pins config
};

constexpr void
ILIDrawerIO::SetPin(Pin pin, bool to_be_set)
{
    switch (pin) {
    case Pin::ChipSelect:
        if (to_be_set) {
            pio_clear(TFT_CS_PIO, TFT_CS_PIN);
        }
        else {
            pio_set(TFT_CS_PIO, TFT_CS_PIN);
        };
        break;
    case Pin::Data: pio_set(TFT_CMD_DATA_PIO, TFT_CMD_DATA_PIN); break;
    case Pin::Command: pio_clear(TFT_CMD_DATA_PIO, TFT_CMD_DATA_PIN); break;
    case Pin::Write:
        if (to_be_set) {
            pio_clear(TFT_WR_PIO, TFT_WR_PIN);
        }
        else {
            pio_set(TFT_WR_PIO, TFT_WR_PIN);
        };
        break;

    case Pin::Reset:
        if (to_be_set) {
            pio_clear(TFT_RESET_PIO, TFT_RESET_PIN);
        }
        else {
            pio_set(TFT_RESET_PIO, TFT_RESET_PIN);
        };
        break;
    case Pin::Read: break;
    }
}

void
ILIDrawerIO::InitGPIO()
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

void
ILIDrawerIO::SetData1(uint8_t data)
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

void
ILIDrawerIO::SetData2(uint8_t data)
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

void
ILIDrawerIO::StrobeWritePin()
{
    SetPin(Pin::Write, true);
    SetPin(Pin::Write, false);
}

class ILIDrawer::Impl : private ILIDrawerIO {
  public:
    void Init();
    void Reset();
    void Clear(ColorT color);
    void SetRotation(uint8_t rotation);
    void SetCursor(PixelNumT x, PixelNumT y) noexcept;
    void SetTextColor(ColorT pixelcolor, ColorT backcolor);

    void FillScreen(ColorT color);
    void DrawPoint(int16_t x0, int16_t y0, ColorT color);
    void DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, ColorT color);
    void DrawVLine(int16_t x, int16_t y, int16_t h, ColorT color);
    void DrawHLine(int16_t x, int16_t y, int16_t w, ColorT color);
    void DrawCircle(int16_t x0, int16_t y0, int16_t r, ColorT color);
    void DrawFiledCircle(int16_t x0, int16_t y0, int16_t r, ColorT color);
    void DrawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, ColorT color);
    void DrawFiledRectangle(int16_t x, int16_t y, int16_t w, int16_t h, ColorT color);
    void Print(const char *string, uint8_t TFT_STRING_MODE, uint8_t size);
    void Print(uint16_t number, uint8_t string_mode, uint8_t size);
    void Print(float number, uint8_t string_mode, uint8_t size);

  protected:
    void WriteData(uint8_t data);
    void WriteCommand(uint8_t cmd);
    void WriteDataDouble(uint16_t data);
    void WriteCommandDouble(uint16_t cmd);

    void SetPartial(int16_t x_beg, int16_t y_beg, int16_t x_end, int16_t y_end);

    void WriteParameter(uint8_t cmd, int8_t n_bytes, uint8_t *data);
    void DrawPixel(uint16_t x, uint16_t y, ColorT color);
    void PutPixel(ColorT color);
    void PutPixel(ColorT color, uint64_t n);

    void PrintChar(uint16_t x, uint16_t y, char data, uint8_t mode, uint8_t size);
    void DrawFiledCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, ColorT color);

  private:
    uint16_t  m_screen_width;
    uint16_t  m_screen_height;
    PixelNumT m_cursor_x, m_cursor_y;
    uint16_t  pixelColour, backColour;
};

void
ILIDrawer::Impl::SetCursor(ILIDrawer::PixelNumT x, ILIDrawer::PixelNumT y) noexcept
{
    m_cursor_x = x;
    m_cursor_y = y;
}

void
ILIDrawer::Impl::Reset()
{
    SetPin(Pin::ChipSelect, false);
    // TFT_CTRL_NOREAD;
    SetPin(Pin::Write, false);

    SetPin(Pin::Reset, false);

    delay_ms(50);
    SetPin(Pin::Reset, true);
    delay_ms(100);

    SetPin(Pin::Reset, false);
    delay_ms(100);
}

void
ILIDrawer::Impl::Clear(ColorT color)
{
    FillScreen(color);
}

void
ILIDrawer::Impl::WriteData(uint8_t data)
{
    SetPin(Pin::ChipSelect, true);
    SetPin(Pin::Data, true);

    SetData1(data);
    StrobeWritePin();

    SetPin(Pin::ChipSelect, false);
}

void
ILIDrawer::Impl::WriteCommand(uint8_t cmd)
{
    SetPin(Pin::ChipSelect, true);
    SetPin(Pin::Command, true);

    SetData1(cmd);
    StrobeWritePin();

    SetPin(Pin::ChipSelect, false);
}

void
ILIDrawer::Impl::WriteDataDouble(uint16_t data)
{
    SetPin(Pin::ChipSelect, true);
    SetPin(Pin::Data, true);

    SetData1(data >> 8);
    StrobeWritePin();

    SetData1(data & 0xff);
    StrobeWritePin();

    SetPin(Pin::ChipSelect, false);
}

void
ILIDrawer::Impl::WriteCommandDouble(uint16_t cmd)
{
    SetPin(Pin::ChipSelect, true);
    SetPin(Pin::Command, true);

    SetData1(cmd >> 8);
    StrobeWritePin();
    SetData1(cmd & 0xff);
    StrobeWritePin();

    SetPin(Pin::ChipSelect, false);
}

void
ILIDrawer::Impl::PutPixel(ColorT color)
{
    SetPin(Pin::ChipSelect, true);
    SetPin(Pin::Data, true);

    SetData1(color & 0xff);
    SetData2((color >> 8) & 0xff);

    StrobeWritePin();

    SetPin(Pin::ChipSelect, false);
}

void
ILIDrawer::Impl::PutPixel(ColorT color, uint64_t n)
{
    WriteCommand(TFT_RAMWR);

    SetPin(Pin::ChipSelect, true);
    SetPin(Pin::Data, true);

    SetData1(color & 0xff);
    SetData2((color >> 8) & 0xff);

    while (n) {
        StrobeWritePin();
        n--;
    }

    SetPin(Pin::ChipSelect, false);
}

void
ILIDrawer::Impl::WriteParameter(uint8_t cmd, int8_t n_bytes, uint8_t *data)
{
    WriteCommand(cmd);

    while (n_bytes--) {
        WriteData(*data++);
    }
}

void
ILIDrawer::Impl::SetPartial(int16_t x_beg, int16_t y_beg, int16_t x_end, int16_t y_end)
{
    uint8_t caset_data[4] = { static_cast<uint8_t>(x_beg >> 8),
                              static_cast<uint8_t>(x_beg),
                              static_cast<uint8_t>(x_end >> 8),
                              static_cast<uint8_t>(x_end) };

    uint8_t paset_data[4] = { static_cast<uint8_t>(y_beg >> 8),
                              static_cast<uint8_t>(y_beg),
                              static_cast<uint8_t>(y_end >> 8),
                              static_cast<uint8_t>(y_end) };

    WriteParameter(TFT_CASET, 4, caset_data);
    WriteParameter(TFT_PASET, 4, paset_data);
}

void
ILIDrawer::Impl::PrintChar(uint16_t x, uint16_t y, char data, uint8_t mode, uint8_t size)
{
    if ((x > m_screen_width - FONT_WIDTH * size) || (y > m_screen_height - FONT_HEIGHT * size))
        return;

    uint8_t byte_row, bit_row, column, temp;
    uint8_t bit_row_counter   = 0;
    bool    char_color_is_set = false;
    uint8_t counter           = 0;
    uint8_t data_from_memory[FONT_ONE_CHAR_BYTES];

    SetPartial(x, y, x + FONT_WIDTH * size - 1, y + FONT_HEIGHT * size - 1);
    WriteCommand(TFT_RAMWR);

    SetPin(Pin::ChipSelect, true);
    SetPin(Pin::Data, true);
    SetData1(0xff & backColour);
    SetData2((backColour >> 8) && 0Xff);

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
                        SetData1(0xff & pixelColour);
                        SetData2((pixelColour >> 8) & 0xff);
                        StrobeWritePin();

                        if (2 == size)
                            StrobeWritePin();
                    }
                }
                else {
                    if (char_color_is_set) {
                        char_color_is_set = false;
                        SetData1(0xff & backColour);
                        SetData2((backColour >> 8) & 0xff);
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

    SetPin(Pin::ChipSelect, false);
}

void
ILIDrawer::Impl::DrawFiledCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, ColorT color)
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
            DrawVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
            DrawVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
        }

        if (cornername & 0x2) {
            DrawVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
            DrawVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
        }
    }
}

void
ILIDrawer::Impl::SetRotation(uint8_t rotation)
{
    WriteCommand(TFT_MADCTL);

    if (rotation == 0) {
        WriteData(0x48);   // 0b 0100 1000;
        m_screen_width  = TFT_WIDTH;
        m_screen_height = TFT_HEIGHT;
    }

    if (rotation == 1) {   // 0b 0010 1000;
        WriteData(0x28);
        m_screen_width  = TFT_HEIGHT;
        m_screen_height = TFT_WIDTH;
    }

    if (rotation == 2) {
        WriteData(0x98);   // 0b 1001 1000;
        m_screen_width  = TFT_WIDTH;
        m_screen_height = TFT_HEIGHT;
    }

    if (rotation == 3) {
        WriteData(0xF8);
        m_screen_width  = TFT_HEIGHT;
        m_screen_height = TFT_WIDTH;
    }
}

void
ILIDrawer::Impl::SetTextColor(uint16_t pixelcolor, uint16_t backcolor)
{
    pixelColour = pixelcolor;
    backColour  = backcolor;
}

void
ILIDrawer::Impl::FillScreen(ColorT color)
{
    DrawFiledRectangle(0, 0, m_screen_width, m_screen_height, color);
}

void
ILIDrawer::Impl::DrawPoint(int16_t x0, int16_t y0, ILIDrawer::ColorT color)
{
    int16_t r = 1;
    DrawVLine(x0, y0 - r, 2 * r + 1, color);
    DrawFiledCircleHelper(x0, y0, r, 3, 0, color);
}

void
ILIDrawer::Impl::DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, ILIDrawer::ColorT color)
{
    int16_t i;
    int16_t dx, dy;
    int16_t sx, sy;
    int16_t E;

    /* distance between two points */
    if (x2 > x1)
        dx = x2 - x1;
    else
        dx = x1 - x2;

    if (y2 > y1)
        dy = y2 - y1;
    else
        dy = y1 - y2;

    /* direction of two point */

    if (x2 > x1)
        sx = 1;
    else
        sx = -1;

    if (y2 > y1)
        sy = 1;
    else
        sy = -1;

    if (y1 == y2) {
        if (x2 > x1)
            DrawHLine(x1, y1, x2 - x1 + 1, color);
        else
            DrawHLine(x2, y1, x1 - x2 + 1, color);
         return;
    }
    else if (x1 == x2) {
        if (y2 > y1)
            DrawVLine(x1, y1, y2 - y1 + 1, color);
        else
            DrawVLine(x1, y2, y1 - y2 + 1, color);

        return;
    }

    /* inclination < 1 */
    if (dx > dy) {
        E = -dx;

        for (i = 0; i <= dx; i++) {
            DrawPixel(x1, y1, color);
            x1 += sx;
            E += 2 * dy;

            if (E >= 0) {
                y1 += sy;
                E -= 2 * dx;
            }
        }

        /* inclination >= 1 */
    }
    else {
        E = -dy;

        for (i = 0; i <= dy; i++) {
            DrawPixel(x1, y1, color);
            y1 += sy;
            E += 2 * dx;

            if (E >= 0) {
                x1 += sx;
                E -= 2 * dy;
            }
        }
    }
}

void
ILIDrawer::Impl::DrawPixel(uint16_t x, uint16_t y, ColorT color)
{
    if (x < 0 || y < 0 || x >= m_screen_width || y >= m_screen_height)
        return;

    uint8_t caset_data[4] = { static_cast<uint8_t>(x >> 8),
                              static_cast<uint8_t>(x),
                              static_cast<uint8_t>(x >> 8),
                              static_cast<uint8_t>(x) };

    uint8_t paset_data[4] = { static_cast<uint8_t>(y >> 8),
                              static_cast<uint8_t>(y),
                              static_cast<uint8_t>(y >> 8),
                              static_cast<uint8_t>(y) };

    WriteParameter(TFT_CASET, 4, caset_data);
    WriteParameter(TFT_PASET, 4, paset_data);

    WriteCommand(TFT_RAMWR);
    PutPixel(color);
}

void
ILIDrawer::Impl::DrawVLine(int16_t x, int16_t y, int16_t h, ILIDrawer::ColorT color)
{
    if ((x >= m_screen_width) || (y >= m_screen_height || h < 1))
        return;

    if ((y + h - 1) >= m_screen_height)
        h = m_screen_height - y;

    DrawFiledRectangle(x, y, 1, h, color);
}

void
ILIDrawer::Impl::DrawHLine(int16_t x, int16_t y, int16_t w, ILIDrawer::ColorT color)
{
    if ((x >= m_screen_width) || (y >= m_screen_height || w < 1))
        return;

    if ((x + w - 1) >= m_screen_width)
        w = m_screen_width - x;

    DrawFiledRectangle(x, y, w, 1, color);
}
void
ILIDrawer::Impl::DrawCircle(int16_t x0, int16_t y0, int16_t r, ILIDrawer::ColorT color)
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    DrawPixel(x0, y0 + r, color);
    DrawPixel(x0, y0 - r, color);
    DrawPixel(x0 + r, y0, color);
    DrawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        DrawPixel(x0 + x, y0 + y, color);
        DrawPixel(x0 - x, y0 + y, color);
        DrawPixel(x0 + x, y0 - y, color);
        DrawPixel(x0 - x, y0 - y, color);
        DrawPixel(x0 + y, y0 + x, color);
        DrawPixel(x0 - y, y0 + x, color);
        DrawPixel(x0 + y, y0 - x, color);
        DrawPixel(x0 - y, y0 - x, color);
    }
}
void
ILIDrawer::Impl::DrawFiledCircle(int16_t x0, int16_t y0, int16_t r, ILIDrawer::ColorT color)
{
    DrawVLine(x0, y0 - r, 2 * r + 1, color);
    DrawFiledCircleHelper(x0, y0, r, 3, 0, color);
}
void
ILIDrawer::Impl::DrawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, ILIDrawer::ColorT color)
{
    DrawHLine(x, y, w, color);
    DrawHLine(x, y + h - 1, w, color);
    DrawVLine(x, y, h, color);
    DrawVLine(x + w - 1, y, h, color);
}
void
ILIDrawer::Impl::DrawFiledRectangle(int16_t x, int16_t y, int16_t w, int16_t h, ILIDrawer::ColorT color)
{
    int16_t  end;
    uint32_t n_pixels;

    if (w < 0) {
        w = -w;
        x -= w;
    }

    end = x + w;

    if (x < 0)
        x = 0;

    if (end > m_screen_width)
        end = m_screen_width;

    w = end - x;

    if (h < 0) {
        h = -h;
        y -= h;
    }

    end = y + h;

    if (y < 0)
        y = 0;

    if (end > m_screen_height)
        end = m_screen_height;

    h        = end - y;
    n_pixels = (uint32_t)h * w;

    SetPartial(x, y, x + w - 1, y + h - 1);
    PutPixel(color, n_pixels);
}
void
ILIDrawer::Impl::Print(const char *string, uint8_t string_mode, uint8_t size)
{
    uint8_t i      = 0;
    uint8_t font_w = (FONT_WIDTH - FONT_SQUISH) * size;
    uint8_t font_h = (FONT_HEIGHT - FONT_SQUISH) * size;

    while ((*(string + i) != '\0') && (*string)) {
        if (*(string + i) == '\n') {
            m_cursor_y += font_h;
            m_cursor_x = 0;
            string++;
        }

        if (m_cursor_x > m_screen_width - font_w) {
            m_cursor_x = 0;
            m_cursor_y += font_h;
        }

        if (m_cursor_y > m_screen_height - font_h)
            m_cursor_y = m_cursor_x = 0;

        PrintChar(m_cursor_x, m_cursor_y, *(string + i), string_mode, size);
        m_cursor_x += font_w;
        i++;
    }
}
void
ILIDrawer::Impl::Print(uint16_t number, uint8_t string_mode, uint8_t size)
{
    int16_t temp = 1;

    if (number <= 0) {
        PrintChar(m_cursor_x, m_cursor_y, '0', string_mode, size);
        m_cursor_x += 14 * size;
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
            PrintChar(m_cursor_x, m_cursor_y, remainder + 48, string_mode, size);
            m_cursor_x += 14 * size;
        }
    }
}
void
ILIDrawer::Impl::Print(float number, uint8_t n_digits, uint8_t size)
{
    static char str[20];

    //    snprintf(str, sizeof(str), "%f", value);

    gcvtf(number, n_digits, str);

    Print(str, TFT_STR_M_BACKGR, size);
}
void
ILIDrawer::Impl::Init()
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

    SetRotation(TFT_PORTRAIT);
}

ILIDrawer::ILIDrawer()
  : impl{ std::make_unique<Impl>() }
{
    impl->Init();
    impl->Clear(COLOR_BLACK);
}

ILIDrawer::ILIDrawer(const ILIDrawer &other)
  : impl{ std::make_unique<Impl>(*other.impl) }
{ }

ILIDrawer &
ILIDrawer::operator=(const ILIDrawer &rhs)
{
    impl = std::make_unique<Impl>(*rhs.impl);
    return *this;
}

ILIDrawer::~ILIDrawer()                       = default;
ILIDrawer &ILIDrawer::operator=(ILIDrawer &&) = default;
ILIDrawer::ILIDrawer(ILIDrawer &&)            = default;

void
ILIDrawer::Clear(ColorT color)
{
    impl->Clear(color);
}
void
ILIDrawer::SetRotation(uint8_t rotation)
{
    impl->SetRotation(rotation);
}
void
ILIDrawer::SetTextColor(ColorT pixelcolor, ColorT backcolor)
{
    impl->SetTextColor(pixelcolor, backcolor);
}
void
ILIDrawer::SetCursor(ILIDrawer::PixelNumT x, ILIDrawer::PixelNumT y) noexcept
{
    impl->SetCursor(x, y);
}
void
ILIDrawer::FillScreen(ILIDrawer::ColorT color)
{
    impl->FillScreen(color);
}
void
ILIDrawer::DrawPoint(int16_t x0, int16_t y0, ILIDrawer::ColorT color)
{
    impl->DrawPoint(x0, y0, color);
}
void
ILIDrawer::DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, ILIDrawer::ColorT color)
{
    impl->DrawLine(x0, y0, x1, y1, color);
}
void
ILIDrawer::DrawVLine(int16_t x, int16_t y, int16_t h, ILIDrawer::ColorT color)
{
    impl->DrawVLine(x, y, h, color);
}
void
ILIDrawer::DrawHLine(int16_t x, int16_t y, int16_t w, ILIDrawer::ColorT color)
{
    impl->DrawHLine(x, y, w, color);
}
void
ILIDrawer::DrawCircle(int16_t x0, int16_t y0, int16_t r, ILIDrawer::ColorT color)
{
    impl->DrawCircle(x0, y0, r, color);
}
void
ILIDrawer::DrawFiledCircle(int16_t x0, int16_t y0, int16_t r, ILIDrawer::ColorT color)
{
    impl->DrawFiledCircle(x0, y0, r, color);
}
void
ILIDrawer::DrawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, ILIDrawer::ColorT color)
{
    impl->DrawRectangle(x, y, w, h, color);
}
void
ILIDrawer::DrawFiledRectangle(int16_t x, int16_t y, int16_t w, int16_t h, ILIDrawer::ColorT color)
{
    impl->DrawFiledRectangle(x, y, w, h, color);
}
void
ILIDrawer::Print(const char *string, uint8_t TFT_STRING_MODE, uint8_t size)
{
    impl->Print(string, TFT_STRING_MODE, size);
}
void
ILIDrawer::Print(uint16_t number, uint8_t string_mode, uint8_t size)
{
    impl->Print(number, string_mode, size);
}
void
ILIDrawer::Print(float number, uint8_t string_mode, uint8_t size)
{
    impl->Print(number, string_mode, size);
}
