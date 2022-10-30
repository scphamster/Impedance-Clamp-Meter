#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include "pio.h"
#include "pio_handler.h"
#include "pmc.h"
#include "spi.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "freertos_handlers.h"
#include "project_configs.hpp"

#include "mcp3462_driver.hpp"

auto constexpr CLOCK_PCK_ID      = 2;
auto constexpr CLOCK_PCK_PRES    = 0;
auto const CLOCK_PIO             = PIOA;
auto constexpr CLOCK_PIO_PIN_MSK = 0x40000UL;
auto constexpr CLOCK_PIO_PERIPH  = PIO_PERIPH_B;

auto constexpr CMD_STATIC_READ       = 0x01;
auto constexpr CMD_INCREMENTAL_WRITE = 0x02;
auto constexpr CMD_INCREMENTAL_READ  = 0x03;

static MCP3462_driver *driver = nullptr;

extern "C" void
mcp3462_interrupt_handler(uint32_t, uint32_t)
{
    portYIELD_FROM_ISR(driver->HandleInterrupt());
}

void
MCP3462_driver::ClockInit() noexcept
{
    pmc_enable_pck(CLOCK_PCK_ID);

    while (!(pmc_is_pck_enabled(CLOCK_PCK_ID))) {
        ;
    }

    if (pmc_switch_pck_to_mainck(CLOCK_PCK_ID, CLOCK_PCK_PRES)) {
        while (1) {   // todo: make better implementation
            ;
        }
    }

    pmc_disable_pck(CLOCK_PCK_ID);
    pio_set_peripheral(CLOCK_PIO, CLOCK_PIO_PERIPH, CLOCK_PIO_PIN_MSK);
}

void
MCP3462_driver::Initialize() noexcept   // todo: make configurable
{
    driver = this;

    uint16_t config0_regval = 0;
    uint16_t config1_regval = 0;
    uint16_t config2_regval = 0;
    uint16_t config3_regval = 0;
    uint16_t irq_regval     = 0;
    uint16_t mux_regval     = 0;
    uint16_t first_word     = 0;
    uint16_t retval1        = 0;
    uint16_t retval2        = 0;

    ClockInit();

    config0_regval =
      CreateConfig0RegisterValue(false, ClockSelection::CLK_SEL_EXT, CurrentSource::CS_SEL_0, ADC_MODE::Conversion);

    config1_regval = CreateConfig1RegisterValue(Prescaler::PRE_0, OSR::OSR_256);
    config2_regval = CreateConfig2RegisterValue(Boost::BOOST_2, Gain::GAIN_1, AutoZeroingMuxMode::Disabled);
    config3_regval = CreateConfig3RegisterValue(ConversionMode::CONV_MODE_CONT,
                                                DataFormat::Default16Bit,
                                                CRC_Format::CRC_16Only,
                                                false,
                                                false,
                                                false);
    irq_regval     = CreateIRQRegisterValue(false, false, IRQ_PinMode::IRQ, IRQ_InactivePinMode::LogicHigh);
    mux_regval     = CreateMUXRegisterValue(Reference::CH4, Reference::CH5);
    first_word     = CreateFirstCommand_IncrementalWrite(static_cast<CommandT>(Register::Config0));

    spi_write(SPI, first_word, 0, 0);
    spi_read(SPI, &retval1, nullptr);
    spi_write(SPI, config0_regval, 0, 0);
    spi_write(SPI, config1_regval, 0, 0);
    spi_write(SPI, config2_regval, 0, 0);
    spi_write(SPI, config3_regval, 0, 0);
    spi_write(SPI, irq_regval, 0, 0);
    spi_write(SPI, mux_regval, 0, 0);
    spi_set_lastxfer(SPI);

    InterruptInit();
}

