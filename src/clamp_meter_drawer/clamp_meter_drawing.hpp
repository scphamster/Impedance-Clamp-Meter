#pragma once
#include "ili9486_driver.hpp"

//template<typename TestedDriver>
//concept DisplayDriver = requires(const TestedDriver drv) {
//                            //                          &TestedDriver::SetCursor;
////                            typename TestedDriver::PixelNumT;
////                            typename TestedDriver::Color;
////                            typename TestedDriver::Point;
//                        };

class ClampMeasData { };

template<typename Driver>
class ClampMeterDrawer : public Driver {
  public:
//    using typename Driver::PixelNumT;
//    using typename Driver::Color;
//    using Point = typename Driver::Point;

    void ShowMainPage();
    void ShowCalibrationPage();
    void SetDataSource();



  protected:

  private:
    // configs
};

//clamp_meter.display.Show(Page);

// class MainMeasurementsPage {
//      Show();
//
//
//
//
//