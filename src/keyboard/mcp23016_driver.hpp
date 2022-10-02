#pragma once
#ifdef min
#undef min
#endif // max
#ifdef max
#undef max
#endif // max
#ifdef printf
#undef printf
#endif

#include <utility>
#include <vector>

class Pin {
public:
  using PinNumT = int;
  enum { Undefined = -1 };

  enum class PinMode { Input = 0, Output, HiZ, OpenCollector };

  enum class PinState { Low = 0, Hi };

  Pin(PinNumT pin_number, PinMode new_mode, PinState new_state)
      : pinNumber{pin_number}, state{new_state}, mode{new_mode} {}

  void SetPinNumber(PinNumT new_number) noexcept { pinNumber = new_number; }
  void SetPinMode(PinMode new_mode) noexcept { mode = new_mode; }
  void SetPinState(PinState new_state) noexcept { state = new_state; }
  PinNumT GetNumber() const noexcept { return pinNumber; }
  PinState GetPinState() const noexcept { return state; }
  PinMode GetPinMode() const noexcept { return mode; }

private:
  PinNumT pinNumber = Undefined;
  PinState state = PinState::Low;
  PinMode mode = PinMode::HiZ;
};

class MCP23016_driver {
public:
protected:
private:
  std::vector<Pin> pins;
};
