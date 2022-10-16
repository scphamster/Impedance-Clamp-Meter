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

#include "FreeRTOS/FreeRTOSConfig.h"
#include "compiler.h"
#include "pin.hpp"

constexpr Byte _MCP23016_TWI_ADDR       = 0b00100000;   // SLA+W
constexpr int  _MCP23016_INT_PIN        = 11;
Pio *const     _MCP23016_INT_PORT       = PIOD;
constexpr int  _MCP23016_INT_PORT_ID    = ID_PIOD;
constexpr IRQn _MCP23016_IRQ_ID         = PIOD_IRQn;
constexpr int  _MCP23016_INT_PIN_NUM    = _MCP23016_INT_PIN + 32 * 3;
constexpr int  _MCP23016_INTERRUPT_PRIO = 3;
constexpr int  _MCP23016_TWI_FREQ       = 200000UL;

extern "C" void MCP23016_driver_IRQHandler(uint32_t id, uint32_t mask);

class MCP23016_driver {
    // todo: implement twi driver to benefit from encapsulation

  public:
    using PinStateChangeCallback = std::function<void(const Pin_MCP23016 &)>;

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
        NumberOfPins                 = 16,
        NumberOfBitsInByte           = 8,
        StreamBufferSinglePacketSize = NumberOfPins / NumberOfBitsInByte,
        StreamBufferSize             = (NumberOfPins / sizeof(Byte)) * 10
    };

    MCP23016_driver(PinStateChangeCallback &&callback);
    void Initialize() const noexcept;

    [[nodiscard]] static uint16_t GetPinsState() noexcept;
    void                          SetPinStateChangeCallback(PinStateChangeCallback &&pin_change_callback);
    void                          InterruptHandler() noexcept;
    void                          StartTask() noexcept;
    [[nodiscard]] static std::array<Pin_MCP23016::PinState, NumberOfPins> StreamBufferToPinStateArray(
      std::array<Byte, StreamBufferSinglePacketSize> &&serial_data);

  protected:
  private:
    static bool isInitialized;
    auto constexpr static rtosTaskPriority          = 4;   // todo: make configurable
    auto constexpr static hardwareInterruptPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY;

    std::array<Pin_MCP23016, NumberOfPins> pins;
    PinStateChangeCallback        pinStateChangeCallback;
};
