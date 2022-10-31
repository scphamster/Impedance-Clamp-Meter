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
        //todo: add angle output: find_angle(sine, cosine, absval);

        if (tickCounter == tickCounterResetTriggeringLimit) {
            tickCounter = 0;
        }

        return retval;
    }
    void ResetTickCounter() noexcept { tickCounter = 0; }

  private:
    int tickCounter                     = 0;
    int tickCounterResetTriggeringLimit = 0;
};