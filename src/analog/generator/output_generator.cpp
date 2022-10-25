#include <tuple>

#include "pmc.h"
#include "pio.h"
#include "dacc.h"
#include "output_generator.hpp"
#include "system_init.h"
OutputGenerator::OutputGenerator() {
    Init();
}

void
OutputGenerator::Init() noexcept
{
    pmc_enable_periph_clk(ID_DACC);
    dacc_reset(DACC);
    dacc_set_transfer_mode(DACC, DACC_TRASNSMODE);
    dacc_set_trigger(DACC, DACC_TRGSEL);
    dacc_set_timing(DACC, 0, 63);
    DaccSetDivider(DACC, true);
    dacc_set_channel_selection(DACC, DACC_CHANNELUSED);
    dacc_set_analog_control(DACC, DACC_ACR_VAL);
    dacc_enable_channel(DACC, DACC_CHANNELUSED);
    pio_set_peripheral(PIOA, PIO_PERIPH_C, DATRG_PIN);
}
void
OutputGenerator::DaccSetDivider(Dacc *p_dacc, bool set_divider) noexcept
{
    uint32_t mr = p_dacc->DACC_MR & (~DACC_MR_CLKDIV);

    if (set_divider)
        mr |= DACC_MR_CLKDIV;

    p_dacc->DACC_MR = mr;
}
void
OutputGenerator::StartGenerating() noexcept
{
//    Init();
    dacc_set_trigger(DACC, DACC_TRGSEL);
    SetAmplitude(50);   // todo make configurable from outside
    pdc_tx_init(dacc_get_pdc_base(DACC), &daccFirstPacket, &daccSecondPacket);
    pdc_enable_transfer(dacc_get_pdc_base(DACC), PERIPH_PTCR_TXTEN);

    NVIC_ClearPendingIRQ(DACC_IRQn);
    NVIC_ClearPendingIRQ(PIOA_IRQn);

    dacc_enable_interrupt(DACC, dacc_interrupt_mask);

    SetStatus(Status::Active);
}
void
OutputGenerator::StopGenerating() noexcept
{
    dacc_disable_interrupt(DACC, dacc_interrupt_mask);
    pdc_disable_transfer(dacc_get_pdc_base(DACC), PERIPH_PTCR_TXTDIS);
    dacc_disable_trigger(DACC);

    SetStatus(Status::Disabled);
}
void
OutputGenerator::CalculateSineTable() noexcept
{
    size_t counter            = 0;
    AmplitudeT constexpr offs = dacc_offset_halfscale;

    if (amplitude > dacc_max_amplitude) {
        amplitude = dacc_max_amplitude;
    }

    amplitude = amplitude * dacc_volts_to_digits_conv_coeff;

    for (size_t phase_counter{ 0 }; auto &table_value : sinTable) {
        table_value =
          (AmplitudeT)(static_cast<float>(amplitude) * sinus_table.at(phase_counter) + static_cast<float>(offs));

        phase_counter++;
    }
}

void
OutputGenerator::SetAmplitude(OutputGenerator::AmplitudeT new_amplitude) noexcept
{
    amplitude = new_amplitude;
    CalculateSineTable();
}
void
OutputGenerator::DaccPdcSetup(void) noexcept
{
    //    g_dacc_pdc_base            = dacc_get_pdc_base(DACC);
    daccFirstPacket.ul_addr  = (uint32_t)sinTable.data();
    daccFirstPacket.ul_size  = sinTable.size();
    daccSecondPacket.ul_addr = (uint32_t)sinTable.data();
    daccSecondPacket.ul_size = sinTable.size();

    NVIC_ClearPendingIRQ(DACC_IRQn);
    NVIC_SetPriority(DACC_IRQn, dacc_interrupt_prio);
    NVIC_EnableIRQ(DACC_IRQn);
}
void
OutputGenerator::SetStatus(OutputGenerator::Status new_status) noexcept
{
    status = new_status;
}
