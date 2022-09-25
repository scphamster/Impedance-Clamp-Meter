/*
 * system.c
 *
 * Created: 02.11.2021 22:20:40
 *  Author: malygosstationar
 */ 
#include "asf.h"
#include "system.h"
#include "system_init.h"

#ifdef __cplusplus
extern "C" {
#endif

System_t System;
uint32_t g_ten_millis;

void TC0_Handler(void)
{
	g_ten_millis++;
	tc_get_status(TC0, 0);
}

uint32_t systick_read(void)
{
	if (!(SysTick->CTRL & (1 << 0)))
	return 0xffffffff;
	else
	return SysTick->VAL;
}

void system_fault_set(system_error_type_t source, uint32_t error_verb, uint32_t error_noun)
{
	System.system_fault = true;
	
	switch (source) {
	case FAULT_SW_FLASH: {
		System.system_hard_fault = true;
		System.system_error_source = FAULT_SW_FLASH;
		System.system_error_verb = error_verb;
		System.system_error_noun = error_noun;
	}
	break;
	
	default: {
		System.system_hard_fault = true;
		System.system_error_source = FAULT_UNKNOWN;
		System.system_error_verb = error_verb;
		System.system_error_noun = error_noun;
	}
	}
}

#ifdef __cplusplus
}
#endif