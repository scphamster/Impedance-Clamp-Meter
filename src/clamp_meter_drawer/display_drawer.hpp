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
#include "ILI9486_config.h"

// #include "ili9486_driver.hpp"

extern "C" char *gcvtf(float, int, char *);

template<typename TestedDriver>
concept DisplayDriver = requires(TestedDriver drv) {
                            drv.Init();
                            {
                                drv.DrawPixel(typename TestedDriver::Point{}, typename TestedDriver::Color{})
                            } noexcept;
                            {
                                drv.SetPartial(typename TestedDriver::Point{}, typename TestedDriver::Point{})
                            } noexcept;
                            {
                                drv.PutPixel(typename TestedDriver::Color{}, typename TestedDriver::PixelNumT{})
                            } noexcept;
                            {
                                drv.PutPixel(typename TestedDriver::Color{})
                            } noexcept;

                            typename TestedDriver::PixelNumT;
                            typename TestedDriver::Color;
                            typename TestedDriver::ScreenSizeT;
                            typename TestedDriver::Point;

                        };

template<DisplayDriver Driver>
class DisplayDrawer {
  public:
    using PixelNumT   = typename Driver::PixelNumT;
    using Color       = typename Driver::Color;
    using ScreenSizeT = typename Driver::ScreenSizeT;
    using Point       = typename Driver::Point;
    using Orientation = typename Driver::Orientation;

    explicit DisplayDrawer(std::shared_ptr<Driver>);
    explicit DisplayDrawer(const DisplayDrawer &)   = default;
    DisplayDrawer &operator=(const DisplayDrawer &) = default;
    explicit DisplayDrawer(DisplayDrawer &&)        = default;
    DisplayDrawer &operator=(DisplayDrawer &&)      = default;
    virtual ~DisplayDrawer()                        = default;

    void Clear(Color color) noexcept;
    void SetCursor(Point) noexcept;
    void SetTextColor(Color pixelcolor, Color backcolor) const noexcept;
    void FillScreen(Color) noexcept;
    void DrawPoint(Point p, Color) const noexcept;
    void DrawLine(Point, Point, Color) const noexcept;
    void DrawVLine(Point, ScreenSizeT h, Color) const noexcept;
    void DrawHLine(Point, ScreenSizeT w, Color) const noexcept;
    void DrawCircle(Point, ScreenSizeT r, Color) const noexcept;
    void DrawFiledCircle(Point, ScreenSizeT r, Color) const noexcept;
    void DrawRectangle(Point, ScreenSizeT w, ScreenSizeT h, Color) const noexcept;
    void DrawFiledRectangle(Point, ScreenSizeT w, ScreenSizeT h, Color) noexcept;
    void Print(const char *string, Byte fontsize) noexcept;
    void Print(uint16_t number, Byte fontsize) noexcept;
    void Print(float number, Byte digits, Byte fontsize) noexcept;

  protected:
    void DrawFiledCircleHelper(const Point point,
                               ScreenSizeT r,
                               int         cornername,
                               int         delta,
                               const Color color) const noexcept;

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
DisplayDrawer<Driver>::Clear(const Color color) noexcept
{
    FillScreen(color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::SetTextColor(const Color pixelcolor, const Color backcolor) const noexcept
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
DisplayDrawer<Driver>::FillScreen(Color color) noexcept
{
    DrawFiledRectangle(Point{ 0, 0 }, driver->m_screen_width, driver->m_screen_height, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawPoint(const Point point, const Color color) const noexcept
{
    ScreenSizeT r = 1;

    DrawVLine(Point{ point.x, point.y - r }, 2 * r + 1, color);
    DrawFiledCircleHelper(point, r, 3, 0, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawFiledCircleHelper(const Point point,
                                             ScreenSizeT r,
                                             int         cornername,
                                             int         delta,
                                             const Color color) const noexcept
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
DisplayDrawer<Driver>::DrawLine(Point point_beg, const Point point_end, const Color color) const noexcept
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
DisplayDrawer<Driver>::DrawVLine(const Point point, const ScreenSizeT h, const Color color) const noexcept
{
    if ((point.x >= driver->m_screen_width) || (point.y >= driver->m_screen_height || h < 1))
        return;

    if ((point.y + h - 1) >= driver->m_screen_height)
        h = driver->m_screen_height - point.y;

    DrawFiledRectangle(point, 1, h, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawHLine(const Point point, const ScreenSizeT w, const Color color) const noexcept
{
    if ((point.x >= driver->m_screen_width) || (point.y >= driver->m_screen_height || w < 1))
        return;

    if ((point.x + w - 1) >= driver->m_screen_width)
        w = driver->m_screen_width - point.x;

    DrawFiledRectangle(point, w, 1, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawCircle(const Point point, const ScreenSizeT r, const Color color) const noexcept
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
DisplayDrawer<Driver>::DrawFiledCircle(const Point point, const ScreenSizeT r, const Color color) const noexcept
{
    DrawVLine(Point{ point.x, point.y - r }, 2 * r + 1, color);
    DrawFiledCircleHelper(Point{ point.x, point.y }, r, 3, 0, color);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawRectangle(const Point       point,
                                     const ScreenSizeT w,
                                     const ScreenSizeT h,
                                     const Color       color) const noexcept
{
    DrawHLine(point, w, color);
    DrawHLine(Point{ point.x, point.y + h - 1 }, w, color);
    DrawVLine(point, h, color);
    DrawVLine(Point{ point.x + w - 1, point.y }, h, color);
}
template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::DrawFiledRectangle(Point point, ScreenSizeT w, ScreenSizeT h, const Color color) noexcept
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

    if (end > driver->m_screen_width)
        end = driver->m_screen_width;

    w = end - point.x;

    if (h < 0) {
        h = -h;
        point.y -= h;
    }

    end = point.y + h;

    if (point.y < 0)
        point.y = 0;

    if (end > driver->m_screen_height)
        end = driver->m_screen_height;

    h        = end - point.y;
    n_pixels = h * w;

    driver->SetPartial(point, Point{ point.x + w - 1, point.y + h - 1 });
    driver->PutPixel(color, n_pixels);
}

template<DisplayDriver Driver>
void
DisplayDrawer<Driver>::Print(const char *string, const Byte size) noexcept
{
    Byte i      = 0;
    Byte font_w = (FONT_WIDTH - FONT_SQUISH) * size;
    Byte font_h = (FONT_HEIGHT - FONT_SQUISH) * size;

    while ((*(string + i) != '\0') && (*string)) {
        if (*(string + i) == '\n') {
            cursorPosition.x += font_h;
            cursorPosition.x = 0;
            string++;
        }

        if (cursorPosition.x > driver->m_screen_width - font_w) {
            cursorPosition.x = 0;
            cursorPosition.y += font_h;
        }

        if (cursorPosition.y > driver->m_screen_height - font_h)
            cursorPosition.y = cursorPosition.x = 0;

        driver->PrintChar(Point{ cursorPosition }, *(string + i), size);
        cursorPosition.x += font_w;
        i++;
    }
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

    //TODO: optimize floating number drawing, its 1000 times slower than integer drawing
    gcvtf(number, n_digits, &buffer[0]);

    Print(buffer, size);
}