Byte
MCP3462_driver::CreateConfig0RegisterValue(bool                           if_full_shutdown,
                                           MCP3462_driver::ClockSelection clock,
                                           MCP3462_driver::CurrentSource  current_source,
                                           MCP3462_driver::ADC_MODE       adc_mode) noexcept
{
    auto constexpr CONFIG0_REG_ADDR = 0x1;
    auto constexpr CONFIG0_POS      = 0x6;
    auto constexpr CONFIG0_MSK      = 0xC0;
    auto constexpr CLK_SEL_POS      = 0x4;
    auto constexpr CLK_SEL_MSK      = 0x30;
    auto constexpr CS_SEL_POS       = 0x2;
    auto constexpr CS_SEL_MSK       = 0xC;
    auto constexpr ADC_MODE_POS     = 0x0;
    auto constexpr ADC_MODE_MSK     = 0x3;

    return (ADC_MODE_MSK bitand (static_cast<Byte>(adc_mode) << ADC_MODE_POS)) bitor
           (CS_SEL_MSK bitand (static_cast<std::underlying_type_t<CurrentSource>>(current_source) << CS_SEL_POS)) bitor
           (CLK_SEL_MSK bitand (static_cast<std::underlying_type_t<ClockSelection>>(clock) << CLK_SEL_POS)) bitor
           (CONFIG0_MSK bitand (static_cast<Byte>(if_full_shutdown) << CONFIG0_POS));
}
Byte
MCP3462_driver::CreateConfig1RegisterValue(MCP3462_driver::Prescaler prescaler,
                                           MCP3462_driver::OSR       oversampling_ratio) noexcept
{
    auto constexpr PRE_POS = 0x6;
    auto constexpr PRE_MSK = 0xC0;
    auto constexpr OSR_POS = 0x2;
    auto constexpr OSR_MSK = 0x3C;

    return (PRE_MSK bitand (static_cast<std::underlying_type_t<Prescaler>>(prescaler) << PRE_POS)) bitor
           (OSR_MSK bitand (static_cast<std::underlying_type_t<OSR>>(oversampling_ratio) << OSR_POS));
}
Byte
MCP3462_driver::CreateConfig2RegisterValue(MCP3462_driver::Boost              boost_value,
                                           MCP3462_driver::Gain               gain,
                                           MCP3462_driver::AutoZeroingMuxMode autozero) noexcept
{
    auto constexpr BOOST_POS    = 0x6;
    auto constexpr BOOST_MSK    = 0xC0;
    auto constexpr GAIN_POS     = 0x3;
    auto constexpr GAIN_MSK     = 0x38;
    auto constexpr AZ_MUX_POS   = 0x2;
    auto constexpr AZ_MUX_MSK   = 0x4;
    auto constexpr overall_mask = 0xfe;

    return ((BOOST_MSK bitand (static_cast<std::underlying_type_t<Boost>>(boost_value) << BOOST_POS)) bitor
            (GAIN_MSK bitand (static_cast<std::underlying_type_t<Gain>>(gain) << GAIN_POS)) bitor
            (AZ_MUX_MSK bitand (static_cast<std::underlying_type_t<AutoZeroingMuxMode>>(autozero) << AZ_MUX_POS))) bitand
           overall_mask;
}
Byte
MCP3462_driver::CreateConfig3RegisterValue(ConversionMode conversion_mode,
                                           DataFormat     data_format,
                                           CRC_Format     crc_format,
                                           bool           enable_crc_checksum,
                                           bool           enable_offset_calibration,
                                           bool           enable_gain_calibration) noexcept
{
    auto constexpr CONV_MODE_POS   = 0x6;
    auto constexpr CONV_MODE_MSK   = 0xC0;
    auto constexpr DATA_FORMAT_POS = 0x4;
    auto constexpr DATA_FORMAT_MSK = 0x30;
    auto constexpr CRC_FORMAT_POS  = 0x3;
    auto constexpr CRC_FORMAT_MSK  = 0x8;
    auto constexpr EN_CRCCOM_POS   = 0x2;
    auto constexpr EN_CRCCOM_MSK   = 0x4;
    auto constexpr EN_OFFCAL_POS   = 0x1;
    auto constexpr EN_OFFCAL_MSK   = 0x2;
    auto constexpr EN_GAINCAL_POS  = 0x0;
    auto constexpr EN_GAINCAL_MSK  = 0x1;

    // clang-format off
    return (CONV_MODE_MSK bitand (static_cast<std::underlying_type_t<ConversionMode>>(conversion_mode) << CONV_MODE_POS)) bitor
           (DATA_FORMAT_MSK bitand (static_cast<std::underlying_type_t<DataFormat>>(data_format) << DATA_FORMAT_POS)) bitor
           (EN_CRCCOM_MSK bitand (static_cast<std::underlying_type_t<CRC_Format>>(crc_format) << CRC_FORMAT_POS)) bitor
           (EN_CRCCOM_MSK bitand (static_cast<Byte>(enable_crc_checksum) << EN_CRCCOM_POS)) bitor
           (EN_OFFCAL_MSK bitand (static_cast<Byte>(enable_offset_calibration) << EN_OFFCAL_POS)) bitor
           (EN_GAINCAL_MSK bitand (static_cast<Byte>(enable_gain_calibration) << EN_GAINCAL_POS));
    // clang-format on
}
Byte
MCP3462_driver::CreateIRQRegisterValue(bool                enable_conversion_start_interrupt,
                                       bool                enable_fast_command,
                                       IRQ_PinMode         irq_pin_mode,
                                       IRQ_InactivePinMode inactive_irq_pin_mode) noexcept
{
    auto constexpr WRITE_MASK                   = 0x0f;
    auto constexpr conversion_start_int_mask    = 0x01;
    auto constexpr conversion_start_int_pos     = 0;
    auto constexpr fast_cmd_mask                = 0x02;
    auto constexpr fast_cmd_pos                 = 1;
    auto constexpr irq_inactive_pin_config_mask = 0x04;
    auto constexpr irq_inactive_pin_config_pos  = 2;
    auto constexpr irq_pin_mode_mask            = 0x08;
    auto constexpr irq_pin_mode_pos             = 3;

    return (conversion_start_int_mask bitand
            (static_cast<Byte>(enable_conversion_start_interrupt) << conversion_start_int_pos)) bitor
           (fast_cmd_mask bitand (static_cast<Byte>(enable_fast_command) << fast_cmd_pos)) bitor
           (irq_inactive_pin_config_mask bitand
            (static_cast<std::underlying_type_t<IRQ_InactivePinMode>>(inactive_irq_pin_mode)
             << irq_inactive_pin_config_pos)) bitor
           (irq_pin_mode_mask bitand (static_cast<std::underlying_type_t<IRQ_PinMode>>(irq_pin_mode))
                                       << irq_pin_mode_pos);
}

