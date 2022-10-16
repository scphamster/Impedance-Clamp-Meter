#include "asf.h"
#include "system_init.h"
#include "system.h"
#include "twi_pdc.h"
#include "LCD1608.h"
#include "keyboard.h"
#include "DSP_functions.h"
#include "external_periph_ctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t test_timer;
uint32_t SystemCoreClock2      = CHIP_FREQ_MAINCK_RC_4MHZ;
uint32_t switch_to_plla_result = 0;
//
void master_clock_init(void);
void periph_clock_init(void);
void ten_milliseconds_timer_init(void);
void dacc_set_divider(Dacc *p_dacc, bool set_divider);
void spi_init(void);
void twi_init(void);
void io_init(void);

void
watchdog_init(void)
{
    wdt_disable(WDT);
}
//
void
flash_memory_init(void)
{
    efc_set_wait_state(EFC, FAST_CLOCK_EFC_WSTATE);
    efc_enable_cloe(EFC);
}

void
io_init(void)
{
    ;
}

void
master_clock_init(void)
{
    uint32_t prescaller;

    pmc_osc_enable_main_xtal(SYS_XTAL_COUNT);

    while (!pmc_osc_is_ready_main_xtal())
        ;

    pmc_switch_mainck_to_xtal(false, SYS_XTAL_COUNT);

    while (!pmc_osc_is_ready_mainck())
        ;

    pmc_enable_pllack(MULA_VAL, PLL_COUNT_VAL, DIVA_VAL);

    while (!pmc_is_locked_pllack())
        ;

    prescaller = (PLL_PRESCALLER_VAL << PMC_MCKR_PRES_Pos);

    switch_to_plla_result = pmc_switch_mck_to_pllack(prescaller);
}

void
periph_clock_init(void)
{
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOB);
    pmc_enable_periph_clk(ID_PIOD);
}

void
buzzer_timer_init(void)
{
    uint32_t clock_source = (3 << 0);
    uint32_t wavsel       = (2 << 13);
    uint32_t wavemode     = (1 << 15);
    uint32_t ACPC_val     = (3 << 18);
    uint32_t burst        = (3 << 4);   // burst with xc2
    uint32_t block        = (3 << 4);   // TIOA1 connected to XC2

    pmc_enable_periph_clk(ID_TC2);
    pmc_enable_periph_clk(ID_TC1);
    tc_set_block_mode(TC0, block);
    tc_init(TC0, 1, clock_source | wavsel | wavemode | ACPC_val);
    tc_init(TC0, 2, clock_source | burst | wavsel | wavemode | ACPC_val);
    tc_write_rc(TC0, 1, 2000);
    tc_write_rc(TC0, 2, 5000);
    tc_start(TC0, 1);
    // buzzer_enable();
}

void
buzzer_enable(void)
{
    pio_set_peripheral(PIOA, PIO_PERIPH_B, (1 << BUZZER_PIN));
    tc_start(BUZZER_TIMER, BUZZER_TIMER_CH);
}

void
buzzer_disable(void)
{
    pio_set_input(PIOA, (1 << BUZZER_PIN), 0);
    tc_stop(BUZZER_TIMER, BUZZER_TIMER_CH);
}

void
buzzer_set_freq(float32_t current)
{
    static uint32_t  current_rc_val  = 0;
    static float32_t k               = 372.5246f;
    static float32_t offst           = 6.6439;
    static uint32_t  last_entry      = 0;
    static uint8_t   lf_buzzer_mode  = 0;
    static uint32_t  lf_buzzer_rcval = 0;

    if (g_ten_millis - last_entry < BUZZER_REFRESH_DELAY)
        return;

    uint32_t  new_rc_val;
    uint32_t  new_lf_rc_val;
    float32_t rc_val_f;
    float32_t rc_lf_val_f;
    float32_t clamp_I_i;

    if (clamp_measurements_result.I_clamp_I < 0)
        clamp_I_i = -clamp_measurements_result.I_clamp_I;
    else
        clamp_I_i = clamp_measurements_result.I_clamp_I;

    rc_val_f    = 50 + k * (-log2f(current) - offst);
    rc_lf_val_f = 10000 + 90e3 * (-log2f(clamp_I_i) - 9.95f);

    if (rc_val_f > BUZZER_FREQ_RC_MAX)
        rc_val_f = BUZZER_FREQ_RC_MAX;
    if (rc_val_f < BUZZER_FREQ_RC_MIN)
        rc_val_f = BUZZER_FREQ_RC_MIN;

    new_rc_val = (uint32_t)rc_val_f;

    if (new_rc_val != current_rc_val) {
        tc_write_rc(BUZZER_TIMER, BUZZER_TIMER_CH, new_rc_val);
        tc_start(BUZZER_TIMER, BUZZER_TIMER_CH);
        current_rc_val = new_rc_val;
        last_entry     = g_ten_millis;
    }

    if (rc_lf_val_f > 100000)
        rc_lf_val_f = 100000;
    if (rc_lf_val_f < 10000)
        rc_lf_val_f = 10000;

    new_lf_rc_val = (uint32_t)rc_lf_val_f;

    if (new_lf_rc_val != lf_buzzer_rcval) {
        tc_write_rc(BUZZER_TIMER, 1, new_lf_rc_val);
        tc_start(BUZZER_TIMER, 1);
        lf_buzzer_rcval = new_lf_rc_val;
    }
}

