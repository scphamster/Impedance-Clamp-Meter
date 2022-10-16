#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include "asf.h"
#include "pio.h"
#include "pin.hpp"

extern "C" void encoder_a_handler(uint32_t id, uint32_t mask);
extern "C" void encoder_b_handler(uint32_t id, uint32_t mask);

auto
convert_pio_to_id(Pio *pio_of_interest)
{
    if (pio_of_interest == PIOA)
        return ID_PIOA;
    else if (pio_of_interest == PIOB)
        return ID_PIOB;
    else if (pio_of_interest == PIOD)
        return ID_PIOD;
    else
        return -1;
}

class Encoder {
  public:
    Encoder(int  _left_leg_pin_num  = 1,
            int  _right_leg_pin_num = 13,
            Pio *_left_leg_pin_PIO  = PIOA,
            Pio *_right_leg_pin_PIO = PIOD)
      : left_leg_pin_num{ _left_leg_pin_num }
      , right_leg_pin_num{ _right_leg_pin_num }
      , left_leg_pin_PIO(_left_leg_pin_PIO)
      , right_leg_pin_PIO(_right_leg_pin_PIO)
    {
        auto encoder_a_pin_mask = (1 << left_leg_pin_num);
        auto encoder_b_pin_mask = (1 << right_leg_pin_num);

        pio_set_debounce_filter(left_leg_pin_PIO, encoder_a_pin_mask, encoder_filter_debounce_hz);
        pio_set_debounce_filter(right_leg_pin_PIO, encoder_b_pin_mask, encoder_filter_debounce_hz);

        pio_set_input(left_leg_pin_PIO, encoder_a_pin_mask, pio_attribute);
        pio_set_input(right_leg_pin_PIO, encoder_b_pin_mask, pio_attribute);

        pio_handler_set(left_leg_pin_PIO,
                        convert_pio_to_id(left_leg_pin_PIO),
                        encoder_a_pin_mask,
                        interruptDetectionType,
                        encoder_a_handler);
        pio_enable_interrupt(left_leg_pin_PIO, encoder_b_pin_mask);

        pio_handler_set(right_leg_pin_PIO,
                        convert_pio_to_id(right_leg_pin_PIO),
                        encoder_a_pin_mask,
                        interruptDetectionType,
                        encoder_b_handler);
        pio_enable_interrupt(right_leg_pin_PIO, encoder_b_pin_mask);
    }

  protected:
  private:
    int  left_leg_pin_num{ 0 };
    int  right_leg_pin_num{ 0 };
    Pio *left_leg_pin_PIO                        = nullptr;
    Pio *right_leg_pin_PIO                       = nullptr;
    auto static constexpr interruptDetectionType = PIO_IT_EDGE;

    auto static constexpr enc_state_l_arm            = (1 << 0);
    auto static constexpr enc_state_r_arm            = (1 << 1);
    auto static constexpr enc_state_a_down           = (1 << 2);
    auto static constexpr enc_state_b_down           = (1 << 3);
    auto static constexpr encoder_filter_debounce_hz = 300;
    auto static constexpr encoder_a_pin_num          = 1;
    auto static constexpr encoder_b_pin_num          = 32 * 3 + 13;
    auto static constexpr pio_attribute              = PIO_DEBOUNCE;
};