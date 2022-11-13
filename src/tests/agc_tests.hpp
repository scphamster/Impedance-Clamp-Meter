#pragma once
#include <memory>

#include "automatic_gain_controller.hpp"
#include "amplifier_controller.hpp"
#include "task.hpp"
#include "menu_model_dialog.hpp"
class AGCTests {
  public:
    using ValueT = float;
    using AGC    = AutomaticGainController<GainController, ValueT>;

    AGCTests(std::shared_ptr<MenuModelDialog> dialog)
      : agc{ 1, 10, amplitudeLow, amplitudeHigh, 50, 500 }
      , msgBox{ std::move(dialog) }
      , testTask([this]() { TestTask(); }, stackSize, taskPriority, "agc test")
    { }

  protected:
    void TestTask() noexcept
    {
        auto   gain_lvl     = agc.GetGainLevel();
        size_t tick_counter = 0;

        while (true) {
            msgBox->ShowMsg("agc test: amplitude to low, gain should go high");
            Task::DelayMs(3000);

            while (true) {
                agc.InspectSignalAmplitude(amplitudeLow - 1);

                if (agc.GetGainLevel() != gain_lvl) {
                    gain_lvl = agc.GetGainLevel();
                    msgBox->ShowMsg("agc gain change triggered at tick=" + std::to_string(tick_counter) +
                                    " gain_lvl =" + std::to_string(gain_lvl));

                    Task::DelayMs(2000);

                    tick_counter = 0;
                    if (gain_lvl == agc.GetMaxGainLevel()) {
                        msgBox->ShowMsg("amplitude too low test passed");
                        Task::DelayMs(3000);
                        break;
                    }
                }
                else if (tick_counter > counterOverflowValue) {
                    CounterOverflowCallback();
                }
                else
                    tick_counter++;
            }

            msgBox->ShowMsg("agc test: amplitude too high, gain should go low");
            Task::DelayMs(3000);

            while (true) {
                agc.InspectSignalAmplitude(amplitudeHigh + 1);

                if (agc.GetGainLevel() != gain_lvl) {
                    gain_lvl = agc.GetGainLevel();
                    msgBox->ShowMsg("agc gain change triggered at tick=" + std::to_string(tick_counter) +
                                    " gain_lvl =" + std::to_string(gain_lvl));

                    Task::DelayMs(2000);

                    tick_counter = 0;
                    if (gain_lvl == agc.GetMinGainLevel()) {
                        msgBox->ShowMsg("amplitude too high test passed");
                        Task::DelayMs(3000);
                        break;
                    }
                }
                else if (tick_counter > counterOverflowValue) {
                    CounterOverflowCallback();
                }
                else
                    tick_counter++;
            }

            msgBox->ShowMsg("all test passed!");
            Task::DelayMs(5000);
        }
    }

    void CounterOverflowCallback() noexcept { msgBox->ShowMsg("counter overflow! test failed"); Task::DelayMs(6000); }

  private:
    AGC                              agc;
    std::shared_ptr<MenuModelDialog> msgBox;
    Task                             testTask;

    auto static constexpr stackSize              = 500;
    auto static constexpr taskPriority           = 3;
    ValueT static constexpr amplitudeLow         = 1e4f;
    ValueT static constexpr amplitudeHigh        = 3e6f;
    size_t static constexpr counterOverflowValue = 1000;
};