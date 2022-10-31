#include "tc.h"
#include "pmc.h"
#define FREERTOS_TIMESTATS_100US 93
#define MY_TIMER_PRIO   11

uint32_t g_100us = 0;

void TC0_Handler(void)
{
    //	g_ten_millis++;
    g_100us++;
    tc_get_status(TC0, 0);
}

void freertos_timestat_timer_init(void) {
    uint32_t clock_srce = (3 << 0);    // MCK/128
    uint32_t wavsel     = (2 << 13);   // upmode with trigger on rc compare
    uint32_t wavemode   = (1 << 15);

    pmc_enable_periph_clk(ID_TC0);

    tc_init(TC0, 0, clock_srce | wavsel | wavemode);
    tc_write_rc(TC0, 0, FREERTOS_TIMESTATS_100US);
    tc_enable_interrupt(TC0, 0, (1 << 4));   // enable rc compare interrupt

    NVIC_ClearPendingIRQ(TC0_IRQn);
    NVIC_SetPriority(TC0_IRQn, MY_TIMER_PRIO);
    NVIC_EnableIRQ(TC0_IRQn);

    tc_start(TC0, 0);
}