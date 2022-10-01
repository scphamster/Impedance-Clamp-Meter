#pragma once
#include "ili9486_driver.hpp"

template<typename DrawingDriver>
concept DisplayDrawer = requires(const DrawingDriver drv) {
                            //                          &DrawingDriver::SetCursor;
                            typename DrawingDriver::PixelNumT;
                            typename DrawingDriver::ColorT;
                        };

class ClampMeasData { };

template<DisplayDrawer DrawingDriver>
class ClampMeterDrawer : public DrawingDriver {
  public:
    using typename DrawingDriver::PixelNumT;
    using typename DrawingDriver::ColorT;

  protected:
  private:
    // configs
};