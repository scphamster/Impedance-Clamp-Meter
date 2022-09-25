/*
 * keyboard.c
 *
 * Created: 30.10.2021 13:33:16
 *  Author: malygosstationar
 */

#include "asf.h"
#include "keyboard.h"
#include "twi_pdc.h"
#include "menu.h"
#include "system.h"
#include "DSP_functions.h"
#include "MCP23016.h"

#define ENC_STATE_L_ARM (1 << 0)
#define ENC_STATE_R_ARM (1 << 1)
#define ENC_STATE_A_DOWN (1 << 2)
#define ENC_STATE_B_DOWN (1 << 3)

#ifdef __cplusplus
extern "C" {
#endif

uint16_t kbrddd;

uint16_t encoderA_testval;
uint16_t encoderB_testval;
uint16_t encoderL_testval;
uint16_t encoderR_testval;

uint8_t g_Encoder_state = 0;

Keyboard_t Keyboard;

void keyboard_handler(uint16_t keys)
{
	static uint16_t prev_state = 0;
	static uint32_t last_entry_time	= 0;
	uint32_t current_time;
	uint16_t state_changed;

	state_changed = (keys & KEYS_USED_MAP) ^ prev_state;

	if (state_changed) {
		kbrddd |= prev_state & ((~keys) & KEYS_USED_MAP);
		prev_state = (keys & KEYS_USED_MAP);
	}

	Keyboard.keys |= kbrddd;
}

void encoder_fastmode_helper(uint32_t key)
{
	uint32_t time_delta = g_ten_millis - Keyboard.enc_last_action;

	if (Keyboard.enc_fastmode) {
		if (time_delta > ENCODER_FAST_MODE_OUT_THR) {
			Keyboard.enc_fastmode = false;
			Keyboard.enc_hsm_counter = 0;
			Keyboard.enc_superHSM = false;
			Keyboard.enc_superHSM_counter = 0;
		} else if (!Keyboard.enc_superHSM) {
			if (key == KEY_ENCL)
				Keyboard.enc_superHSM_counter--;
			else
				Keyboard.enc_superHSM_counter++;

			if ((Keyboard.enc_superHSM_counter > ENCODER_SHSM_ACTIONS_THR) ||
			    (Keyboard.enc_superHSM_counter < -ENCODER_SHSM_ACTIONS_THR))
				Keyboard.enc_superHSM = true;
		}
	} else {
		if (time_delta < ENCODER_FAST_MODE_IN_THR) {
			if (key == KEY_ENCL)
				Keyboard.enc_hsm_counter--;
			else
				Keyboard.enc_hsm_counter++;

			if ((Keyboard.enc_hsm_counter > ENCODER_HSM_ACTIONS_THR) ||
			    (Keyboard.enc_hsm_counter < -ENCODER_HSM_ACTIONS_THR))
				Keyboard.enc_fastmode = true;
		} else
			Keyboard.enc_hsm_counter = 0;

	}

	Keyboard.enc_last_action = g_ten_millis;
}

void encoder_a_handler(uint32_t id, uint32_t mask)
{
	if (!pio_get_pin_value(ENCODER_A_PIN_NUM)) {
		g_Encoder_state |= ENC_STATE_A_DOWN;

		if (g_Encoder_state & ENC_STATE_B_DOWN) {
			kbrddd |= KEY_ENCL;
			encoder_fastmode_helper(KEY_ENCL);
		}
	} else {
		g_Encoder_state &= ~ENC_STATE_A_DOWN;

		if ((~g_Encoder_state) & ENC_STATE_B_DOWN) {
			kbrddd |= KEY_ENCL;
			encoder_fastmode_helper(KEY_ENCL);
		}
	}
	
	Keyboard.keys |= kbrddd;
}

void encoder_b_handler(uint32_t id, uint32_t mask)
{
	if (!pio_get_pin_value(ENCODER_B_PIN_NUM)) {
		g_Encoder_state |= ENC_STATE_B_DOWN;

		if (g_Encoder_state & ENC_STATE_A_DOWN) {
			kbrddd |= KEY_ENCR;
			encoder_fastmode_helper(KEY_ENCR);
		}
	} else {
		g_Encoder_state &= ~ENC_STATE_B_DOWN;

		if ((~g_Encoder_state) & ENC_STATE_A_DOWN)
			kbrddd |= KEY_ENCR;

		encoder_fastmode_helper(KEY_ENCR);
	}
	
	Keyboard.keys |= kbrddd;
}

void keyboard_encoder_init(void)
{
	const uint32_t encoder_a_pin_mask = ENCODER_A_PIN;
	const uint32_t encoder_b_pin_mask = ENCODER_B_PIN;
	const uint32_t debounceHz = ENCODER_FILTER_DEBOUNCE_HZ;

	pio_set_debounce_filter(PIOA, encoder_a_pin_mask, debounceHz);
	pio_set_debounce_filter(PIOD, encoder_b_pin_mask, debounceHz);

	pio_set_input(PIOA, ENCODER_A_PIN, PIO_DEBOUNCE);
	pio_set_input(PIOD, ENCODER_B_PIN, PIO_DEBOUNCE);

	pio_handler_set(PIOA, ID_PIOA, ENCODER_A_PIN, PIO_IT_EDGE,
	                encoder_a_handler);
	pio_enable_interrupt(PIOA, ENCODER_A_PIN);

	pio_handler_set(PIOD, ID_PIOD, ENCODER_B_PIN, PIO_IT_EDGE,
	                encoder_b_handler);
	pio_enable_interrupt(PIOD, ENCODER_B_PIN);
}

void keyboard_init(void)
{
	keyboard_encoder_init();
	mcp23016_init();
}

#ifdef __cplusplus
}
#endif