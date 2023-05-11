#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <memory>
#include "asf.h"

class ClampSensor {
  public:
    using PinT  = uint32_t;
    using GainT = uint8_t;

    static std::shared_ptr<ClampSensor> Get() noexcept
    {
        if (_this)
            return _this;
        else {
            Create();
            return _this;
        }
    }

    void SetGain(GainT gain) noexcept
    {
        switch (gain) {
        case 0: {
            /*turn off all pull_up*/
            pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, false);

            pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);

            /* turn on all pull_down */
            pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);

            pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
        } break;

        case 1: {
            /*turn off pull_down at A, turn off pull_up at B,C,D*/
            pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);
            pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, false);

            /* turn on pull_up at A, turn on pull_down at B,C,D */
            pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
            pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);
        } break;

        case 2: {
            /*turn off pull_down at B, turn off pull_up at A,C,D*/
            pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN, false);
            pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, false);
            pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);

            /* turn on pull_up at B, turn on pull_down at A,C,D */
            pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN, true);
            pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
            pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);
        } break;

        case 3:
        case 4: {
            /*turn off pull_down at D, turn off pull_up at A,B,C*/
            pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_D_PIN, false);
            pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN, false);
            pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);

            /* turn on pull_up at D, turn on pull_down at A,B,C*/
            pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_D_PIN, true);
            pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
            pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN, true);
        } break;
        }
    }

  protected:
    static void Create() noexcept { _this = std::shared_ptr<ClampSensor>(new ClampSensor{}); }

  private:
    ClampSensor() = default;

    static std::shared_ptr<ClampSensor> _this;

    static constexpr PinT CLAMP_SENSOR_GAIN_A_PIN = (1 << 30);
    static constexpr PinT CLAMP_SENSOR_GAIN_B_PIN = (1 << 7);
    static constexpr PinT CLAMP_SENSOR_GAIN_C_PIN = (1 << 8);
    static constexpr PinT CLAMP_SENSOR_GAIN_D_PIN = (1 << 22);
};
