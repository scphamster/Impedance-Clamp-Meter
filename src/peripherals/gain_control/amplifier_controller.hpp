#pragma once
#include "compiler_compatibility_workaround.hpp"
#include "gain_controller.hpp"
#include "automatic_gain_controller.hpp"

class AmplifierController {
  public:
    using AGCType = AutomaticGainController<GainController, float>;
    using GainT   = typename GainController::GainLevelT;

    AmplifierController(GainT min_gain, GainT max_gain)
      : gainController{ std::make_shared<GainController>(min_gain, max_gain) }
    { }

    void EnableAGC(bool if_enable) noexcept { agcIsEnabled = if_enable; }
    void SetAGC(std::unique_ptr<AGCType> &&new_agc) noexcept
    {
        agc = std::forward<decltype(new_agc)>(new_agc);
        agc->AttachGainController(gainController);
    }
    void ForceForwardAmplitudeValueToAGC(auto new_value) noexcept { agc->InspectSignalAmplitude(new_value); }
    void ForwardAmplitudeValueToAGCIfEnabled(auto new_value)
    {
        if (agcIsEnabled)
            agc->InspectSignalAmplitude(new_value);
    }
    void SetGain(GainT new_gain) noexcept
    {
        if (not agcIsEnabled)
            gainController->SetGain(new_gain);
    };
    void ForceSetGain(GainT new_gain) noexcept { gainController->SetGain(new_gain); }

    bool                       AgcIsEnabled() const noexcept { return agcIsEnabled; }
    std::unique_ptr<AGCType> &&TakeAGC() noexcept
    {
        agcIsEnabled = false;
        return std::move(agc);
    };

  protected:
    std::shared_ptr<GainController> gainController;
    std::unique_ptr<AGCType>        agc;
    bool                            agcIsEnabled = false;
};
