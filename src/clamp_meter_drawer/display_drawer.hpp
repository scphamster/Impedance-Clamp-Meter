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

#include <memory>
#include "compiler.h"
#include "clamp_meter_concepts.hpp"
#include "ILI9486_config.h"

extern "C" char *gcvtf(float, int, char *);

template<DisplayDriver Driver>
class DisplayDrawer {
  public:
    using PixelNumT   = typename Driver::PixelNumT;
    using ColorT      = typename Driver::ColorT;
    using ScreenSizeT = typename Driver::ScreenSizeT;
    using Point       = typename Driver::Point;
    using Orientation = typename Driver::Orientation;

    enum class Color : uint32_t {
        black        = 0x0000,
        navy         = 0x000F,
        darkgreen    = 0x03E0,
        ddgreen      = 0xc0,
        darkcyan     = 0x03EF,
        maroon       = 0x7800,
        purple       = 0x780F,
        olive        = 0x7BE0,
        lightgrey    = 0xC618,
        darkgrey     = 0x7BEF,
        blue         = 0x001F,
        green        = 0x07E0,
        cyan         = 0x07FF,
        red          = 0xF800,
        magenta      = 0xF81F,
        yellow       = 0xFFE0,
        white        = 0xFFFF,
        orange       = 0xFD20,
        greenyellow  = 0xAFE5,
        pink         = 0xF81F,
        darkdarkgrey = 0x514A,
        redyellow    = 60580,
        lightmud     = 42023,
        grey         = 10565,
    };

    explicit DisplayDrawer(std::shared_ptr<Driver>);
    explicit DisplayDrawer(const DisplayDrawer &)   = default;
    DisplayDrawer &operator=(const DisplayDrawer &) = default;
    explicit DisplayDrawer(DisplayDrawer &&)        = default;
    DisplayDrawer &operator=(DisplayDrawer &&)      = default;
    virtual ~DisplayDrawer()                        = default;

    [[nodiscard]] Point GetCursorPosition() const noexcept { return cursorPosition; }

    void Clear(ColorT color) noexcept;
    void SetCursor(Point) noexcept;
    void SetTextColor(ColorT pixelcolor, ColorT backcolor) const noexcept;
    void FillScreen(ColorT) noexcept;
    void DrawPoint(Point p, ColorT) const noexcept;
    void DrawLine(Point, Point, ColorT) const noexcept;
    void DrawVLine(Point, ScreenSizeT h, ColorT) const noexcept;
    void DrawHLine(Point, ScreenSizeT w, ColorT) const noexcept;
    void DrawCircle(Point, ScreenSizeT r, ColorT) const noexcept;
    void DrawFiledCircle(Point, ScreenSizeT r, ColorT) const noexcept;
    void DrawRectangle(Point, ScreenSizeT w, ScreenSizeT h, ColorT) const noexcept;
    void DrawFiledRectangle(Point, ScreenSizeT w, ScreenSizeT h, ColorT) noexcept;
    void DrawFiledRectangle(Point top_left, Point bot_right, ColorT color) noexcept
    {
        auto w        = bot_right.x - top_left.x;
        auto h        = bot_right.y - top_left.y;
        auto n_pixels = h * w;

        driver->SetPartial(top_left, bot_right);
        driver->PutPixel(color, n_pixels);
    }
    void Print(const std::string &string, Byte fontsize) noexcept;
    void Print(uint16_t number, Byte fontsize) noexcept;
    void Print(float number, Byte digits, Byte fontsize) noexcept;
    void Print(float number, int fontsize) noexcept { Print(number, 5, fontsize); }

  protected:
    void DrawFiledCircleHelper(const Point point, ScreenSizeT r, int cornername, int delta, const ColorT color) const noexcept;

  private:
    std::shared_ptr<Driver> driver;
    Point                   cursorPosition{ 0, 0 };
};

