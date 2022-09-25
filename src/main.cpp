#define BOARD_FREQ_SLCK_XTAL     32768UL
#define BOARD_FREQ_SLCK_BYPASS   32768UL
#define BOARD_FREQ_MAINCK_XTAL   20000000UL
#define BOARD_FREQ_MAINCK_BYPASS 20000000UL
#define BOARD_OSC_STARTUP_US     15625UL

#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max

#include <asf.h>
#include "system_init.h"
#include "DSP_functions.h"
#include "MCP23016.h"
#include "signal_conditioning.h"
#include "ILI9486_public.h"
#include "menu_ili9486_kbrd_mngr.h"
#include "menu_ili9486.h"

#ifndef ADC_TEST_DEF
void adc_interrupt_init(void)
{
    pio_handler_set(PIOA, ID_PIOA, (1 << ADC_INTERRUPT_PIN), PIO_IT_FALL_EDGE,
                    adc_interrupt_handler);
    pio_handler_set_priority(PIOA, PIOA_IRQn, 1);
    pio_enable_interrupt(PIOA, (1 << ADC_INTERRUPT_PIN));
}
#endif

int main ()
{
#ifdef __TEST__
    uint32_t filter_data_counter = 0;
    uint32_t last_display_timebuff = 0;
    float32_t rad2deg_conv_coeff = 180 / PI;

    decimator_databuff_is_ready = false;
    float32_t shunt_current = 0;

    if (mytestfunc() > 3) {
        while(true){

        }
    }
#else
    system_init();

    TFT_Init();
    TFT_Clear(COLOR_BLACK);
    display_init();

#ifndef ADC_TEST_DEF
    adc_interrupt_init();
#endif

    mcp23016_read_pindata();
    delay_ms(30);

    while (1) {
        if (Analog.generator_is_active) {
            dsp_integrating_filter();

            if (Dsp.new_data_is_ready) {
                Dsp.new_data_is_ready = false;
                display_refresh();
            }

            if ((Calibrator.is_calibrating) && (Calibrator.new_data_is_ready)) {
                calibration_display_measured_data();
                Calibrator.new_data_is_ready = false;
            }

        }

        kbrd_manager();
    }
#endif
}
