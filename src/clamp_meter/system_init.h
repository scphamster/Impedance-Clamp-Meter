/*
 * sys_config.h
 *
 * Created: 05.09.2021 20:53:10
 *  Author: malygosstationar
 */

#pragma once

#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#include "arm_math.h"

#define MAINXTAL_FREQ         20000000UL /*20MHz*/
#define MULA_VAL              11
#define PLL_COUNT_VAL         63
#define DIVA_VAL              1
#define SYS_XTAL_COUNT        8
#define FAST_CLOCK_EFC_WSTATE 5
#define PLL_PRESCALLER_VAL    2

#define TWI_CLOCK_SPEED_HZ 40000UL
#define TWI_CHIP_ADDR      0
#define DATRG_PIN          (1 << 2)
#define DACC_TRGSEL        0
#define DACC_TRASNSMODE    0
#define DACC_CHANNELUSED   0
#define DACC_ACR_VAL       0x103

#define MY_TIMER_RC_VAL 9375
#define MY_TIMER_PRIO   3

#define BUZZER_PIN           26
#define BUZZER_TIMER         TC0
#define BUZZER_TIMER_CH      2
#define BUZZER_FREQ_RC_MAX   5000
#define BUZZER_FREQ_RC_MIN   50
#define BUZZER_REFRESH_DELAY 10
#define BUZZER_LF_MIN        100000
#define BUZZER_LF_MIDDLE     50000
#define BUZZER_LF_MAX        20000

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t test_timer;

void buzzer_enable(void);
void buzzer_disable(void);
void buzzer_set_freq(float32_t current);
void system_init(void);
void dacc_init(void);

#ifdef __cplusplus
}
#endif