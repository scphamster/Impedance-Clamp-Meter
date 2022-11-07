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
    ValueT PushBackAndGetRelativeStdDeviation(ValueT new_value) noexcept
    {
        std::transform(buffer.begin() + 1, buffer.end(), buffer.begin(), [](auto &input) { return input; });

        *(buffer.end() - 1) = new_value;

        CalculateMean();

        auto dev_squared = FindSumOfDeviationsSquared();
        dev_squared /= buffer.size();
        ValueT std_deviation;
        arm_sqrt_f32(dev_squared, &std_deviation);

        return std_deviation / mean;
    }

    void CalculateMean() noexcept
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
};