#pragma once
#include "compiler_compatibility_workaround.hpp"

#include <memory>
#include <functional>

template<typename AmplifierT, typename ValueT>
class AutomaticGainController {
  public:
    using GainLevelT           = int;
    using GainChangeFunctor    = std::function<void()>;
    using InititalizingFunctor = std::function<void()>;

    AutomaticGainController(GainLevelT new_min_gain,
                            GainLevelT new_max_gain,
                            ValueT     lower_amplitude_agc_limit,
                            ValueT     higher_amplitude_agc_limit,
                            size_t     agc_samples_number_trigger_HIGH,
                            size_t     agc_samples_number_trigger_LOW)
      : maxGain(new_max_gain)
      , minGain(new_min_gain)
      , amplitudeTooHighTriggeringLimit(higher_amplitude_agc_limit)
      , amplitudeTooLowTriggeringLimit(lower_amplitude_agc_limit)
      , numberOfSamplesToTriggerGainIncrease{ agc_samples_number_trigger_LOW }
      , numberOfSamplesToTriggerGainDecrease{ agc_samples_number_trigger_HIGH }
      , resetExceedanceCountersAtSampleCounterValue{ (numberOfSamplesToTriggerGainDecrease >
                                                      numberOfSamplesToTriggerGainIncrease)
                                                       ? numberOfSamplesToTriggerGainDecrease
                                                       : numberOfSamplesToTriggerGainIncrease }
    { }

    void InspectSignalAmplitude(ValueT amplitude) noexcept
    {
        if (compareByAbsoluteValue and (amplitude < 0))
            amplitude = -amplitude;

        if (amplitude < amplitudeTooLowTriggeringLimit) {
            if (gain == maxGain)
                return;

            amplitudeTooLowCounter++;

            if (amplitudeTooLowCounter >= numberOfSamplesToTriggerGainIncrease) {
                IncreaseGain();
                amplitudeTooLowCounter = 0;
            }
        }
        else if (amplitude > amplitudeTooHighTriggeringLimit) {
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
    void SetUpperAmplitudeAGCLimit(ValueT new_upper_limit) noexcept { amplitudeTooHighTriggeringLimit = new_upper_limit; }
    void SetLowerAmplitudeAGCLimit(ValueT new_lower_limit) noexcept { amplitudeTooLowTriggeringLimit = new_lower_limit; }
    void AttachGainController(std::shared_ptr<GainController> new_gain_controller) noexcept
    {
        gainController = new_gain_controller;
    }

    [[nodiscard]] bool IsGainControllerAttached() const noexcept {return (not gainController.expired());}

  protected:
    void IncreaseGain() noexcept
    {
        if (std::shared_ptr<GainController> controller_instance = gainController.lock()) {
            controller_instance->IncreaseGain();
        }
    }
    void DecreaseGain() noexcept
    {
        if (std::shared_ptr<GainController> controller_instance = gainController.lock()) {
            controller_instance->DecreaseGain();
        }
    }
    void ResetCounters() noexcept
    {
        amplitudeTooLowCounter  = 0;
        amplitudeTooHighCounter = 0;
        sampleCounter           = 0;
    }

  private:
    std::weak_ptr<AmplifierT> gainController;
    bool                      compareByAbsoluteValue = true;

    GainLevelT maxGain = 0;
    GainLevelT minGain = 0;
    GainLevelT gain    = minGain;

    ValueT amplitudeTooHighTriggeringLimit{};
    ValueT amplitudeTooLowTriggeringLimit{};

    size_t amplitudeTooLowCounter{};
    size_t amplitudeTooHighCounter{};
    size_t numberOfSamplesToTriggerGainIncrease{};
    size_t numberOfSamplesToTriggerGainDecrease{};
    size_t resetExceedanceCountersAtSampleCounterValue;
    size_t sampleCounter{};
};