void
ten_milliseconds_timer_init(void)
{
    uint32_t clock_srce = (3 << 0);    // MCK/128
    uint32_t wavsel     = (2 << 13);   // upmode with trigger on rc compare
    uint32_t wavemode   = (1 << 15);

    pmc_enable_periph_clk(ID_TC0);

    tc_init(TC0, 0, clock_srce | wavsel | wavemode);
    tc_write_rc(TC0, 0, MY_TIMER_RC_VAL);
    tc_enable_interrupt(TC0, 0, (1 << 4));   // enable rc compare interrupt

    NVIC_ClearPendingIRQ(TC0_IRQn);
    NVIC_SetPriority(TC0_IRQn, MY_TIMER_PRIO);
    NVIC_EnableIRQ(TC0_IRQn);

    tc_start(TC0, 0);
}

void
system_tick_init(void)
{
    SysTick->LOAD = 0xffffff;
    SysTick->CTRL = (1 << 0) | (1 << 2);   // enable, clock source = prcsr clock
}

void
dacc_set_divider(Dacc *p_dacc, bool set_divider)
{
    uint32_t mr = p_dacc->DACC_MR & (~DACC_MR_CLKDIV);

    if (set_divider)
        mr |= DACC_MR_CLKDIV;

    p_dacc->DACC_MR = mr;
}

void
dacc_init(void)
{
    pmc_enable_periph_clk(ID_DACC);
    dacc_reset(DACC);
    dacc_set_transfer_mode(DACC, DACC_TRASNSMODE);
    dacc_set_trigger(DACC, DACC_TRGSEL);
    dacc_set_timing(DACC, 0, 63);
    dacc_set_divider(DACC, true);
    dacc_set_channel_selection(DACC, DACC_CHANNELUSED);
    dacc_set_analog_control(DACC, DACC_ACR_VAL);
    dacc_enable_channel(DACC, DACC_CHANNELUSED);
    pio_set_peripheral(PIOA, PIO_PERIPH_C, DATRG_PIN);
}

void
spi_init(void)
{
    pmc_enable_periph_clk(ID_SPI);
    spi_enable(SPI);
    spi_set_baudrate_div(SPI, 0, 4);
    spi_set_master_mode(SPI);
    spi_set_clock_polarity(SPI, 0, 1);
    spi_set_clock_phase(SPI, 0, 0);
    spi_set_peripheral_chip_select_value(SPI, 0x0E);
    spi_configure_cs_behavior(SPI, 0, SPI_CS_KEEP_LOW);
    spi_set_bits_per_transfer(SPI, 0, SPI_CSR_BITS_8_BIT);
    spi_disable_mode_fault_detect(SPI);

    pio_set_peripheral(PIOA, PIO_PERIPH_A, 0X0800);   // NPCS
    pio_set_peripheral(PIOA, PIO_PERIPH_A, 0x1000);   // MISO
    pio_set_peripheral(PIOA, PIO_PERIPH_A, 0X2000);   // MOSI
    pio_set_peripheral(PIOA, PIO_PERIPH_A, 0X4000);   // SPCK
}

void
twi_init(void)
{
    pmc_enable_periph_clk(ID_TWI0);
    pio_set_peripheral(PIOA, PIO_PERIPH_A, (1 << 3));   // I2C_DATA
    pio_set_peripheral(PIOA, PIO_PERIPH_A, (1 << 4));   // I2C_SCL
    static twi_options_t twi_params;

    twi_params.chip       = TWI_CHIP_ADDR;
    twi_params.master_clk = sysclk_get_peripheral_hz();
    twi_params.smbus      = false;
    twi_params.speed      = TWI_CLOCK_SPEED_HZ;

    twi_master_init(TWI0, &twi_params);
}

void
system_init(void)
{
    watchdog_init();
    flash_memory_init();
    master_clock_init();
    periph_clock_init();
    io_init();
    external_periph_ctrl_init();
    system_tick_init();
    keyboard_encoder_init();
    dacc_init();
    spi_init();
    twi_init();
    twiPdc_init();

    ten_milliseconds_timer_init();
    buzzer_timer_init();

    delay_ms(100);

    keyboard_init();
    LCD_init();

    delay_ms(10);

    MCP3462_init();
    dsp_init();
}

#ifdef __cplusplus
}
#endif