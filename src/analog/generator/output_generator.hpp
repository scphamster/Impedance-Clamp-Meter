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
#include "dsp_resources.hpp"


class OutputGenerator {
  public:
    using AmplitudeT = uint16_t;
    enum class Status : bool {
        Active,
        Disabled
    };

    OutputGenerator();

    void Init() noexcept;
    void DaccSetDivider(Dacc *p_dacc, bool set_divider) noexcept;
    void StartGenerating() noexcept;
    void StopGenerating() noexcept;
    void CalculateSineTable() noexcept;
    void SetAmplitude(AmplitudeT new_amplitude) noexcept;

  protected:
    void SetStatus(OutputGenerator::Status new_status) noexcept;
    void DaccPdcSetup() noexcept;

  private:
    AmplitudeT amplitude;
    Status     status{ Status::Disabled };

    pdc_packet_t daccFirstPacket;
    pdc_packet_t daccSecondPacket;
    auto constexpr static sintableLen = 22;

    std::array<AmplitudeT, sintableLen> sinTable;

    auto constexpr static dacc_packetlen                  = sintableLen;
    auto constexpr static dacc_offset_halfscale           = 2047;
    auto constexpr static dacc_max_amplitude              = 50;
    auto constexpr static dacc_volts_to_digits_conv_coeff = 40;
    auto constexpr static dacc_interrupt_prio             = 2;
    auto constexpr static dacc_interrupt_mask             = 0X04;
};

