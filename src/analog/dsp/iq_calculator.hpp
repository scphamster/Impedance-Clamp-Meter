#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <memory>
#include "dsp_resources.hpp"
#include "arm_math.h"

template<typename ValueT>
class SynchronousIQCalculator {
  public:
    SynchronousIQCalculator(int reset_tick_counter_at_number_of_ticks)
      : tickCounterResetTriggeringLimit{ reset_tick_counter_at_number_of_ticks }
    { }

    std::pair<ValueT, ValueT> CalculateIQ(ValueT fromValue)
    {
        auto retval = std::pair{ sinus_table.at(tickCounter) * fromValue, cosine_table.at(tickCounter) * fromValue };
        tickCounter++;
        // todo: add angle output: find_angle(sine, cosine, absval);

        if (tickCounter == tickCounterResetTriggeringLimit) {
            tickCounter = 0;
        }

        return retval;
    }

    static std::pair<ValueT, ValueT> GetAbsoluteAndDegreeFromIQ(ValueT Ival, ValueT Qval) noexcept
    {
        ValueT abs;
        arm_sqrt_f32(Ival * Ival + Qval * Qval, &abs);

        return { abs, FindAngle(Qval, Ival, abs) };
    }

    void ResetTickCounter() noexcept { tickCounter = 0; }

    static ValueT FindAngle(ValueT Qval, ValueT Ival, ValueT absolute_val)
    {
        ValueT        abs_sin;
        ValueT        abs_cos;
        ValueT        degree;
        static ValueT rad2deg_conv_coeff = 180 / PI;

        if (Qval < 0)
            abs_sin = -Qval;
        else
            abs_sin = Qval;

        if (Ival < 0)
            abs_cos = -Ival;
        else
            abs_cos = Ival;

        if (abs_sin < abs_cos)
            degree = acosf(abs_cos / absolute_val) * rad2deg_conv_coeff;
        else
            degree = asinf(abs_sin / absolute_val) * rad2deg_conv_coeff;

        if (Qval < 0) {
            if (Ival < 0)
                degree += 180;
            else
                degree = 360 - degree;
        }
        else {
            if (Ival < 0)
                degree = 180 - degree;
        }

        return degree;
    }

  private:
    int tickCounter                     = 0;
    int tickCounterResetTriggeringLimit = 0;
};