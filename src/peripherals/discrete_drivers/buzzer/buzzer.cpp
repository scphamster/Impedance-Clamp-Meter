#include <cmath>

#include "buzzer.hpp"
#include "tc.h"
#include "pmc.h"
#include "pio.h"

#define BUZZER_TIMER TC0

std::shared_ptr<Buzzer> Buzzer::_this = nullptr;

void
Buzzer::Init() noexcept
{
    if (isInitialized)
        return;

    // todo: make configurable
    timerInternals.min     = 50;
    timerInternals.max     = 5000;
    timerInternals.channel = 2;

    // todo: implement check of correctness of frequency value

    uint32_t clock_source = (3 << 0);
    uint32_t wavsel       = (2 << 13);
    uint32_t wavemode     = (1 << 15);
    uint32_t ACPC_val     = (3 << 18);
//    uint32_t burst        = (3 << 4);   // burst with xc2
    uint32_t block        = (3 << 4);   // TIOA1 connected to XC2

    pmc_enable_periph_clk(ID_TC0);
//    pio_configure_pin(usedPinNumber, )
    tc_init(TC0, timerInternals.channel, clock_source | /*burst |*/ wavsel | wavemode | ACPC_val);
    tc_write_rc(TC0, timerInternals.channel, 5000);

    isInitialized = true;
}

void
Buzzer::SetFrequency(float current) noexcept
{
    auto new_rc = 50 + k * (-log2f(current) - offst);

    if (static_cast<RegisterSubtype>(new_rc) > timerInternals.max)
        new_rc = static_cast<float>(timerInternals.max);
    if (static_cast<RegisterSubtype>(new_rc) < timerInternals.min)
        new_rc = static_cast<float>(timerInternals.min);

    if (static_cast<RegisterSubtype>(new_rc) != timerInternals.rc) {
        tc_write_rc(BUZZER_TIMER, timerInternals.channel, static_cast<RegisterSubtype>(new_rc));
        tc_start(BUZZER_TIMER, timerInternals.channel);

        timerInternals.rc = static_cast<RegisterSubtype>(new_rc);
    }
}
std::shared_ptr<Buzzer>
Buzzer::Get() noexcept
{
    if (_this)
        return _this;
    else {
        _this = std::shared_ptr<Buzzer>(new Buzzer);
        return _this;
    }
}
void
Buzzer::Enable() noexcept
{
    pio_set_peripheral(PIOA, PIO_PERIPH_B, (1 << usedPinNumber));
    tc_start(BUZZER_TIMER, timerInternals.channel);
}
void
Buzzer::Disable() noexcept
{
    pio_set_input(PIOA, (1 << usedPinNumber), 0);
    tc_stop(BUZZER_TIMER, timerInternals.channel);
}

