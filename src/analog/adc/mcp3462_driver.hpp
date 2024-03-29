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

#include <functional>
#include <memory>
#include "compiler.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "stream_buffer.h"

#include "clamp_meter_concepts.hpp"
#include "queue.hpp"

// todo: try to make data transfer more c++ish, to make class not SINGLETONE
class MCP3462_driver {
  public:
    using SPI_CommandT         = uint16_t;
    using CommandT             = Byte;
    using AddressT             = Byte;
    using ValueT               = int32_t;
    using InterruptCallbackT   = void(ValueT);
    using QueueT               = QueueHandle_t;
    using StreamBufferBorrowed = StreamBuffer<ValueT>;
    using OutputStreamBuffer   = StreamBuffer<ValueT>;

    enum class MeasurementMode {
        SingleShot,
        Continuous
    };

    enum class Command {
    };

    enum class Reference : CommandT {
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

    enum class ClockSelection : CommandT {
        CLK_SEL_EXT        = 0,
        CLK_SEL_INT_OUTDIS = 2,
        CLK_SEL_INT_OUTEN
    };

    enum class CurrentSource : CommandT {
        CS_SEL_0 = 0,
        CS_SEL_0P9,
        CS_SEL_3P7,
        CS_SEL_15
    };

    enum class PowerState : CommandT {
        ADC_SHUTDOWN_MODE = 0,
        ADC_STBY_MODE     = 2,
        ADC_CONV_MODE
    };

    enum class Prescaler : CommandT {
        PRE_0 = 0,
        PRE_2,
        PRE_4,
        PRE_8
    };

    enum class OSR : CommandT {
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

    enum class Gain : CommandT {
        GAIN_1V3 = 0,
        GAIN_1,
        GAIN_2,
        GAIN_4,
        GAIN_8,
        GAIN_16,
        GAIN_32,
        GAIN_64
    };

    enum class Boost : CommandT {
        BOOST_0P5 = 0,
        BOOST_0P66,
        BOOST_1,
        BOOST_2
    };

    enum class ConversionMode : CommandT {
        CONV_MODE_ONESHOT_SHUTDOWN = 0,
        CONV_MODE_ONESHOT_STBY     = 2,
        CONV_MODE_CONT
    };

    enum class ADC_MODE : CommandT {
        ShutdownDefault = 0,
        Shutdown,
        Standby,
        Conversion
    };

    enum class AutoZeroingMuxMode : CommandT {
        Disabled = 0,
        Enabled  = 1
    };

    enum class DataFormat : CommandT {
        Default16Bit = 0,
        _32Bit_16BitLeftJustified,
        _32Bit_17BitRightJustified,
        _32Bit_17BitRightJustifiedWithChannelId
    };

    enum class CRC_Format : CommandT {
        CRC_16Only = 0,
        CRC_16And16Zeros
    };

    enum class IRQ_InactivePinMode : CommandT {
        HiZ = 0,
        LogicHigh
    };

    enum class IRQ_PinMode : CommandT {
        IRQ = 0,
        MDAT
    };

    enum class Register : CommandT {
        ADC_Data = 0,
        Config0,
        Config1,
        Config2,
        Config3,
        IRQ,
        MUX,
        Scan,
        Timer,
        OffsetCalibration,
        GainCalibration,
        Lock      = 0xd,
        CRCConfig = 0xf
    };

    enum Configs {
        QueueMsgSize           = sizeof(ValueT),
        StreamBufferPacketsLen = 5,
        StreamBufferLen        = QueueMsgSize * StreamBufferPacketsLen
    };

    explicit MCP3462_driver(AddressT device_address /*= 0x40*/, size_t stream_buffer_capacity, size_t stream_buffer_triggering_size)
      : deviceAddress{ device_address }
      , streamBufferCapacity{ stream_buffer_capacity }
      , streamBufferTriggeringSize{ stream_buffer_triggering_size }
    {
        Initialize();
    }

    void Initialize() noexcept;

    void SetGain(Gain new_gain) noexcept;
    void ClockInit() noexcept;
    void SetMux(Reference positive_channel, Reference negative_channel) noexcept;
    void SetDataInterruptCallback(MCP3462_driver::InterruptCallbackT &&new_callback) noexcept;
    void StartMeasurement() noexcept
    {
        if (not outputSB)
            std::terminate();

        EnableInterrupt();
        EnableClock();
    }
    void StopMeasurement() noexcept;
    void SetMeasurementMode(MeasurementMode new_mode) noexcept;   // todo implement
    void SetOutputStreamBuffer(std::shared_ptr<OutputStreamBuffer> new_sb) noexcept { outputSB = new_sb; }
    void StartSynchronousMeasurements() noexcept
    {
        if (not outputSB)
            std::terminate();

        EnableInterrupt(false);
        EnableClock();
    }
    void SynchronizationCallback() noexcept { EnableInterrupt(false); }

    ValueT         SingleShotMeasurement() noexcept;   // todo implement
    ValueT         Read() noexcept;
    BaseType_t     HandleInterruptStreamBuffer() noexcept { return outputSB->SendFromISR(Read()); }
    decltype(auto) CreateNewStreamBuffer() noexcept
    {
        return std::make_shared<OutputStreamBuffer>(streamBufferCapacity, streamBufferTriggeringSize);
    }
    std::underlying_type_t<Gain> GetGainLevel() const noexcept { return static_cast<std::underlying_type_t<Gain>>(gain); }

  protected:
    void EnableInterrupt(bool if_enable = true) noexcept;
    void EnableClock(bool if_enable = true) noexcept;

    CommandT        CreateConfig0RegisterValue(bool           if_full_shutdown,
                                               ClockSelection clock,
                                               CurrentSource  current_source,
                                               ADC_MODE       adc_mode) noexcept;
    CommandT        CreateConfig1RegisterValue(Prescaler prescaler, OSR oversampling_ratio) noexcept;
    CommandT        CreateConfig2RegisterValue(Boost boost_value, Gain gain, AutoZeroingMuxMode autozero) noexcept;
    CommandT        CreateConfig3RegisterValue(ConversionMode conversion_mode,
                                               DataFormat     data_format,
                                               CRC_Format     crc_format,
                                               bool           enable_crc_checksum,
                                               bool           enable_offset_calibration,
                                               bool           enable_gain_calibration) noexcept;
    CommandT        CreateIRQRegisterValue(bool                enable_conversion_start_interrupt,
                                           bool                enable_fast_command,
                                           IRQ_PinMode         irq_pin_mode,
                                           IRQ_InactivePinMode inactive_irq_pin_mode) noexcept;
    static CommandT CreateMUXRegisterValue(Reference positive_channel, Reference negative_channel) noexcept;
    SPI_CommandT    CreateFirstCommand_IncrementalWrite(CommandT command) noexcept;
    SPI_CommandT    CreateFirstCommand_StaticRead(CommandT command) noexcept;
    void            InterruptInit() noexcept;

  private:
    Byte                                deviceAddress{};
    Gain                                gain;
    std::shared_ptr<OutputStreamBuffer> outputSB;

    size_t streamBufferCapacity;
    size_t streamBufferTriggeringSize;

    bool clockIsEnabled = false;

    std::function<InterruptCallbackT> interruptCallback;
    auto constexpr static interruptPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY;
    auto constexpr static adc_interrupt_pin = 2;
};