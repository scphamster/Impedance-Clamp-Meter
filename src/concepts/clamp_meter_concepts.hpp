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
concept ButtonC = requires(ButtonT button) { typename ButtonT::ButtonState; };

template<typename KeyboardT>
concept KeyboardC = requires(KeyboardT                                 keyboard,
                             typename KeyboardT::ButtonEvent           event,
                             typename KeyboardT::ButtonEventCallback &&callback) {
                        keyboard.SetButtonEventCallback(int{}, event, std::move(callback));
                    };

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

template<typename DisplayDrawerT>
concept DisplayDrawerC = requires(DisplayDrawerT drawer) {
                             drawer.Clear(typename DisplayDrawerT::Color{}, typename DisplayDrawerT::Color{});
                             drawer.SetCursor(typename DisplayDrawerT::Point{});
                             drawer.SetTextColor(typename DisplayDrawerT::Color{},
                                                 typename DisplayDrawerT::Color{},
                                                 typename DisplayDrawerT::Color{},
                                                 typename DisplayDrawerT::Color{});
                             drawer.FillScreen(typename DisplayDrawerT::Color{});

                             drawer.DrawPoint(typename DisplayDrawerT::Point{}, typename DisplayDrawerT::Color{});
                             drawer.DrawLine(typename DisplayDrawerT::Point{},
                                             typename DisplayDrawerT::Point{},
                                             typename DisplayDrawerT::Color{});
                             drawer.DrawVLine(typename DisplayDrawerT::Point{},
                                              typename DisplayDrawerT::ScreenSizeT{},
                                              typename DisplayDrawerT::Color{});
                             drawer.DrawHLine(typename DisplayDrawerT::Point{},
                                              typename DisplayDrawerT::ScreenSizeT{},
                                              typename DisplayDrawerT::Color{});
                             drawer.DrawCircle(typename DisplayDrawerT::Point{},
                                               typename DisplayDrawerT::ScreenSizeT{},
                                               typename DisplayDrawerT::Color{});
                             drawer.DrawFiledCircle(typename DisplayDrawerT::Point{},
                                                    typename DisplayDrawerT::ScreenSizeT{},
                                                    typename DisplayDrawerT::Color{});
                             drawer.DrawRectangle(typename DisplayDrawerT::Point{},
                                                  typename DisplayDrawerT::ScreenSizeT{},
                                                  typename DisplayDrawerT::ScreenSizeT{},
                                                  typename DisplayDrawerT::Color{});
                             drawer.DrawFiledRectangle(typename DisplayDrawerT::Point{},
                                                       typename DisplayDrawerT::ScreenSizeT{},
                                                       typename DisplayDrawerT::ScreenSizeT{},
                                                       typename DisplayDrawerT::Color{});
                             drawer.Print(static_cast<char *>(nullptr), Byte{});
                             drawer.Print(uint16_t{}, Byte{});
                             drawer.Print(float{}, Byte{}, Byte{});
                         };