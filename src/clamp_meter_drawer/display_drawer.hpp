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

#include "ili9486_driver.hpp"

//template<typename TestedDriver>
//concept DisplayDriver = requires {
//                            1 + 1;
//                        };
//
//template<DisplayDriver Driver>
class DisplayDrawer {
  public:
    DisplayDrawer(/*std::shared_ptr<DisplayDriver>*/);
    explicit DisplayDrawer(const DisplayDrawer &);
    DisplayDrawer &operator=(const DisplayDrawer &);
    explicit DisplayDrawer(DisplayDrawer &&);
    DisplayDrawer &operator=(DisplayDrawer &&);
    virtual ~DisplayDrawer();

    void Clear(Color color) const noexcept;
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
    std::shared_ptr<Impl> impl;
};