Byte
MCP3462_driver::CreateMUXRegisterValue(MCP3462_driver::Reference positive_channel,
                                       MCP3462_driver::Reference negative_channel) noexcept
{
    auto constexpr positive_channel_pos  = 4;
    auto constexpr positive_channel_mask = 0xf0;
    auto constexpr negative_channel_mask = 0x0f;
    auto constexpr negative_channel_pos  = 0;

    return (positive_channel_mask bitand
            (static_cast<std::underlying_type_t<Reference>>(positive_channel) << positive_channel_pos)) bitor
           (negative_channel_mask bitand
            (static_cast<std::underlying_type_t<Reference>>(negative_channel) << negative_channel_pos));
}

MCP3462_driver::SPI_CommandT
MCP3462_driver::CreateFirstCommand_IncrementalWrite(MCP3462_driver::CommandT command) noexcept
{
    auto constexpr CMD_ADDR_POS = 0x2;
    auto constexpr CMD_ADDR_MSK = 0x3C;

    return deviceAddress bitor (CMD_ADDR_MSK bitand (command << CMD_ADDR_POS)) bitor CMD_INCREMENTAL_WRITE;
}

MCP3462_driver::SPI_CommandT
MCP3462_driver::CreateFirstCommand_StaticRead(MCP3462_driver::CommandT command) noexcept
{
    auto constexpr CMD_ADDR_POS = 0x2;
    auto constexpr CMD_ADDR_MSK = 0x3C;

    return deviceAddress bitor (CMD_ADDR_MSK bitand (command << CMD_ADDR_POS)) bitor CMD_STATIC_READ;
}

MCP3462_driver::MCP3462_driver(MCP3462_driver::AddressT device_address /* = 0x40*/, size_t queue_size /*= 200*/)
  : deviceAddress{ device_address }
  , queueSize{ queue_size }
  , data_queue{ xQueueCreate(queue_size, QueueMsgSize) }
{
    if (data_queue == nullptr)
        std::terminate();   // todo: make exception handling

    Initialize();
}

MCP3462_driver::~MCP3462_driver()
{
    DeleteQueue();
}

void
MCP3462_driver::EnableClock(bool if_enable) noexcept
{
    if (if_enable)
        pmc_enable_pck(CLOCK_PCK_ID);
    else
        pmc_disable_pck(CLOCK_PCK_ID);
}

void
MCP3462_driver::InterruptInit() noexcept
{
    pio_handler_set(PIOA, ID_PIOA, (1 << adc_interrupt_pin), PIO_IT_FALL_EDGE, mcp3462_interrupt_handler);
    pio_handler_set_priority(PIOA, PIOA_IRQn, interruptPriority);
}

MCP3462_driver::ValueT
MCP3462_driver::Read() noexcept
{
    auto constexpr spi_timeout_retval = 12345;
    uint32_t timeout_counter          = SPI_TIMEOUT;
    uint8_t  firstbyte                = 0;
    uint16_t readout                  = 0;
    int32_t  retval                   = 0;

    uint16_t data0 = 0;
    uint16_t data1 = 0;
    uint16_t data2 = 0;

    firstbyte = CreateFirstCommand_StaticRead(static_cast<CommandT>(Register::ADC_Data));

    while ((!spi_is_tx_ready(SPI)) && (timeout_counter--))
        ;

    if (!timeout_counter)
        return spi_timeout_retval;

    spi_set_bits_per_transfer(SPI, 0, SPI_CSR_BITS_8_BIT);
    spi_configure_cs_behavior(SPI, 0, SPI_CS_KEEP_LOW);

    spi_write(SPI, firstbyte, 0, 0);
    spi_read(SPI, &readout, NULL);
    spi_write(SPI, 0, 0, 0);
    spi_read(SPI, &data0, NULL);
    spi_write(SPI, 0, 0, 0);
    spi_read(SPI, &data1, NULL);
    spi_configure_cs_behavior(SPI, 0, SPI_CS_RISE_FORCED);
    spi_write(SPI, 0, 0, 0);
    spi_read(SPI, &data2, NULL);
    spi_set_lastxfer(SPI);

    retval = data2 | (data1 << 8) | (data0 << 16);

    // sign bit
    if (retval & (1 << 23)) {
        retval--;
        retval = -(~retval & (0xffffff));
    }

    return retval;
}

