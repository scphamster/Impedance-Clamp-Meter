#include <array>

#include "asf.h"
#include "twi_pdc.h"
#include "mcp23016_driver.hpp"
#include "FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"
#include "FreeRTOS/include/task.h"

static SemaphoreHandle_t gInterruptSemaphore = static_cast<SemaphoreHandle_t>(nullptr);

extern "C" void
MCP23016_driver_IRQHandler(uint32_t id, uint32_t mask)
{
    static BaseType_t higher_priority_task_woken;

    if (pio_get_pin_value(MCP23016_INT_PIN_NUM))
        return;

    // todo: eliminate this check after todo in MCP23016_driver_IRQ_task is implemented
    if (gInterruptSemaphore == static_cast<SemaphoreHandle_t>(nullptr))
        return;

    xSemaphoreGiveFromISR(gInterruptSemaphore, &higher_priority_task_woken);

    // check for higher priority task woken and call context switch if so
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

extern "C" void
MCP23016_driver_IRQ_task(void *driver_instance)
{
    // todo: make initialization of interrupt after semaphore is created, this would eliminate need for nullptr check in
    // interrupt. Bounded todos: MCP23016_driver_IRQHandler
    gInterruptSemaphore = xSemaphoreCreateBinary();
    auto driver         = static_cast<MCP23016_driver *>(driver_instance);

    while (true) {
        if (xSemaphoreTake(gInterruptSemaphore, portMAX_DELAY) == pdTRUE) {
            driver->InterruptHandler();
        }
    }
}

MCP23016_driver::MCP23016_driver()
{
    if (not isInitialized)
        Initialize();
}

void
MCP23016_driver::Initialize() const noexcept
{
    pio_set_input(MCP23016_INT_PORT, (1 << MCP23016_INT_PIN), PIO_PULLUP);

    Byte data1[] = { static_cast<Byte>(Register::IODIR0), 0xff };
    Byte data2[] = { static_cast<Byte>(Register::IODIR1), 0xff };
    Byte data3[] = { static_cast<Byte>(Register::IPOL0), 0xff };
    Byte data4[] = { static_cast<Byte>(Register::IPOL1), 0xff };
    Byte data5[] = { static_cast<Byte>(Register::IOCON0), 0x01 };

    twiPdc_write(data1, 2, MCP23016_TWI_ADDR);
    twiPdc_write(data2, 2, MCP23016_TWI_ADDR);
    twiPdc_write(data3, 2, MCP23016_TWI_ADDR);
    twiPdc_write(data4, 2, MCP23016_TWI_ADDR);
    twiPdc_write(data5, 2, MCP23016_TWI_ADDR);

    pio_handler_set(MCP23016_INT_PORT,
                    MCP23016_INT_PORT_ID,
                    (1 << MCP23016_INT_PIN),
                    PIO_IT_FALL_EDGE,
                    MCP23016_driver_IRQHandler);

    pio_handler_set_priority(MCP23016_INT_PORT, MCP23016_IRQ_ID, MCP23016_INTERRUPT_PRIO);
    pio_enable_interrupt(MCP23016_INT_PORT, (1 << MCP23016_INT_PIN));
}

std::array<Pin::PinState, MCP23016_driver::NumberOfPins>
MCP23016_driver::GetPinsState() const noexcept
{
    const auto LCD_TWI_FREQ = 100000UL;

    twiPdc_disable();

    uint32_t cpu_freq = sysclk_get_cpu_hz();

    twi_set_speed(TWI0, MCP23016_TWI_FREQ, cpu_freq);

    Byte         pins_state[2];
    twi_packet_t packet;

    // todo: try to use std::array as buffer
    packet.buffer      = pins_state;
    packet.length      = 3;
    packet.chip        = MCP23016_TWI_ADDR;
    packet.addr[0]     = static_cast<Byte>(Register::GP0);
    packet.addr_length = 1;

    twi_master_read(TWI0, &packet);

    twi_set_speed(TWI0, LCD_TWI_FREQ, cpu_freq);
    twiPdc_enable();

    return std::array<Pin::PinState, NumberOfPins>{
        static_cast<Pin::PinState>(pins_state[0] & (1 << 0)), static_cast<Pin::PinState>(pins_state[0] & (1 << 1)),
        static_cast<Pin::PinState>(pins_state[0] & (1 << 2)), static_cast<Pin::PinState>(pins_state[0] & (1 << 3)),
        static_cast<Pin::PinState>(pins_state[0] & (1 << 4)), static_cast<Pin::PinState>(pins_state[0] & (1 << 5)),
        static_cast<Pin::PinState>(pins_state[0] & (1 << 6)), static_cast<Pin::PinState>(pins_state[0] & (1 << 7)),
        static_cast<Pin::PinState>(pins_state[1] & (1 << 0)), static_cast<Pin::PinState>(pins_state[1] & (1 << 1)),
        static_cast<Pin::PinState>(pins_state[1] & (1 << 2)), static_cast<Pin::PinState>(pins_state[1] & (1 << 3)),
        static_cast<Pin::PinState>(pins_state[1] & (1 << 4)), static_cast<Pin::PinState>(pins_state[1] & (1 << 5)),
        static_cast<Pin::PinState>(pins_state[1] & (1 << 6)), static_cast<Pin::PinState>(pins_state[1] & (1 << 7))
    };
}

void
MCP23016_driver::InterruptHandler() const noexcept
{
    auto pins_state = GetPinsState();
}

void
MCP23016_driver::StartTask() noexcept
{
    xTaskCreate(MCP23016_driver_IRQ_task,
                "mcp23016 keyboard",
                configMINIMAL_STACK_SIZE,
                static_cast<void *const>(this),
                taskPriority,
                nullptr);
}
void
MCP23016_driver::SetPinChangeCallback(std::function<void(const Pin &)> &&pin_change_callback)
{
    pinChangeCallback = std::move(pin_change_callback);
}
