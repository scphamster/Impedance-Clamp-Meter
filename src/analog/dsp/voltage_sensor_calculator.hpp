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

#include <arm_math.h>
#include "signal_conditioning.h"

class VoltageSensorCalculator {
  public:
    using ValueType = float;

    void Calculate(ValueType value);
  protected:
    static float FindAngle(float sine, float cosine, float absval) noexcept
    {
        float        abs_sin;
        float        abs_cos;
        float        degree;
        static float rad2deg_conv_coeff = 180 / PI;

        if (sine < 0)
            abs_sin = -sine;
        else
            abs_sin = sine;

        if (cosine < 0)
            abs_cos = -cosine;
        else
            abs_cos = cosine;

        if (abs_sin < abs_cos)
            degree = acosf(abs_cos / absval) * rad2deg_conv_coeff;
        else
            degree = asinf(abs_sin / absval) * rad2deg_conv_coeff;

        if (sine < 0) {
            if (cosine < 0)
                degree += 180;
            else
                degree = 360 - degree;
        }
        else {
            if (cosine < 0)
                degree = 180 - degree;
        }

        return degree;
    }

  private:
};