BaseType_t
MCP3462_driver::HandleInterrupt() noexcept
{
    BaseType_t higher_prio_task_woken;

    // todo test
    auto adc_value = Read();
    //    auto adc_value = 153;
    // end test

    if (xQueueSendFromISR(data_queue, &adc_value, &higher_prio_task_woken) == errQUEUE_FULL) {
        QueueFullHook(xTaskGetCurrentTaskHandle(), "adc interrupt handler");
    };

    return higher_prio_task_woken;
}

void
MCP3462_driver::SetDataInterruptCallback(MCP3462_driver::InterruptCallbackT &&new_callback) noexcept
{
    interruptCallback = std::move(new_callback);
}
QueueHandle_t
MCP3462_driver::GetDataQueue() const noexcept
{
    return data_queue;
}

void
MCP3462_driver::SetMux(MCP3462_driver::Reference positive_channel, MCP3462_driver::Reference negative_channel) noexcept
{
    uint32_t timeout_counter = SPI_TIMEOUT;

    // todo: why this is here?
    if ((positive_channel > Reference::CH5) || (negative_channel > Reference::CH5))
        return;

    auto first_word = CreateFirstCommand_IncrementalWrite(static_cast<std::underlying_type_t<Register>>(Register::MUX));
    auto value_word = CreateMUXRegisterValue(positive_channel, negative_channel);

    while ((!spi_is_tx_ready(SPI)) && (timeout_counter--))
        ;

    if (!timeout_counter)
        return;

    spi_configure_cs_behavior(SPI, 0, SPI_CS_RISE_FORCED);
    spi_set_bits_per_transfer(SPI, 0, SPI_CSR_BITS_16_BIT);

    spi_write(SPI, (first_word << 8) | value_word, 0, 0);
    spi_set_lastxfer(SPI);
}
void
MCP3462_driver::EnableInterrupt(bool if_enable) noexcept
{
    const uint32_t int_pin = (1 << adc_interrupt_pin);
    NVIC_ClearPendingIRQ(PIOA_IRQn);
    pio_enable_interrupt(PIOA, int_pin);
}
void
MCP3462_driver::StartMeasurement() noexcept
{
    EnableInterrupt();
    EnableClock();
}
void
MCP3462_driver::StopMeasurement() noexcept
{
    EnableInterrupt(false);
    EnableClock(false);
}
void
MCP3462_driver::SetGain(MCP3462_driver::Gain new_gain) noexcept
{
    size_t   timeout_counter = SPI_TIMEOUT;
    CommandT initWord;
    CommandT config2Word;
    CommandT retval;

    auto constexpr config2_reg_addr = 0x3;

    initWord    = CreateFirstCommand_IncrementalWrite(config2_reg_addr);
    config2Word = CreateConfig2RegisterValue(Boost::BOOST_2, new_gain, AutoZeroingMuxMode::Disabled);

    while ((!spi_is_tx_ready(SPI)) && (timeout_counter--))
        ;

    if (!timeout_counter)
        return;

    spi_configure_cs_behavior(SPI, 0, SPI_CS_RISE_FORCED);
    spi_set_bits_per_transfer(SPI, 0, SPI_CSR_BITS_16_BIT);

    spi_write(SPI, (initWord << 8) | config2Word, 0, 0);
    spi_set_lastxfer(SPI);
}
void
MCP3462_driver::SetOutputQueue(MCP3462_driver::QueueT new_output_queue) noexcept
{
    DeleteQueue();
    data_queue = new_output_queue;
}
void
MCP3462_driver::DeleteQueue() noexcept
{
    if (data_queue != nullptr) {
        vQueueDelete(data_queue);
    }
}

MCP3462_driver::QueueT
MCP3462_driver::CreateNewOutputQueue(size_t new_queue_size) noexcept
{
    queueSize = new_queue_size;
    return xQueueCreate(new_queue_size, QueueMsgSize);
}

MCP3462_driver::QueueT
MCP3462_driver::CreateNewOutputQueue() noexcept
{
    return CreateNewOutputQueue(queueSize);
}
