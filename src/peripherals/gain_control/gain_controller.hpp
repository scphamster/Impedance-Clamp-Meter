#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <functional>
#include <memory>
#include <map>

class GainController {
  public:
    using GainLevelT        = int;
    using GainChangeFunctor = std::function<void()>;
    using GainValueT        = float;
    using PhaseShift        = float;
    using GainAndPhaseShift = std::pair<GainValueT, PhaseShift>;

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
    void SetGainChangeFunctor(GainLevelT for_gain_level, GainChangeFunctor &&new_functor, GainAndPhaseShift gain_and_phase)
    {
        if (for_gain_level > maxGain)
            return;

        gainChangeFunctors[for_gain_level] = std::move(new_functor);
        gainLevelData[for_gain_level]      = { gain_and_phase.first, gain_and_phase.second };
    }
    void SetGainValueAndPhaseShift(GainLevelT for_level, GainValueT gainValue, PhaseShift phaseShift) noexcept
    {
        gainLevelData.at(for_level).gainValue  = gainValue;
        gainLevelData.at(for_level).phaseShift = phaseShift;
    }

    [[nodiscard]] GainLevelT GetMaxGain() const noexcept { return maxGain; }
    [[nodiscard]] GainLevelT GetMinGain() const noexcept { return minGain; }
    [[nodiscard]] GainValueT GetGainValue() const noexcept { return gainLevelData.at(gain).gainValue; }
    [[nodiscard]] PhaseShift GetPhaseShift() const noexcept { return gainLevelData.at(gain).phaseShift; }

  private:
    GainLevelT maxGain = 0;
    GainLevelT minGain = 0;
    GainLevelT gain    = minGain;

    struct GainLevelData {
      public:
        GainValueT gainValue;
        PhaseShift phaseShift;
    };

    std::map<GainLevelT, GainChangeFunctor> gainChangeFunctors;
    std::map<GainLevelT, GainLevelData>     gainLevelData;
};