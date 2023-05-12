#include "misc/compiler_compatibility_workaround.hpp"
#include <algorithm>
#include <tuple>

#include "pmc.h"
#include "pio.h"
#include "dacc.h"
#include "output_generator.hpp"
#include "system_init.h"

OutputGenerator *driver;

void
DACC_Handler(void)
{
    driver->InterruptHandler();
}

void
OutputGenerator::Init() noexcept
{
    InitDacc();
    StopGenerating();
    driver = this;
}
void
OutputGenerator::DaccSetDivider(Dacc *p_dacc, bool if_set) noexcept
{
    uint32_t mr = p_dacc->DACC_MR & (~DACC_MR_CLKDIV);

    if (if_set)
        mr |= DACC_MR_CLKDIV;

    p_dacc->DACC_MR = mr;
}
void
OutputGenerator::StartGenerating() noexcept
{
    itsFirstTimeInterrupt = true;

    pmc_enable_periph_clk(DaccPeripheralId);
    dacc_set_trigger(DACC, DaccTriggerSelection);
    SetAmplitude(50);   // todo make configurable from outside
    pdc_tx_init(dacc_get_pdc_base(DACC), &daccFirstPacket, &daccSecondPacket);
    pdc_enable_transfer(dacc_get_pdc_base(DACC), PERIPH_PTCR_TXTEN);

    NVIC_ClearPendingIRQ(DACC_IRQn);
    //    NVIC_ClearPendingIRQ(PIOA_IRQn);   // todo: why is this here? adc input trigger?

    dacc_enable_interrupt(DACC, DacInterruptMask);

    SetStatus(Status::Active);
}
void
OutputGenerator::StopGenerating() noexcept
{
    dacc_disable_interrupt(DACC, DacInterruptMask);
    pdc_disable_transfer(dacc_get_pdc_base(DACC), PERIPH_PTCR_TXTDIS);
    dacc_disable_trigger(DACC);
    pmc_disable_periph_clk(DaccPeripheralId);

    SetStatus(Status::Disabled);
}
void
OutputGenerator::RecalculateSineTableForNewAmplitude() noexcept
{
    auto amplitude_volts = (outputAmplitudeVolts > MaxAmplitudeVolts) ? MaxAmplitudeVolts : outputAmplitudeVolts;

    std::transform(sinus_table.begin(),
                   sinus_table.end(),
                   sinTable.begin(),
                   [amplitude = static_cast<SineTableValueT>(amplitude_volts * OneVoltDacValue),
                    offset    = static_cast<SineTableValueT>(HalfScaleOutputDacValue)](auto &sine_value) {
                       return sine_value * amplitude + offset;
                   });
}
void
OutputGenerator::SetAmplitude(OutputGenerator::AmplitudeT new_amplitude_volts) noexcept
{
    outputAmplitudeVolts = (new_amplitude_volts > MaxAmplitudeVolts) ? MaxAmplitudeVolts : new_amplitude_volts;
    RecalculateSineTableForNewAmplitude();
}
void
OutputGenerator::InitDacPdcPackets() noexcept
{
    daccFirstPacket.ul_addr  = reinterpret_cast<uint32_t>(sinTable.data());
    daccFirstPacket.ul_size  = sinTable.size();
    daccSecondPacket.ul_addr = reinterpret_cast<uint32_t>(sinTable.data());
    daccSecondPacket.ul_size = sinTable.size();
}
void
OutputGenerator::SetStatus(OutputGenerator::Status new_status) noexcept
{
    status = new_status;
}
