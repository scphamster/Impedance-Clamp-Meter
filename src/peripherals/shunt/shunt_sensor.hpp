#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <memory>
#include "asf.h"

/***
 * @brief SingleTone class for shunt sensor control
 */
class ShuntSensor {
public:
    using GainT = uint8_t;
    using PinT = uint32_t;

    static std::shared_ptr<ShuntSensor> Get() noexcept {
        if (_this) return _this;
        else {
            Create();
            return _this;
        }
    }

    void SetGain(GainT new_gain) noexcept {
        switch (new_gain) {
            case 0: {
                pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, 0);

                pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, 0);

                pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);

                pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
            } break;

            case 1: {
                pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN, false);
                pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN, true);

                pio_pull_down(PIOD, SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, false);
                pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, false);
                pio_pull_up(PIOD, SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
                pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
            } break;

            case 2: {
                pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, false);
                pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN, false);
                pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, true);
                pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN, true);

                pio_pull_down(PIOD, SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, false);
                pio_pull_up(PIOD, SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
            } break;

            case 3: {
                pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, false);
                pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_D_PIN, false);
                pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, true);
                pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_D_PIN, true);

                pio_pull_down(PIOD, SH_SENSOR_GAIN_C_PIN, false);
                pio_pull_up(PIOD, SH_SENSOR_GAIN_C_PIN, true);
            } break;

            case 4: {
                pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, false);
                pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, false);
                pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, true);
                pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
            } break;

            // should never get here!
            default: std::terminate();
        }
    }


protected:
    static void Create() noexcept {
        _this = std::shared_ptr<ShuntSensor>(new ShuntSensor{});
        Init();
    }
    static void Init() noexcept {
        pio_set_input(PIOA, SH_SENSOR_GAIN_A_PIN, 0);
        pio_set_input (PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN, 0);

        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
    }


private:
    ShuntSensor() = default;
    static std::shared_ptr<ShuntSensor> _this;

    static constexpr PinT SH_SENSOR_GAIN_A_PIN	= (1 << 5);
    static constexpr PinT SH_SENSOR_GAIN_B_PIN	= (1 << 17);
    static constexpr PinT SH_SENSOR_GAIN_C_PIN	= (1 << 16);
    static constexpr PinT SH_SENSOR_GAIN_D_PIN	= (1 << 14);
};