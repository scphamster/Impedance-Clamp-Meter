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

#include <cstdlib>
#include "abstract_peripheral.hpp"
#include "pio.h"

class HVPowerSupply : public AbstractPeripheral {
  public:
    using PinMask = uint32_t;

    HVPowerSupply(Pio *new_pio, size_t pio_id, size_t pin_num) noexcept;

    void Initialize() noexcept override;
    void Deactivate() noexcept override;
    void Activate() noexcept override;

  private:
    Pio    *pio = nullptr;
    size_t  pioId{};
    PinMask pinMask{};
};
