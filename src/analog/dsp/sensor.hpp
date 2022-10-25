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

#include <array>

class Sensor {
    using ValueT = float;

  private:
    ValueT rawValue_I{};
    ValueT rawValue_Q{};
    ValueT rawAbsoluteValue{};
    ValueT rawDegree{};

    ValueT trueValue_I{};
    ValueT trueValue_Q{};
    ValueT trueAbsoluteValue{};
    ValueT trueDegree{};

    std::array<ValueT, GainSteps> gainValues;
    int gainStep;

};

