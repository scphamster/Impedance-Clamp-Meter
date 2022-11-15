#include "tc.h"
#include "pmc.h"
#define PERFMON_TIMER_RC_VAL       93   // 100us if mclck = 120MHz
#define PERFMON_TIMER_INT_PRIO     11
#define PERFMON_TIMER_DEVICE       TC0
#define PERFMON_TIMER_DEVICE_ID ID_TC0
uint32_t freertos_perfmon_timer_T100us = 0;

void
TC0_Handler(void)
{
    freertos_perfmon_timer_T100us++;
    tc_get_status(TC0, 0);
}

void
freertos_perfmon_timer_init(void)
{
    uint32_t clock_srce = (3 << 0);    // MCK/128
    uint32_t wavsel     = (2 << 13);   // upmode with trigger on rc compare
    uint32_t wavemode   = (1 << 15);

    pmc_enable_periph_clk(PERFMON_TIMER_DEVICE_ID);

    tc_init(TC0, 0, clock_srce | wavsel | wavemode);
    tc_write_rc(TC0, 0, PERFMON_TIMER_RC_VAL);
    tc_enable_interrupt(TC0, 0, (1 << 4));   // enable rc compare interrupt

    NVIC_ClearPendingIRQ(TC0_IRQn);
    NVIC_SetPriority(TC0_IRQn, PERFMON_TIMER_INT_PRIO);
    NVIC_EnableIRQ(TC0_IRQn);

    tc_start(TC0, 0);
}