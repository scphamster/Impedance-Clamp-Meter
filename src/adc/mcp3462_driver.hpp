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

#include <memory>
#include "compiler.h"

class MCP3462_driver {
  public:
    enum class Reference : Byte {
        CH0 = 0,
        CH1,
        CH2,
        CH3,
        CH4,
        CH5,
        CH6,
        CH7,
        AGND,
        AVDD,
        REFIN_POS = 11,
        REFIN_NEG,
        DIODE_P,
        DIODE_M,
        VCM
    };

    enum class ClockSelection : Byte {
        CLK_SEL_EXT        = 0,
        CLK_SEL_INT_OUTDIS = 2,
        CLK_SEL_INT_OUTEN
    };

    enum class CurrentSource : Byte {
        CS_SEL_0 = 0,
        CS_SEL_0P9,
        CS_SEL_3P7,
        CS_SEL_15
    };

    enum class PowerState : Byte {
        ADC_SHUTDOWN_MODE = 0,
        ADC_STBY_MODE     = 2,
        ADC_CONV_MODE
    };

    enum class Prescaler : Byte {
        PRE_0 = 0,
        PRE_2,
        PRE_4,
        PRE_8
    };

    enum class OSR : Byte {
        OSR_32 = 0,
        OSR_64,
        OSR_128,
        OSR_256,
        OSR_512,
        OSR_1024,
        OSR_2048,
        OSR_4096,
        OSR_8192,
        OSR_16384,
        OSR_20480,
        OSR_24576,
        OSR_40960,
        OSR_49152,
        OSR_81920,
        OSR_98304
    };

    enum class Gain : Byte {
        GAIN_1V3 = 0,
        GAIN_1,
        GAIN_2,
        GAIN_4,
        GAIN_8,
        GAIN_16,
        GAIN_32,
        GAIN_64
    };

    enum class Boost : Byte {
        BOOST_0P5 = 0,
        BOOST_0P66,
        BOOST_1,
        BOOST_2
    };

    enum class ConversionMode : Byte {
        CONV_MODE_ONESHOT_SHUTDOWN = 0,
        CONV_MODE_ONESHOT_STBY     = 2,
        CONV_MODE_CONT
    };

    enum class ADC_MODE : Byte {
        ShutdownDefault = 0,
        Shutdown,
        Standby,
        Conversion
    };

    enum class AutoZeroingMuxMode : Byte {
        Disabled = 0,
        Enabled  = 1
    };

    enum class DataFormat : Byte {
        Default16Bit = 0,
        _32Bit_16BitLeftJustified,
        _32Bit_17BitRightJustified,
        _32Bit_17BitRightJustifiedWithChannelId
    };

    enum class CRC_Format : Byte {
        CRC_16Only = 0,
        CRC_16And16Zeros
    };

    enum class IRQ_InactivePinMode : Byte {
        HiZ = 0,
        LogicHigh
    };

    enum class IRQ_PinMode : Byte {
        IRQ = 0,
        MDAT
    };

    explicit MCP3462_driver(Byte device_address = 0x40);
    void Initialize() noexcept;

    void SetGain(Gain new_gain) { gain = new_gain; }
    void ClockInit() noexcept;

  protected:
    Byte CreateConfig0RegisterValue(bool           if_full_shutdown,
                                    ClockSelection clock,
                                    CurrentSource  current_source,
                                    ADC_MODE       adc_mode) noexcept;

    Byte CreateConfig1RegisterValue(Prescaler prescaler, OSR oversampling_ratio) noexcept;
    Byte CreateConfig2RegisterValue(Boost boost_value, Gain gain, AutoZeroingMuxMode autozero) noexcept;
    Byte CreateConfig3RegisterValue(ConversionMode conversion_mode,
                                    DataFormat     data_format,
                                    CRC_Format     crc_format,
                                    bool           enable_crc_checksum,
                                    bool           enable_offset_calibration,
                                    bool           enable_gain_calibration) noexcept;
    Byte CreateIRQRegisterValue(bool                enable_conversion_start_interrupt,
                                bool                enable_fast_command,
                                IRQ_PinMode         irq_pin_mode,
                                IRQ_InactivePinMode inactive_irq_pin_mode) noexcept;
    Byte CreateMUXRegisterValue(Reference positive_channel, Reference negative_channel) noexcept;

  private:
    Gain gain;
    Byte deviceAddress{};
};