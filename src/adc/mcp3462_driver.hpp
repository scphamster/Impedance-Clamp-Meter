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

class MCP3462_driver {
  public:
    enum class Reference {
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

    enum class ClockSelection {
        CLK_SEL_EXT        = 0,
        CLK_SEL_INT_OUTDIS = 2,
        CLK_SEL_INT_OUTEN
    };

    enum class cs_sel_type_t {
        CS_SEL_0 = 0,
        CS_SEL_0P9,
        CS_SEL_3P7,
        CS_SEL_15
    };

    enum class PowerState {
        ADC_SHUTDOWN_MODE = 0,
        ADC_STBY_MODE     = 2,
        ADC_CONV_MODE
    };

    enum class Prescaler {
        PRE_0 = 0,
        PRE_2,
        PRE_4,
        PRE_8
    };

    enum class OSR {
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

    enum class Gain{
        GAIN_1V3 = 0,
        GAIN_1,
        GAIN_2,
        GAIN_4,
        GAIN_8,
        GAIN_16,
        GAIN_32,
        GAIN_64
    };

    enum class Boost {
        BOOST_0P5 = 0,
        BOOST_0P66,
        BOOST_1,
        BOOST_2
    };

    enum class ConversionMode{
        CONV_MODE_ONESHOT_SHUTDOWN = 0,
        CONV_MODE_ONESHOT_STBY     = 2,
        CONV_MODE_CONT
    };

    void SetGain(Gain new_gain) {
        gain = new_gain;

        commBussDriver->

    }

  protected:
  private:
    std::shared_ptr<BusDriver> commBussDriver;

    Gain gain;
};