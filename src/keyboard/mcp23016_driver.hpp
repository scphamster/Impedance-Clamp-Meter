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

#include <utility>
#include <vector>
#include <array>
#include <functional>

#include "compiler.h"
#include "pin.hpp"

constexpr Byte MCP23016_TWI_ADDR       = 0b00100000;   // SLA+W
constexpr int  MCP23016_INT_PIN        = 11;
Pio *const     MCP23016_INT_PORT       = PIOD;
constexpr int  MCP23016_INT_PORT_ID    = ID_PIOD;
constexpr IRQn MCP23016_IRQ_ID         = PIOD_IRQn;
constexpr int  MCP23016_INT_PIN_NUM    = MCP23016_INT_PIN + 32 * 3;
constexpr int  MCP23016_INTERRUPT_PRIO = 3;
constexpr int  MCP23016_TWI_FREQ       = 200000UL;

extern "C" void MCP23016_driver_IRQHandler(uint32_t id, uint32_t mask);

class MCP23016_driver {
  public:
    enum class Register : Byte {
        GP0     = 0X00,
        GP1     = 0X01,
        OLAT0   = 0X02,
        OLAT1   = 0X03,
        IPOL0   = 0X04,
        IPOL1   = 0X05,
        IODIR0  = 0X06,
        IODIR1  = 0X07,
        INTCAP0 = 0X08,
        INTCAP1 = 0X09,
        IOCON0  = 0X0A,
        IOCON1  = 0X0B
    };

    enum {
        NumberOfPins = 16,
    };

    MCP23016_driver();
    void Initialize() const noexcept;

    std::array<Pin::PinState, NumberOfPins> GetPinsState() const noexcept;
    void SetPinChangeCallback(std::function<void(const Pin &)> &&pin_change_callback);
    void InterruptHandler() const noexcept;
    void StartTask() noexcept;

  protected:
  private:
    std::array<Pin, NumberOfPins> pins;
    static bool                   isInitialized;
    int taskPriority = 3;
    std::function<void(const Pin &)> pinChangeCallback;
};
