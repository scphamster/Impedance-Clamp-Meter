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

#include "clamp_meter_concepts.hpp"

//class AbstractPage {
//  public:
//
//};
//
//template<DisplayDrawerC Drawer>
//class MainPage {
//  public:
//
//
//
//  private:
//    std::unique_ptr<Drawer> drawer;
//    std::array<std::unique_ptr<AbstractPage>,
//};