/*
 * system.h
 *
 * Created: 02.11.2021 22:20:52
 *  Author: malygosstationar
 */ 


#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	FAULT_NOFAULT = 0,
	FAULT_PS,
	FAULT_AMPLIFIER,
	FAULT_CLAMP,
	FAULT_DISPLAY,
	FAULT_KEYBOARD,
	FAULT_SW_FLASH,
	FAULT_UNKNOWN
}system_error_type_t;

typedef struct {
	bool system_fault :1;
	bool system_hard_fault :1;
	bool system_soft_fault :1;
	
	system_error_type_t system_error_source;
	uint32_t system_error_verb;
	uint32_t system_error_noun;
}System_t;

extern System_t System;

extern uint32_t g_ten_millis;

uint32_t systick_read(void);
void system_fault_set(system_error_type_t source, uint32_t error_verb, uint32_t error_noun);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_H_ */