#pragma once
#include "compiler_compatibility_workaround.hpp"
#include "gain_controller.hpp"
#include "automatic_gain_controller.hpp"
#include "menu_model_item.hpp"
#include "task.hpp"

class AmplifierController {
  public:
    using AGCType = AutomaticGainController<GainController, float>;
    using GainT   = typename GainController::GainLevelT;

    explicit AmplifierController(std::shared_ptr<GainController> new_gain_controller,
                                 std::unique_ptr<AGCType>      &&new_agc,
                                 bool                            enable_agc = true)
      : gainController{ std::move(new_gain_controller) }
      , agc{ std::move(new_agc) }
      , agcIsEnabled(enable_agc)
    {
        agc->AttachGainController(gainController);
    }

    explicit AmplifierController(std::shared_ptr<GainController> new_gain_controller)
      : gainController{ std::move(new_gain_controller) }
    {


    }

//    explicit AmplifierController(GainT min_gain, GainT max_gain)
//      : AmplifierController{ std::make_shared<GainController>(min_gain, max_gain) }
//    { }

    void EnableAGC(bool if_enable) noexcept { agcIsEnabled = if_enable; }
    void SetAGC(std::unique_ptr<AGCType> &&new_agc, bool enable_agc = true) noexcept
    {
        agc = std::forward<decltype(new_agc)>(new_agc);
        agc->AttachGainController(gainController);
        agcIsEnabled = enable_agc;
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
    void                       ForceSetGain(GainT new_gain) noexcept { gainController->SetGain(new_gain); }
    void                       SetMinGain() noexcept { gainController->SetGain(gainController->GetMinGain()); }
    void                       SetMaxGain() noexcept { gainController->SetGain(gainController->GetMaxGain()); }
    bool                       AgcIsEnabled() const noexcept { return agcIsEnabled; }
    std::unique_ptr<AGCType> &&TakeAGC() noexcept
    {
        agcIsEnabled = false;
        return std::move(agc);
    };

  protected:


    std::shared_ptr<GainController> gainController;
    std::shared_ptr<UniversalSafeType> testVal;
    std::unique_ptr<AGCType>        agc;
    bool                            agcIsEnabled = false;
};
