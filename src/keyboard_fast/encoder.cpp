#include "encoder.hpp"

extern "C" void
encoder_a_handler(uint32_t id, uint32_t mask)
{
    if (!pio_get_pin_value(ENCODER_A_PIN_NUM)) {
        g_Encoder_state |= ENC_STATE_A_DOWN;

        if (g_Encoder_state & ENC_STATE_B_DOWN) {
            kbrddd |= KEY_ENCL;
            encoder_fastmode_helper(KEY_ENCL);
        }
    }
    else {
        g_Encoder_state &= ~ENC_STATE_A_DOWN;

        if ((~g_Encoder_state) & ENC_STATE_B_DOWN) {
            kbrddd |= KEY_ENCL;
            encoder_fastmode_helper(KEY_ENCL);
        }
    }

    Keyboard.keys |= kbrddd;
}

extern "C" void
encoder_b_handler(uint32_t id, uint32_t mask)
{
    if (!pio_get_pin_value(ENCODER_B_PIN_NUM)) {
        g_Encoder_state |= ENC_STATE_B_DOWN;

        if (g_Encoder_state & ENC_STATE_A_DOWN) {
            kbrddd |= KEY_ENCR;
            encoder_fastmode_helper(KEY_ENCR);
        }
    }
    else {
        g_Encoder_state &= ~ENC_STATE_B_DOWN;

        if ((~g_Encoder_state) & ENC_STATE_A_DOWN)
            kbrddd |= KEY_ENCR;

        encoder_fastmode_helper(KEY_ENCR);
    }

    Keyboard.keys |= kbrddd;
}
