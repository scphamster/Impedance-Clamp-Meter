#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <array>
#include <algorithm>

#include "asf.h"
#include "twi_pdc.h"
#include "mcp23016_driver_fast.hpp"
#include "FreeRTOS.h"
#include "FreeRTOS/include/task.h"
#include "FreeRTOS/include/stream_buffer.h"
#include "mcp23016.h"

#define MCP23016_GP0     0X00
#define MCP23016_GP1     0X01
#define MCP23016_OLAT0   0X02
#define MCP23016_OLAT1   0X03
#define MCP23016_IPOL0   0X04
#define MCP23016_IPOL1   0X05
#define MCP23016_IODIR0  0X06
#define MCP23016_IODIR1  0X07
#define MCP23016_INTCAP0 0X08
#define MCP23016_INTCAP1 0X09
#define MCP23016_IOCON0  0X0A
#define MCP23016_IOCON1  0X0B

#ifdef assert
#undef assert
#endif

#ifdef DEBUG
#define assert(fault_if_zero) configASSERT(fault_if_zero)
#else
#define assert(empty) ((void)0)
#endif

#include "project_configs.hpp"

static StreamBufferHandle_t SB_pins_status                 = nullptr;
bool                        MCP23016_driver::isInitialized = false;

extern "C" void
MCP23016_driver_IRQHandler(uint32_t id, uint32_t mask)
{
    if (pio_get_pin_value(MCP23016_INT_PIN_NUM))
        return;

    BaseType_t higher_prio_task_woken;

    uint16_t pin_data = MCP23016_driver::GetPinsState();
    uint8_t  data_buffer[2];
    data_buffer[0] = pin_data & 0xff;
    data_buffer[1] = 0xff & (pin_data >> 8);

    xStreamBufferSendFromISR(SB_pins_status, &data_buffer, sizeof(data_buffer), &higher_prio_task_woken);
    portYIELD_FROM_ISR(higher_prio_task_woken);
}

extern "C" [[noreturn]] void
MCP23016_DriverTask(void *driver_instance)
{
    auto driver = static_cast<MCP23016_driver *>(driver_instance);

    while (1)
        driver->InterruptHandler();
}

MCP23016_driver::MCP23016_driver(MCP23016_driver::PinStateChangeCallback &&callback)
  : pins{ Pin_MCP23016{ 0, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 1, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 2, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 3, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 4, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 5, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 6, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 7, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 8, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 9, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 10, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 11, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 12, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 13, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 14, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low },
          Pin_MCP23016{ 15, Pin_MCP23016::PinMode::Input, Pin_MCP23016::PinState::Low } }
  , pinStateChangeCallback{ std::forward<PinStateChangeCallback>(callback) }
{
    // workaround to unlock mcp23016:
    auto dummy = GetPinsState();

    StartTask();

    if (not MCP23016_driver::isInitialized) {
        Initialize();
    }
}

void
MCP23016_driver::Initialize() const noexcept
{
    pio_set_input(MCP23016_INT_PORT, (1 << MCP23016_INT_PIN), PIO_PULLUP);

    uint8_t data1[] = { MCP23016_IODIR0, 0xff };

    twiPdc_write(data1, 2, MCP23016_TWI_ADDR);

    uint8_t data2[] = { MCP23016_IODIR1, 0xff };

    twiPdc_write(data2, 2, MCP23016_TWI_ADDR);

    uint8_t data3[] = { MCP23016_IPOL0, 0xff };

    twiPdc_write(data3, 2, MCP23016_TWI_ADDR);

    uint8_t data4[] = { MCP23016_IPOL1, 0xff };

    twiPdc_write(data4, 2, MCP23016_TWI_ADDR);

    uint8_t data5[] = { MCP23016_IOCON0, 0x01 };

    twiPdc_write(data5, 2, MCP23016_TWI_ADDR);

    pio_handler_set(MCP23016_INT_PORT,
                    MCP23016_INT_PORT_ID,
                    (1 << MCP23016_INT_PIN),
                    PIO_IT_FALL_EDGE,
                    MCP23016_driver_IRQHandler);

    pio_handler_set_priority(MCP23016_INT_PORT,
                             MCP23016_IRQ_ID,
                             ProjectConfigs::GetInterruptPriority(ProjectConfigs::Interrupts::Keyboard));
    pio_enable_interrupt(MCP23016_INT_PORT, (1 << MCP23016_INT_PIN));

    MCP23016_driver::isInitialized = true;
}

uint16_t
MCP23016_driver::GetPinsState() noexcept
{
    uint32_t constexpr lcd_twi_freq = 100000;
    uint32_t     cpu_freq           = sysclk_get_cpu_hz();
    uint8_t      buffer[2];
    twi_packet_t packet{ .addr        = { MCP23016_GP0, 0, 0 },
                         .addr_length = 1,
                         .buffer      = buffer,
                         .length      = 3,
                         .chip        = MCP23016_TWI_ADDR };

    twiPdc_disable();
    twi_set_speed(TWI0, MCP23016_TWI_FREQ, cpu_freq);

    twi_master_read(TWI0, &packet);

    twi_set_speed(TWI0, lcd_twi_freq, cpu_freq);
    twiPdc_enable();

    return (buffer[0] | (buffer[1] << 8));
}

void
MCP23016_driver::InterruptHandler() noexcept
{
    auto pins_state_buffer = std::array<Byte, StreamBufferSinglePacketSize>{};
    xStreamBufferReceive(SB_pins_status, &pins_state_buffer[0], MCP23016_driver::StreamBufferSize, portMAX_DELAY);

    auto new_pins_state = StreamBufferToPinStateArray(std::move(pins_state_buffer));

    for (int pin_num = 0; const auto &pin_state : new_pins_state) {
        if (pin_state != pins.at(pin_num).GetPinState()) {
            pinStateChangeCallback(pins.at(pin_num));
            pins.at(pin_num).SetPinState(pin_state);
        }

        assert(pin_num < NumberOfPins);
        pin_num++;
    }
}

void
MCP23016_driver::StartTask() noexcept
{
    const auto buffer_size                = StreamBufferSize;
    const auto triggering_number_of_bytes = 2;

    SB_pins_status = xStreamBufferCreate(buffer_size, triggering_number_of_bytes);

    if (SB_pins_status == nullptr) {
        while (1) { }
    }
    // todo: make better exception catcher
    auto task_creation_result = xTaskCreate(MCP23016_DriverTask,
                                            "mcp23016 keyboard",
                                            ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::MCP23016),
                                            static_cast<void *const>(this),
                                            ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::MCP23016),
                                            nullptr);

    configASSERT(task_creation_result == pdPASS);
}
void
MCP23016_driver::SetPinStateChangeCallback(std::function<void(const Pin_MCP23016 &)> &&pin_change_callback)
{
    pinStateChangeCallback = std::move(pin_change_callback);
}

std::array<Pin_MCP23016::PinState, MCP23016_driver::NumberOfPins>
MCP23016_driver::StreamBufferToPinStateArray(
  std::array<Byte, MCP23016_driver::StreamBufferSinglePacketSize> &&serial_data)
{
    auto pins_state = std::array<Pin_MCP23016::PinState, NumberOfPins>{};

    for (int pin_num = 0, bit_num = 0, byte_num = 0; pin_num < NumberOfPins; pin_num++) {
        pins_state.at(pin_num) =
          static_cast<Pin_MCP23016::PinState>(static_cast<bool>((serial_data.at(byte_num) & (1 << bit_num))));

        if (bit_num == 7) {
            bit_num = 0;
            byte_num++;
        }
        else
            bit_num++;
    }

    return pins_state;
}
