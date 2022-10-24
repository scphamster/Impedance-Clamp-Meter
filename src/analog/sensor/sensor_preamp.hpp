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
      , numberOfSamplesAGCTriggerHIGH{ agc_samples_number_trigger_HIGH }
      , numberOfSamplesAGCTriggerLOW{ agc_samples_number_trigger_LOW }
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
            gain++;
        }
    }
    void DecreaseGain() noexcept
    {
        if (gainChangeFunctors.contains(gain - 1) && (gain - 1) >= minGain) {
            gainChangeFunctors.at(gain - 1)();
            gain--;
        }
    }
    void CheckAmplitudeAndCorrectGainIfNeeded(ValueType amplitude) noexcept
    {
        if (amplitude < lowerAmplitudeAGCLimit) {
            if (gain == maxGain)
                return;

            amplitudeTooLowCounter++;

            if (amplitudeTooLowCounter >= numberOfSamplesAGCTriggerLOW) {
                IncreaseGain();
            }
        }
        else if (amplitude > upperAmplitudeAGCLimit) {
            if (gain == minGain)
                return;

            amplitude_TooHighCounter++;

            if (amplitude_TooHighCounter >= numberOfSamplesAGCTriggerHIGH)
                DecreaseGain();
        }

        check_counter++;

        if (check_counter > numberOfSamplesAGCTriggerHIGH && check_counter > numberOfSamplesAGCTriggerLOW) {
            amplitude_TooHighCounter = 0;
            amplitudeTooLowCounter   = 0;
            check_counter            = 0;
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

  private:
    GainLevelT maxGain = 4;
    GainLevelT minGain = 0;
    GainLevelT gain    = minGain;

    bool      AGC_enabled = false;
    ValueType upperAmplitudeAGCLimit{};
    ValueType lowerAmplitudeAGCLimit{};

    size_t amplitudeTooLowCounter{};
    size_t amplitude_TooHighCounter{};
    size_t numberOfSamplesAGCTriggerLOW{};
    size_t numberOfSamplesAGCTriggerHIGH{};
    size_t check_counter{};

    std::map<GainLevelT, GainChangeFunctor> gainChangeFunctors;
};
