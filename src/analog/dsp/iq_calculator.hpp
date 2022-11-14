#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <memory>
#include <complex>

#include "dsp_resources.hpp"
#include "arm_math.h"

template<typename ValueT>
class SynchronousIQCalculator {
  public:
    using IValueT = ValueT;
    using QValueT = ValueT;
    using ComplexT = std::complex<ValueT>;

    SynchronousIQCalculator(int reset_tick_counter_at_number_of_ticks)
      : tickCounterResetTriggeringLimit{ reset_tick_counter_at_number_of_ticks }
    { }

    std::pair<IValueT, QValueT> GetSynchronousIQ(ValueT fromValue)
    {
        auto retval = std::pair{ sinus_table.at(tickCounter) * fromValue, cosine_table.at(tickCounter) * fromValue };
        tickCounter++;

        if (tickCounter == tickCounterResetTriggeringLimit) {
            tickCounter = 0;
        }

        return retval;
    }

    static ComplexT GetIQFromAmplitudeAndPhase(ValueT amplitude, ValueT phase_degree)
    {
        ValueT sine, cosine;

        arm_sin_cos_f32(phase_degree, &sine, &cosine);

        return ComplexT{ amplitude * cosine, amplitude * sine };
    }

    static std::pair<ValueT, ValueT> GetAbsoluteAndDegreeFromIQ(ValueT Ival, ValueT Qval) noexcept
    {
        ValueT abs;
        arm_sqrt_f32(Ival * Ival + Qval * Qval, &abs);

        return { abs, FindAngle(Qval, Ival, abs) };
    }

    void ResetTickCounter() noexcept { tickCounter = 0; }

    static ValueT FindAngle(ValueT Qval, ValueT Ival, ValueT absolute_val) noexcept
    {   // todo: use arm_atan_f32
        ValueT static constexpr rad2deg_conv_coeff = 180.f / PI;

        auto qAbs = Absolute(Qval);
        auto iAbs = Absolute(Ival);

        auto degree = [qAbs, iAbs, absolute_val]() {
            return (qAbs < iAbs) ? acosf(iAbs / absolute_val) * rad2deg_conv_coeff
                                 : asinf(qAbs / absolute_val) * rad2deg_conv_coeff;
        }();

        if (Qval < 0.f) {
            if (Ival < 0.f)
                degree += 180.f;
            else
                degree = 360.f - degree;
        }
        else {
            if (Ival < 0.f)
                degree = 180.f - degree;
        }

        return degree;
    }
    static ValueT NormalizeAngle(ValueT degree) noexcept
    {
        if (degree > 360.f or degree < -360.f) {
            auto full_circle_number = static_cast<int>(degree / 360.f);
            degree                  = degree - 360.f * static_cast<ValueT>(full_circle_number);
        }

        if (degree < 0.f)
            degree += 360.f;

        return degree;
    }
    static std::pair<QValueT, IValueT> GetSinCosFromAngle(ValueT degree)
    {
        ValueT sin, cos;
        arm_sin_cos_f32(degree, &sin, &cos);
        return { sin, cos };
    }
    static ValueT Absolute(ValueT value) { return (value > 0.f) ? value : -value; }

  private:
    int tickCounter                     = 0;
    int tickCounterResetTriggeringLimit = 0;
};