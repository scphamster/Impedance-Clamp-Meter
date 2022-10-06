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

#include "pin.hpp"

#include "clamp_meter_concepts.hpp"

template<IODriver Driver, typename TimerT>
class Keyboard {
  public:
    explicit Keyboard(std::shared_ptr<Driver> new_driver) noexcept
      : ioDriver{ new_driver }
    { }

    Keyboard() noexcept { Keyboard{ std::make_shared<Driver>() }; }

  protected:
    // slots:
    void ButtonsStateChangeCallback(const Pin &pin) const noexcept;

  private:
    std::shared_ptr<Driver> ioDriver;
};