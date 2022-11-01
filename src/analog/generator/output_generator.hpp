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

#include <cstdint>
#include <array>

#include "pdc.h"
#include "dacc.h"

#include "dsp_resources.hpp"
#include "project_configs.hpp"

class OutputGenerator {
  public:
    using AmplitudeT      = uint16_t;
    using SineTableValueT = float;
    enum class Status : bool {
        Active,
        Disabled
    };

    OutputGenerator();

    void Init() noexcept;
    void DaccSetDivider(Dacc *p_dacc, bool if_set) noexcept;
    void StartGenerating() noexcept;
    void StopGenerating() noexcept;
    void RecalculateSineTableForNewAmplitude() noexcept;
    void SetAmplitude(AmplitudeT new_amplitude_volts) noexcept;
    void InterruptHandler() { pdc_tx_init(dacc_get_pdc_base(DACC), nullptr, &daccSecondPacket); }

  protected:
    void SetStatus(OutputGenerator::Status new_status) noexcept;
    void InitDacPdcPackets() noexcept;
    void InitDacc() noexcept
    {
        dacc_reset(DACC);
        dacc_set_transfer_mode(DACC, TransferMode);
        dacc_set_timing(DACC, DaccTiming_A, DaccTiming_B);
        DaccSetDivider(DACC, true);
        dacc_set_channel_selection(DACC, DacChannelInUse);
        dacc_set_analog_control(DACC, DaccAnalogControlACRValue);
        dacc_enable_channel(DACC, DacChannelInUse);
        pio_set_peripheral(PIOA, PIO_PERIPH_C, DaccPin);

        InitDacPdcPackets();

        NVIC_ClearPendingIRQ(DACC_IRQn);
        NVIC_SetPriority(DACC_IRQn, ProjectConfigs::GetInterruptPriority(ProjectConfigs::Interrupts::DaccPdc));
        NVIC_EnableIRQ(DACC_IRQn);
    }

  private:
    enum Configs {
        HalfScaleOutputDacValue   = 2047,
        MaxAmplitudeVolts         = 50,
        OneVoltDacValue           = 40,
        DacInterruptMask          = 0X04,
        SineTableLength           = 22,
        DacPacketLength           = SineTableLength,
        TransferMode              = 0,
        DacChannelInUse           = 0,
        DaccAnalogControlACRValue = 0x103,
        DaccPin                   = (1 << 2),
        DaccTiming_A              = 0,
        DaccTiming_B              = 63,
        DaccTriggerSelection      = 0,
        DaccPeripheralId          = ID_DACC
    };

    AmplitudeT                              outputAmplitudeVolts;
    std::array<AmplitudeT, SineTableLength> sinTable;
    pdc_packet_t                            daccFirstPacket;
    pdc_packet_t                            daccSecondPacket;
    Status                                  status{ Status::Disabled };
};
