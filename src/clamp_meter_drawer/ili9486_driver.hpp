#pragma once
#include <stdint.h>
#include <memory>

class ILIDrawer {
  public:
    ILIDrawer();
    explicit ILIDrawer(const ILIDrawer &);
    ILIDrawer &operator=(const ILIDrawer &);
    explicit ILIDrawer(ILIDrawer &&);
    ILIDrawer &operator=(ILIDrawer &&);
    virtual ~ILIDrawer();

    using PixelNumT = uint16_t;
    using ColorT    = uint16_t;

//    void Init();
//    void Reset();
    void Clear(uint16_t color);
    void SetRotation(uint8_t rotation);
    void SetCursor(PixelNumT x, PixelNumT y) noexcept;
    void SetTextColor(uint16_t pixelcolor, uint16_t backcolor);

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

  private:
    class Impl;
    std::unique_ptr<Impl> impl;
};