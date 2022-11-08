#pragma once

#include "compiler_compatibility_workaround.hpp"
#include <cstdint>
#include <compiler.h>
#include <array>
#include <algorithm>
#include "arm_math.h"

template<typename ValueT, size_t BufferLen>
class DeviationCalculator {
  public:
    void PushBackAndCalculate(ValueT new_value) noexcept
    {
        std::transform(buffer.crbegin() + 1, buffer.crend(), buffer.rbegin(), [](auto &input) { return input; });

        buffer.at(0) = new_value;

        CalculateMean();

        auto dev_squared = FindSumOfDeviationsSquared();
        dev_squared /= buffer.size();

        arm_sqrt_f32(dev_squared, &stdDev);

        relativeStdDev = stdDev / mean;
    }
    ValueT GetStandardDeviation() const noexcept { return stdDev; }
    ValueT GetRelativeStdDev() const noexcept { return relativeStdDev; }

    void   CalculateMean() noexcept
    {
        ValueT sum{ 0 };

        for (auto const &value : buffer) {
            sum += value;
        }

        mean = sum / buffer.size();
    }

    ValueT FindSumOfDeviationsSquared() noexcept
    {
        ValueT squared_deviations{};

        for (auto const &value : buffer) {
            squared_deviations += (value - mean) * (value - mean);
        }

        return squared_deviations;
    }

  private:
    std::array<ValueT, BufferLen> buffer;
    ValueT                        mean;
    ValueT                        stdDev;
    ValueT                        relativeStdDev;
};