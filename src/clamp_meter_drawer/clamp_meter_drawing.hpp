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

template<typename Driver>
class ClampMeterDrawer : public Driver {
  public:
    // set_keyboard_input(std::shared_ptr<Keyboard> new_keyboard);
    /*
        keyboard.SetCallbackForKey<Keyboard::Enter>(std::function<void()> function);

    */

  protected:
  private:
    // configs
};
