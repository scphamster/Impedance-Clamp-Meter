#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include "display_drawer.hpp"
#include "ili9486_driver.hpp"

extern "C" char *gcvtf(float, int, char *);


class DisplayDrawer::Impl {
  public:
    Impl()
      : driver{ std::make_shared<ILI9486Driver>() }
    {
        driver->Init();
    }
    Impl(const Impl &)               = default;
    Impl &operator=(const Impl &rhs) = default;
    Impl(Impl &&)                    = default;
    Impl &operator=(Impl &&rhs)      = default;
    ~Impl()                          = default;

    void Clear(const Color color) const noexcept;

    void SetCursor(const Point) noexcept;
    void SetTextColor(const Color pixelcolor, const Color backcolor) noexcept;

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
    void Print(uint16_t number, uint8_t size) noexcept;
    void Print(float number, uint8_t n_digits, uint8_t size) noexcept;

  protected:
    void DrawFiledCircleHelper(const Point, ScreenSizeT r, uint8_t cornername, int delta, const Color) const noexcept;

  private:
    Point cursorPosition;

    std::shared_ptr<ILI9486Driver> driver;
};

void
DisplayDrawer::Impl::SetCursor(const Point point) noexcept
{
    cursorPosition = point;
}

void
DisplayDrawer::Impl::Clear(const Color color) const noexcept
{
    FillScreen(color);
}

void
DisplayDrawer::Impl::DrawFiledCircleHelper(const Point point,
                                           ScreenSizeT r,
                                           uint8_t     cornername,
                                           int         delta,
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

void
DisplayDrawer::Impl::SetTextColor(Color pixelcolor, Color backcolor) noexcept
{
    driver->pixelColour = pixelcolor;
    driver->backColour  = backcolor;
}

void
DisplayDrawer::Impl::FillScreen(const Color color) const noexcept
{
    DrawFiledRectangle(Point{ 0, 0 }, driver->m_screen_width, driver->m_screen_height, color);
}

void
DisplayDrawer::Impl::DrawPoint(const Point point, const Color color) const noexcept
{
    ScreenSizeT r = 1;

    DrawVLine(Point{ point.x, point.y - r }, 2 * r + 1, color);
    DrawFiledCircleHelper(point, r, 3, 0, color);
}

void
DisplayDrawer::Impl::DrawLine(Point point_beg, const Point point_end, const Color color) const noexcept
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

void
DisplayDrawer::Impl::DrawVLine(const Point point, ScreenSizeT h, const Color color) const noexcept
{
    if ((point.x >= driver->m_screen_width) || (point.y >= driver->m_screen_height || h < 1))
        return;

    if ((point.y + h - 1) >= driver->m_screen_height)
        h = driver->m_screen_height - point.y;

    DrawFiledRectangle(point, 1, h, color);
}

void
DisplayDrawer::Impl::DrawHLine(const Point point, ScreenSizeT w, Color color) const noexcept
{
    if ((point.x >= driver->m_screen_width) || (point.y >= driver->m_screen_height || w < 1))
        return;

    if ((point.x + w - 1) >= driver->m_screen_width)
        w = driver->m_screen_width - point.x;

    DrawFiledRectangle(point, w, 1, color);
}

void
DisplayDrawer::Impl::DrawCircle(const Point point, ScreenSizeT r, Color color) const noexcept
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

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

void
DisplayDrawer::Impl::DrawFiledCircle(const Point point, ScreenSizeT r, Color color) const noexcept
{
    DrawVLine(Point{ point.x, point.y - r }, 2 * r + 1, color);
    DrawFiledCircleHelper(Point{ point.x, point.y }, r, 3, 0, color);
}

void
DisplayDrawer::Impl::DrawRectangle(const Point point, ScreenSizeT w, ScreenSizeT h, Color color) const noexcept
{
    DrawHLine(point, w, color);
    DrawHLine(Point{ point.x, point.y + h - 1 }, w, color);
    DrawVLine(point, h, color);
    DrawVLine(Point{ point.x + w - 1, point.y }, h, color);
}

void
DisplayDrawer::Impl::DrawFiledRectangle(Point point, ScreenSizeT w, ScreenSizeT h, Color color) const noexcept
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
    n_pixels = (uint32_t)h * w;

    driver->SetPartial(point, Point{ point.x + w - 1, point.y + h - 1 });
    driver->PutPixel(color, n_pixels);
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

void
DisplayDrawer::Impl::Print(uint16_t number, uint8_t size) noexcept
{
    int16_t temp = 1;

    if (number <= 0) {
        driver->PrintChar(cursorPosition, '0', size);
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
            driver->PrintChar(cursorPosition, remainder + 48, size);
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

DisplayDrawer::DisplayDrawer(/*std::shared_ptr<DisplayDriver>*/)
  : impl{ std::make_shared<Impl>() }
{
    impl->Clear(COLOR_BLACK);
}

DisplayDrawer::DisplayDrawer(const DisplayDrawer &other)
  : impl{ other.impl }
{ }

DisplayDrawer &

DisplayDrawer::operator=(const DisplayDrawer &rhs)
{
    impl = rhs.impl;
    return *this;
}

DisplayDrawer::~DisplayDrawer() = default;

DisplayDrawer &DisplayDrawer::operator=(DisplayDrawer &&) = default;

DisplayDrawer::DisplayDrawer(DisplayDrawer &&) = default;

void
DisplayDrawer::Clear(const Color color) const noexcept
{
    impl->Clear(color);
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
DisplayDrawer::FillScreen(Color color) const noexcept
{
    impl->FillScreen(color);
}

void
DisplayDrawer::DrawPoint(const Point p, const Color color) const noexcept
{
    impl->DrawPoint(p, color);
}

void
DisplayDrawer::DrawLine(Point point_beg, const Point point_end, const Color color) const noexcept
{
    impl->DrawLine(point_beg, point_end, color);
}

void
DisplayDrawer::DrawVLine(const Point point, const ScreenSizeT h, const Color color) const noexcept
{
    impl->DrawVLine(point, h, color);
}

void
DisplayDrawer::DrawHLine(const Point point, const ScreenSizeT w, const Color color) const noexcept
{
    impl->DrawHLine(point, w, color);
}

void
DisplayDrawer::DrawCircle(const Point point, const ScreenSizeT r, const Color color) const noexcept
{
    impl->DrawCircle(point, r, color);
}

void
DisplayDrawer::DrawFiledCircle(const Point point, const ScreenSizeT r, const Color color) const noexcept
{
    impl->DrawFiledCircle(point, r, color);
}

void
DisplayDrawer::DrawRectangle(const Point       point,
                             const ScreenSizeT w,
                             const ScreenSizeT h,
                             const Color       color) const noexcept
{
    impl->DrawRectangle(point, w, h, color);
}
void
DisplayDrawer::DrawFiledRectangle(const Point       point,
                                  const ScreenSizeT w,
                                  const ScreenSizeT h,
                                  const Color       color) const noexcept
{
    impl->DrawFiledRectangle(point, w, h, color);
}

void
DisplayDrawer::Print(const char *string, const uint8_t fontsize) const noexcept
{
    impl->Print(string, fontsize);
}

void
DisplayDrawer::Print(uint16_t number, const uint8_t fontsize) const noexcept
{
    impl->Print(number, fontsize);
}

void
DisplayDrawer::Print(float number, uint8_t digits, const uint8_t fontsize) const noexcept
{
    impl->Print(number, digits, fontsize);
}

