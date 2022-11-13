#pragma once
#include "compiler_compatibility_workaround.hpp"
#include "gain_controller.hpp"
#include "automatic_gain_controller.hpp"
#include "menu_model_item.hpp"
#include "task.hpp"

class AmplifierController {
  public:
    using AGCType    = AutomaticGainController<GainController, float>;
    using GainLevelT = typename GainController::GainLevelT;
    using GainValueT = GainController::GainValueT;
    using PhaseShift = GainController::PhaseShift;

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
    { }

    void EnableAGC() noexcept
    {
        if (agc)
            agcIsEnabled = true;
    }
    void                  DisableAGC() noexcept { agcIsEnabled = false; }
    [[maybe_unused]] void SetAGC(std::unique_ptr<AGCType> &&new_agc, bool enable_agc = true) noexcept
    {
        agc = std::forward<decltype(new_agc)>(new_agc);
        agc->AttachGainController(gainController);
        agcIsEnabled = enable_agc;
    }
    [[maybe_unused]] void ForceForwardAmplitudeValueToAGC(auto new_value) noexcept { agc->InspectSignalAmplitude(new_value); }
    void                  ForwardAmplitudeValueToAGCIfEnabled(auto new_value)
    {
        if (agcIsEnabled) {

            agc->InspectSignalAmplitude(new_value);

        }
    }
    void SetGain(GainLevelT new_gain) noexcept
    {
        if (not agcIsEnabled)
            gainController->SetGain(new_gain);
    };
    [[maybe_unused]] void                       ForceSetGain(GainLevelT new_gain) noexcept { gainController->SetGain(new_gain); }
    void                                        SetMinGain() noexcept { gainController->SetGain(gainController->GetMinGain()); }
    void                                        SetMaxGain() noexcept { gainController->SetGain(gainController->GetMaxGain()); }
    [[nodiscard]] bool         AgcIsEnabled() const noexcept { return agcIsEnabled; }
    [[maybe_unused]] std::unique_ptr<AGCType> &&TakeAGC() noexcept
    {
        agcIsEnabled = false;
        return std::move(agc);
    };
    [[nodiscard]] GainValueT                      GetGainValue() const noexcept { return gainController->GetGainValue(); }
    [[nodiscard]] GainLevelT                      GetGainLevel() const noexcept { return gainController->GetGainLevel(); }
    [[nodiscard]] PhaseShift                      GetPhaseShift() const noexcept { return gainController->GetPhaseShift(); }
    [[nodiscard]] std::shared_ptr<GainController> GetGainController() const noexcept { return gainController; }
    [[nodiscard]] GainLevelT                      GetMaxGain() const noexcept { return gainController->GetMaxGain(); }
    [[nodiscard]] GainLevelT                      GetMinGain() const noexcept { return gainController->GetMinGain(); }

  protected:
  private:
    std::shared_ptr<GainController> gainController;
    std::unique_ptr<AGCType>        agc;
    bool                            agcIsEnabled = false;
};
