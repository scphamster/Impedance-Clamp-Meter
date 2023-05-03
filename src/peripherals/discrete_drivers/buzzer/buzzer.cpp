#include <cmath>

#include "buzzer.hpp"
#include "tc.h"
#include "pmc.h"
#include "pio.h"
#include "system.h"

#define BUZZER_TIMER    TC0
#define BUZZER_TIMER_ID ID_TC0
std::shared_ptr<Buzzer> Buzzer::_this = nullptr;

void
Buzzer::Init() noexcept
{
    if (isInitialized)
        return;

    // todo: make configurable
    timerInternals.min              = 50;
    timerInternals.max              = 5000;
    timerInternals.modulatorMax     = 100000;
    timerInternals.modulatorMin     = 10000;
    timerInternals.modulatorChannel = 1;
    timerInternals.mainToneChannel  = 2;

    minimalPeriod    = 2 * 128 / 120e6f;   // todo: cleanup magic numbers || 128-  prescaller, 120e6 - core clock, 2 - updown toggle
    maxToneFrequency = 1 / minimalPeriod;
    minToneFrequency = 50;
    maxModulationFrequency = 30;
    minModulationFrequency = 1;

    // todo: implement check of correctness of frequency value

    uint32_t clock_source = (3 << 0);
    uint32_t wavsel       = (2 << 13);
    uint32_t wavemode     = (1 << 15);
    uint32_t ACPC_val     = (3 << 18);
    uint32_t burst        = (3 << 4);   // burst with xc2
    uint32_t block        = (3 << 4);   // TIOA1 connected to XC2

    //    pmc_enable_periph_clk(BUZZER_TIMER_ID);
    //    tc_init(BUZZER_TIMER, timerInternals.channel, clock_source | /*burst |*/ wavsel | wavemode | ACPC_val);
    //    tc_write_rc(BUZZER_TIMER, timerInternals.channel, 5000);

    pmc_enable_periph_clk(ID_TC2);
    pmc_enable_periph_clk(ID_TC1);
    tc_set_block_mode(TC0, block);
    tc_init(TC0, 1, clock_source | wavsel | wavemode | ACPC_val);
    tc_init(TC0, 2, clock_source | burst | wavsel | wavemode | ACPC_val);
    tc_write_rc(TC0, 1, 2000);
    tc_write_rc(TC0, 2, 5000);
    tc_start(TC0, 1);

    isInitialized = true;
}

bool
Buzzer::SetFrequency(float tone_frequency, float modulation_frequency) noexcept
{
    if ((xTaskGetTickCount() - lastEntry) < pdMS_TO_TICKS(delayBetweenFrequencyChangesMs))
        return false;
    else
        lastEntry = xTaskGetTickCount();

    if (tone_frequency > maxToneFrequency)
        tone_frequency = maxToneFrequency;
    else if (tone_frequency < minToneFrequency)
        tone_frequency = minToneFrequency;

    if (modulation_frequency > maxModulationFrequency)
        modulation_frequency = maxModulationFrequency;
    else if (modulation_frequency < minModulationFrequency)
        modulation_frequency = minModulationFrequency;

    SetChanelFrequency(timerInternals.mainToneChannel, tone_frequency);
    SetChanelFrequency(timerInternals.modulatorChannel, modulation_frequency);

    toneFrequency       = tone_frequency;
    modulationFrequency = modulation_frequency;

    return true;
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
Buzzer::Enable() const noexcept
{
    pio_set_peripheral(PIOA, PIO_PERIPH_B, (1 << usedPinNumber));
    tc_start(BUZZER_TIMER, timerInternals.mainToneChannel);
}
void
Buzzer::Disable() const noexcept
{
    pio_set_input(PIOA, (1 << usedPinNumber), 0);
    tc_stop(BUZZER_TIMER, timerInternals.mainToneChannel);
}

Buzzer::RegisterSubtype
Buzzer::FindRCFromFrequency(float frequency) noexcept
{
    configASSERT(frequency < maxToneFrequency);
    return static_cast<RegisterSubtype>(1 / (minimalPeriod * frequency));
}
void
Buzzer::SetChanelFrequency(int channel, FreqT frequency) noexcept
{
    configASSERT(channel == timerInternals.modulatorChannel or channel == timerInternals.mainToneChannel);
    auto new_rc  = FindRCFromFrequency(frequency);

    if (new_rc - tc_read_cv(BUZZER_TIMER, channel) < timerPeriodChangeMargin) {
        tc_write_rc(BUZZER_TIMER, channel, new_rc);
        tc_start(BUZZER_TIMER, channel);
    }
    else
        tc_write_rc(BUZZER_TIMER, channel, new_rc);
}

std::pair<Buzzer::FreqT, Buzzer::FreqT>
Buzzer::GetFrequencies() const noexcept
{
    return std::pair<FreqT, FreqT>{ toneFrequency, modulationFrequency };
}
