#pragma once
#include "compiler.h"
#include "pin.hpp"

template<typename IODriverT>
concept IODriver = requires(IODriverT driver) { 1 + 1; };

template<typename TimerT>
concept Timer = requires(TimerT timer) {
                    typename TimerT::TimeT;
                    {
                        timer.GetCurrentTime()
                    } noexcept;
                };

template<typename ButtonT>
concept ButtonC = requires(ButtonT button) {
                      typename ButtonT::ButtonState;
                      typename ButtonT::ButtonGroupIdT;
                  };

template<typename KeyboardT>
concept KeyboardC = requires(KeyboardT                                 keyboard,
                             typename KeyboardT::ButtonEvent           event,
                             typename KeyboardT::ButtonEventCallback &&callback) {
                        keyboard.SetButtonEventCallback(typename KeyboardT::ButtonName{}, event, std::move(callback));
                        typename KeyboardT::ButtonId;
                    };

template<typename TestedDriver>
concept DisplayDriver = requires(TestedDriver drv) {
                            drv.Init();
                            {
                                drv.DrawPixel(typename TestedDriver::Point{}, typename TestedDriver::ColorT{})
                            } noexcept;
                            {
                                drv.SetPartial(typename TestedDriver::Point{}, typename TestedDriver::Point{})
                            } noexcept;
                            {
                                drv.PutPixel(typename TestedDriver::ColorT{}, typename TestedDriver::PixelNumT{})
                            } noexcept;
                            {
                                drv.PutPixel(typename TestedDriver::ColorT{})
                            } noexcept;

                            typename TestedDriver::PixelNumT;
                            typename TestedDriver::ColorT;
                            typename TestedDriver::ScreenSizeT;
                            typename TestedDriver::Point;
                        };

template<typename DisplayDrawerT>
concept DisplayDrawerC = requires(DisplayDrawerT drawer) {
                             drawer.Clear(typename DisplayDrawerT::ColorT{});
                             drawer.SetCursor(typename DisplayDrawerT::Point{});
                             drawer.SetTextColor(typename DisplayDrawerT::ColorT{}, typename DisplayDrawerT::ColorT{});
                             drawer.FillScreen(typename DisplayDrawerT::ColorT{});
                             drawer.DrawPoint(typename DisplayDrawerT::Point{}, typename DisplayDrawerT::ColorT{});
                             drawer.DrawLine(typename DisplayDrawerT::Point{},
                                             typename DisplayDrawerT::Point{},
                                             typename DisplayDrawerT::ColorT{});
                             drawer.DrawVLine(typename DisplayDrawerT::Point{},
                                              typename DisplayDrawerT::ScreenSizeT{},
                                              typename DisplayDrawerT::ColorT{});
                             drawer.DrawHLine(typename DisplayDrawerT::Point{},
                                              typename DisplayDrawerT::ScreenSizeT{},
                                              typename DisplayDrawerT::ColorT{});
                             drawer.DrawCircle(typename DisplayDrawerT::Point{},
                                               typename DisplayDrawerT::ScreenSizeT{},
                                               typename DisplayDrawerT::ColorT{});
                             drawer.DrawFiledCircle(typename DisplayDrawerT::Point{},
                                                    typename DisplayDrawerT::ScreenSizeT{},
                                                    typename DisplayDrawerT::ColorT{});
                             drawer.DrawRectangle(typename DisplayDrawerT::Point{},
                                                  typename DisplayDrawerT::ScreenSizeT{},
                                                  typename DisplayDrawerT::ScreenSizeT{},
                                                  typename DisplayDrawerT::ColorT{});
                             drawer.DrawFiledRectangle(typename DisplayDrawerT::Point{},
                                                       typename DisplayDrawerT::ScreenSizeT{},
                                                       typename DisplayDrawerT::ScreenSizeT{},
                                                       typename DisplayDrawerT::ColorT{});
                             drawer.Print(static_cast<char *>(nullptr), Byte{});
                             drawer.Print(uint16_t{}, Byte{});
                             drawer.Print(float{}, Byte{}, Byte{});
                         };