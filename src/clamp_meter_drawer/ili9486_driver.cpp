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
#include <cstdint>
#include "ili9486_driver.hpp"
#include "ili9486_fonts.h"
#include "ILI9486_config.h"

extern "C" char *gcvtf(float, int, char *);

void
ILI9486Driver::Init() noexcept
{
    if (isInitialized)
        return;

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

    isInitialized = true;
}

template<ILI9486Driver::Pin pin, bool to_be_set>
constexpr void
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

void
ILI9486Driver::InitGPIO() const noexcept
{
    auto *p_matrix = static_cast<Matrix *>(MATRIX);

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
ILI9486Driver::SetDataL(uint8_t data) const noexcept
{
    auto data_a = uint32_t{ 0 };
    auto data_d = uint32_t{ 0 };

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

void
ILI9486Driver::StrobeWritePin() const noexcept
{
    SetPin<Pin::Write, true>();
    SetPin<Pin::Write, false>();
}

void
ILI9486Driver::Reset() const noexcept
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

void
ILI9486Driver::WriteData(uint8_t data) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Data, true>();

    SetDataL(data);
    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

void
ILI9486Driver::WriteDataDouble(uint16_t data) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Data, true>();

    SetDataL(data >> 8);
    StrobeWritePin();

    SetDataL(data & 0xff);
    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

void
ILI9486Driver::WriteCommandDouble(uint16_t cmd) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Command, true>();

    SetDataL(cmd >> 8);
    StrobeWritePin();
    SetDataL(cmd & 0xff);
    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

void
ILI9486Driver::WriteParameter(Byte cmd, int n_bytes, Byte *data) const noexcept
{
    WriteCommand(cmd);

    while (n_bytes--) {
        WriteData(*data++);
    }
}

void
ILI9486Driver::SetPartial(const Point point_beg, const Point point_end) const noexcept
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
ILI9486Driver::PrintChar(const Point point, Byte data, int size) const noexcept
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
ILI9486Driver::PutPixel(const Color color, int n) const noexcept
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

template<ILI9486Driver::Orientation orientation>
constexpr void
ILI9486Driver::SetOrientation() noexcept
{
    WriteCommand(TFT_MADCTL);

    if constexpr (orientation == Orientation::Portrait) {
        WriteData(0x48);   // 0b 0100 1000;
        m_screen_width  = TFT_WIDTH;
        m_screen_height = TFT_HEIGHT;
    }

    if constexpr (orientation == Orientation::Landscape) {   // 0b 0010 1000;
        WriteData(0x28);
        m_screen_width  = TFT_HEIGHT;
        m_screen_height = TFT_WIDTH;
    }

    if constexpr (orientation == Orientation::ReversePortrait) {
        WriteData(0x98);   // 0b 1001 1000;
        m_screen_width  = TFT_WIDTH;
        m_screen_height = TFT_HEIGHT;
    }

    if constexpr (orientation == Orientation::ReverseLandscape) {
        WriteData(0xF8);
        m_screen_width  = TFT_HEIGHT;
        m_screen_height = TFT_WIDTH;
    }
}

void
ILI9486Driver::DrawPixel(const Point point, const Color color) const noexcept
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
    PutPixel(color, 1);
}

void
ILI9486Driver::PutPixel(const Color color) const noexcept
{
    WriteCommand(TFT_RAMWR);

    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Data, true>();

    SetDataL(color & 0xff);
    SetDataH((color >> 8) & 0xff);

    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}

void
ILI9486Driver::WriteCommand(Byte cmd) const noexcept
{
    SetPin<Pin::ChipSelect, true>();
    SetPin<Pin::Command, true>();

    SetDataL(cmd);
    StrobeWritePin();

    SetPin<Pin::ChipSelect, false>();
}
