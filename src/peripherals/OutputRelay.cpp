#include "OutputRelay.hpp"
#include "pmc.h"

OutputRelay::OutputRelay(Pio *new_pio, size_t pio_id, size_t pin_num) noexcept
  : pio{ new_pio }
  , pioId{ pio_id }
  , pinMask{ (static_cast<PinMask>(1) << pin_num) }
{
    Initialize();
    Deactivate();
}

void
OutputRelay::Initialize() noexcept
{
    if (not pmc_is_periph_clk_enabled(pioId))
        pmc_enable_periph_clk(pioId);

    pio_set_output(pio, pinMask, pinMask, 0, 0);
}

void
OutputRelay::Deactivate() noexcept
{
    pio_set(pio, pinMask);

    //    if (pio_get(pio, PIO_INPUT, pinMask)) {}

    SetStatus(Status::Disabled);
}

void
OutputRelay::Activate() noexcept
{
    pio_clear(pio, pinMask);

    //    if (pio_get(pio, PIO_INPUT, pinMask)) {}

    SetStatus(Status::Enabled);
}