template<DisplayDriver Driver>
DisplayDrawer<Driver>::DisplayDrawer(std::shared_ptr<Driver> new_driver)
  : driver{ new_driver }
{
    driver->Init();
    Clear(COLOR_BLACK);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::Clear(const ColorT color) noexcept
{
    FillScreen(color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::SetTextColor(const ColorT pixelcolor, const ColorT backcolor) const noexcept
{
    driver->pixelColour = pixelcolor;
    driver->backColour  = backcolor;
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::SetCursor(const Point point) noexcept
{
    cursorPosition = point;
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::FillScreen(ColorT color) noexcept
{
    DrawFiledRectangle(Point{ 0, 0 }, driver->screen_width, driver->screen_height, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawPoint(const Point point, const ColorT color) const noexcept
{
    ScreenSizeT r = 1;

    DrawVLine(Point{ point.x, point.y - r }, 2 * r + 1, color);
    DrawFiledCircleHelper(point, r, 3, 0, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawFiledCircleHelper(const Point point, ScreenSizeT r, int cornername, int delta, const ColorT color)
  const noexcept
{
    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x     = 0;
    int y     = r;

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

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawLine(Point point_beg, const Point point_end, const ColorT color) const noexcept
{
    int i  = 0;
    int dx = 0, dy = 0;
    int sx = 0, sy = 0;
    int E = 0;

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
            driver->DrawPixel(point_beg, color);
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
            driver->DrawPixel(point_beg, color);
            point_beg.y += sy;
            E += 2 * dx;

            if (E >= 0) {
                point_beg.x += sx;
                E -= 2 * dy;
            }
        }
    }
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawVLine(const Point point, const ScreenSizeT h, const ColorT color) const noexcept
{
    if ((point.x >= driver->screen_width) || (point.y >= driver->screen_height || h < 1))
        return;

    if ((point.y + h - 1) >= driver->screen_height)
        h = driver->screen_height - point.y;

    DrawFiledRectangle(point, 1, h, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawHLine(const Point point, const ScreenSizeT w, const ColorT color) const noexcept
{
    if ((point.x >= driver->screen_width) || (point.y >= driver->screen_height || w < 1))
        return;

    if ((point.x + w - 1) >= driver->screen_width)
        w = driver->screen_width - point.x;

    DrawFiledRectangle(point, w, 1, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawCircle(const Point point, const ScreenSizeT r, const ColorT color) const noexcept
{
    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x     = 0;
    int y     = r;

    driver->DrawPixel(Point{ point.x, point.y + r }, color);
    driver->DrawPixel(Point{ point.x, point.y - r }, color);
    driver->DrawPixel(Point{ point.x + r, point.y }, color);
    driver->DrawPixel(Point{ point.x - r, point.y }, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        driver->DrawPixel(Point{ point.x + x, point.y + y }, color);
        driver->DrawPixel(Point{ point.x - x, point.y + y }, color);
        driver->DrawPixel(Point{ point.x + x, point.y - y }, color);
        driver->DrawPixel(Point{ point.x - x, point.y - y }, color);
        driver->DrawPixel(Point{ point.x + y, point.y + x }, color);
        driver->DrawPixel(Point{ point.x - y, point.y + x }, color);
        driver->DrawPixel(Point{ point.x + y, point.y - x }, color);
        driver->DrawPixel(Point{ point.x - y, point.y - x }, color);
    }
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawFiledCircle(const Point point, const ScreenSizeT r, const ColorT color) const noexcept
{
    DrawVLine(Point{ point.x, point.y - r }, 2 * r + 1, color);
    DrawFiledCircleHelper(Point{ point.x, point.y }, r, 3, 0, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawRectangle(const Point point, const ScreenSizeT w, const ScreenSizeT h, const ColorT color) const noexcept
{
    DrawHLine(point, w, color);
    DrawHLine(Point{ point.x, point.y + h - 1 }, w, color);
    DrawVLine(point, h, color);
    DrawVLine(Point{ point.x + w - 1, point.y }, h, color);
}
template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawFiledRectangle(Point point, ScreenSizeT w, ScreenSizeT h, const ColorT color) noexcept
{
    int end      = 0;
    int n_pixels = 0;

    if (w < 0) {
        w = -w;
        point.x -= w;
    }

    end = point.x + w;

    if (point.x < 0)
        point.x = 0;

    if (end > driver->screen_width)
        end = driver->screen_width;

    w = end - point.x;

    if (h < 0) {
        h = -h;
        point.y -= h;
    }

    end = point.y + h;

    if (point.y < 0)
        point.y = 0;

    if (end > driver->screen_height)
        end = driver->screen_height;

    h        = end - point.y;
    n_pixels = h * w;

    driver->SetPartial(point, Point{ point.x + w - 1, point.y + h - 1 });
    driver->PutPixel(color, n_pixels);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::Print(const std::string &str, const Byte size) noexcept
{
    auto string = str.c_str();

    Byte idx    = 0;
    Byte font_w = (FONT_WIDTH - FONT_SQUISH) * size;
    Byte font_h = (FONT_HEIGHT - FONT_SQUISH) * size;

    for (const auto &character : str) {
        if (character == '\n') {
            cursorPosition.x += font_h;
            cursorPosition.x = 0;
            continue;
        }

        if (cursorPosition.x > driver->screen_width - font_w - 10) {
            cursorPosition.x = 0;
            cursorPosition.y += font_h;
        }

        if (cursorPosition.y > driver->screen_height - font_h)
            cursorPosition.y = cursorPosition.x = 0;

        driver->PrintChar(Point{ cursorPosition }, character, size);
        cursorPosition.x += font_w;
    }

    //    while ((*(string + idx) != '\0')) {
    //        if (*(string + idx) == '\n') {
    //            cursorPosition.x += font_h;
    //            cursorPosition.x = 0;
    //            string++;
    //        }
    //
    //        if (cursorPosition.x > driver->screen_width - font_w) {
    //            cursorPosition.x = 0;
    //            cursorPosition.y += font_h;
    //        }
    //
    //        if (cursorPosition.y > driver->screen_height - font_h)
    //            cursorPosition.y = cursorPosition.x = 0;
    //
    //        driver->PrintChar(Point{ cursorPosition }, *(string + idx), size);
    //        cursorPosition.x += font_w;
    //        idx++;
    //    }
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::Print(uint16_t number, Byte const size) noexcept
{
    const auto index_of_first_digit_in_table = 48;

    int temp = 1;

    auto const font_height = 14;

    if (number <= 0) {
        driver->PrintChar(cursorPosition, '0', size);
        cursorPosition.x += font_height * size;
    }
    else {
        while (number != 0) {
            Byte remainder = number % 10;
            number         = number / 10;
            temp           = temp * 10 + remainder;
        }

        while (temp != 1) {
            Byte remainder = temp % 10;
            temp           = temp / 10;
            driver->PrintChar(cursorPosition, remainder + index_of_first_digit_in_table, size);
            cursorPosition.x += font_height * size;
        }
    }
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::Print(float number, Byte n_digits, const Byte size) noexcept
{
    auto const buffsize = 20;
    char       buffer[buffsize];

    // TODO: optimize floating number drawing, its 1000 times slower than integer drawing
    gcvtf(number, n_digits, &buffer[0]);

    Print(buffer, size);
}
