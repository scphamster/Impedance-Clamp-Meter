#include "asf.h"
#include "system_init.h"
#include "twi_pdc.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t switch_to_plla_result = 0;
//
void master_clock_init(void);
void periph_clock_init(void);
void spi_init(void);
void twi_init(void);

void
watchdog_init(void)
{
    wdt_disable(WDT);
}

void
flash_memory_init(void)
{
    efc_set_wait_state(EFC, FAST_CLOCK_EFC_WSTATE);
    efc_enable_cloe(EFC);
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
    SystemCoreClockUpdate();

}

void
periph_clock_init(void)
{
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOB);
    pmc_enable_periph_clk(ID_PIOD);
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

    spi_init();
    twi_init();
    twiPdc_init();
}


#ifdef __cplusplus
}
#endif