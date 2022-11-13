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
        configASSERT(new_gain <= maxGain and new_gain >= minGain);

        // todo: reimplement
        //         if (new_gain == gain)
        //             return;

        configASSERT(gainChangeFunctors.contains(new_gain));

        gainChangeFunctors.at(new_gain)();
        gain = new_gain;
    }
    void SetGainForce(GainLevelT new_gain) noexcept
    {
        // todo implement
    }
    void IncreaseGain() noexcept
    {
        if (gain < maxGain)
            SetGain(gain + 1);
    }
    void DecreaseGain() noexcept
    {
        if (gain > minGain)
            SetGain(gain - 1);
    }
    void SetGainChangeFunctor(GainLevelT for_gain_level, GainChangeFunctor &&new_functor, GainAndPhaseShift gain_and_phase)
    {
        configASSERT(for_gain_level <= maxGain and for_gain_level >= minGain);

        gainChangeFunctors[for_gain_level] = std::move(new_functor);
        gainLevelData[for_gain_level]      = { gain_and_phase.first, gain_and_phase.second };
    }
    void SetGainValueAndPhaseShift(GainLevelT for_level, GainValueT gainValue, PhaseShift phaseShift) noexcept
    {
        gainLevelData.at(for_level).gainValue  = gainValue;
        gainLevelData.at(for_level).phaseShift = phaseShift;
    }
    void SetGainValueAndPhaseShift(GainLevelT for_level, GainAndPhaseShift new_data) noexcept
    {
        gainLevelData.at(for_level).gainValue  = new_data.first;
        gainLevelData.at(for_level).phaseShift = new_data.second;
    }

    [[nodiscard]] GainLevelT GetMaxGain() const noexcept { return maxGain; }
    [[nodiscard]] GainLevelT GetMinGain() const noexcept { return minGain; }
    [[nodiscard]] GainValueT GetGainValue() const noexcept { return gainLevelData.at(gain).gainValue; }
    [[nodiscard]] PhaseShift GetPhaseShift() const noexcept { return gainLevelData.at(gain).phaseShift; }
    [[nodiscard]] GainLevelT GetGainLevel() const noexcept { return gain; }

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