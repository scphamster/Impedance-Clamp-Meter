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

#include <memory>
#include <functional>
#include <map>

template<typename ValueType>
class SensorPreamp {
  public:
    using GainLevelT        = int;
    using GainChangeFunctor = std::function<void()>;

    SensorPreamp(GainLevelT new_min_gain,
                 GainLevelT new_max_gain,
                 ValueType  lower_amplitude_agc_limit,
                 ValueType  higher_amplitude_agc_limit,
                 size_t     agc_samples_number_trigger_HIGH,
                 size_t     agc_samples_number_trigger_LOW)
      : maxGain(new_max_gain)
      , minGain(new_min_gain)
      , upperAmplitudeAGCLimit(higher_amplitude_agc_limit)
      , lowerAmplitudeAGCLimit(lower_amplitude_agc_limit)
      , numberOfSamplesToTriggerGainDecrease{ agc_samples_number_trigger_HIGH }
      , numberOfSamplesToTriggerGainIncrease{ agc_samples_number_trigger_LOW }
    {
        if (upperAmplitudeAGCLimit != lowerAmplitudeAGCLimit)
            AGC_enabled = true;
        else
            AGC_enabled = false;
    }

    void SetGain(SensorPreamp::GainLevelT new_gain) noexcept
    {
        if (new_gain > maxGain)
            new_gain = maxGain;

        if (gainChangeFunctors.contains(new_gain)) {
            gainChangeFunctors.at(new_gain)();
            gain = new_gain;
        }
    }
    void IncreaseGain() noexcept
    {
        if (gainChangeFunctors.contains(gain + 1) && (gain + 1) <= maxGain) {
            gainChangeFunctors.at(gain + 1)();
            ResetCounters();
            gain++;
        }
    }
    void DecreaseGain() noexcept
    {
        if (gainChangeFunctors.contains(gain - 1) && (gain - 1) >= minGain) {
            gainChangeFunctors.at(gain - 1)();
            gain--;
            ResetCounters();
        }
    }
    void CheckAmplitudeAndCorrectGainIfNeeded(ValueType amplitude) noexcept
    {
        if (not AGC_enabled)
            return;

        if (compareByAbsoluteValue and (amplitude < 0))
            amplitude = -amplitude;

        if (amplitude < lowerAmplitudeAGCLimit) {
            if (gain == maxGain)
                return;

            amplitudeTooLowCounter++;

            if (amplitudeTooLowCounter >= numberOfSamplesToTriggerGainIncrease) {
                IncreaseGain();
                amplitudeTooLowCounter = 0;
            }
        }
        else if (amplitude > upperAmplitudeAGCLimit) {
            if (gain == minGain)
                return;

            amplitudeTooHighCounter++;

            if (amplitudeTooHighCounter >= numberOfSamplesToTriggerGainDecrease)
                DecreaseGain();
        }

        sampleCounter++;

        if (sampleCounter > numberOfSamplesToTriggerGainDecrease &&
            sampleCounter > numberOfSamplesToTriggerGainIncrease) {
            ResetCounters();
        }
    }
    void SetUpperAmplitudeAGCLimit(ValueType new_upper_limit) noexcept { upperAmplitudeAGCLimit = new_upper_limit; }
    void SetLowerAmplitudeAGCLimit(ValueType new_lower_limit) noexcept { lowerAmplitudeAGCLimit = new_lower_limit; }
    void EnableAGC(bool if_enable) noexcept { AGC_enabled = if_enable; }
    void SetGainChangeFunctor(GainLevelT for_gain_level, GainChangeFunctor &&new_functor)
    {
        if (for_gain_level > maxGain)
            return;

        gainChangeFunctors[for_gain_level] = std::move(new_functor);
    }

  protected:
    void ResetCounters() noexcept
    {
        amplitudeTooLowCounter  = 0;
        amplitudeTooHighCounter = 0;
        sampleCounter           = 0;
    }

  private:
    bool AGC_enabled            = false;
    bool compareByAbsoluteValue = true;

    GainLevelT maxGain = 0;
    GainLevelT minGain = 0;
    GainLevelT gain    = minGain;

    ValueType upperAmplitudeAGCLimit{};
    ValueType lowerAmplitudeAGCLimit{};

    size_t amplitudeTooLowCounter{};
    size_t amplitudeTooHighCounter{};
    size_t numberOfSamplesToTriggerGainIncrease{};
    size_t numberOfSamplesToTriggerGainDecrease{};
    size_t sampleCounter{};

    std::map<GainLevelT, GainChangeFunctor> gainChangeFunctors;
};
