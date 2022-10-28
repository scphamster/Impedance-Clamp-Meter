#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <functional>
#include <memory>
#include <map>

class GainController {
  public:
    using GainLevelT        = int;
    using GainChangeFunctor = std::function<void()>;

    GainController(GainLevelT new_min_gain, GainLevelT new_max_gain)
      : maxGain(new_max_gain)
      , minGain(new_min_gain)
    { }

    void SetGain(GainLevelT new_gain) noexcept
    {
        if (new_gain > maxGain)
            new_gain = maxGain;

        if (new_gain == gain)
            return;

        if (gainChangeFunctors.contains(new_gain)) {
            gainChangeFunctors.at(new_gain)();
            gain = new_gain;
        }
    }
    void IncreaseGain() noexcept { SetGain(gain + 1); }
    void DecreaseGain() noexcept { SetGain(gain - 1); }
    void SetGainChangeFunctor(GainLevelT for_gain_level, GainChangeFunctor &&new_functor)
    {
        if (for_gain_level > maxGain)
            return;

        gainChangeFunctors[for_gain_level] = std::move(new_functor);
    }

  private:
    GainLevelT maxGain = 0;
    GainLevelT minGain = 0;
    GainLevelT gain    = minGain;

    std::map<GainLevelT, GainChangeFunctor> gainChangeFunctors;
};