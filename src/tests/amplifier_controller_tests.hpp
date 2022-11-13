#pragma once

#include "compiler_compatibility_workaround.hpp"

#include <memory>

#include "gain_controller.hpp"
#include "automatic_gain_controller.hpp"
#include "amplifier_controller.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "menu_model_item.hpp"
#include "menu_model_dialog.hpp"
#include "task.hpp"

class AmplifierControllerTest {
  public:
    using ValueT       = float;
    using Calibrations = std::pair<ValueT, ValueT>;
    using AGC          = AutomaticGainController<GainController, ValueT>;

    AmplifierControllerTest(std::shared_ptr<MenuModelDialog> msg_box)
      : msgBox{ std::move(msg_box) }
      , testTask{ [this]() { TestTask(); }, stackSize, taskPriority, "amplifierController" }
    {
        auto gain_controller = std::make_shared<GainController>(1, 10);
        // clang-format off
        gain_controller->SetGainChangeFunctor(1, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(2, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(3, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(4, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(5, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(6, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(7, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(8, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(9, [this]() {StandardGainChangeCallback();}, Calibrations{});
        gain_controller->SetGainChangeFunctor(10, [this]() {StandardGainChangeCallback();},Calibrations{} );
        // clang-format on
        auto agc                 = std::make_unique<AGC>(1, 10, amplitudeLow, amplitudeHigh, 50, 500);
        amplifierController = std::make_unique<AmplifierController>(gain_controller, std::move(agc), true);
    }

  protected:
    void TestTask() noexcept
    {
        auto   gain_lvl     = amplifierController->GetGainLevel();
        size_t tick_counter = 0;

        while (true) {
            msgBox->ShowMsg("amplitude too low test, gain should go high");
            Task::DelayMs(3000);
            //upward test
            while (true) {
                amplifierController->ForwardAmplitudeValueToAGCIfEnabled(amplitudeLow - 1);

                if (gain_lvl != amplifierController->GetGainLevel()) {
                    gain_lvl = amplifierController->GetGainLevel();

                    msgBox->ShowMsg("gain increase triggered at tickNo:" + std::to_string(tick_counter) +
                                    "gain lvl=" + std::to_string(amplifierController->GetGainLevel()));

                    Task::DelayMs(2000);
                    tick_counter = 0;

                    if (gain_lvl >= amplifierController->GetMaxGain()) {
                        msgBox->ShowMsg("max gain reached, amplitude low test passed!");
                        Task::DelayMs(4000);
                        break;
                    }
                }
                else if (tick_counter >= tickOverflowLimit) {
                    TickOverflowCallback();
                    tick_counter = 0;
                    break;
                }
                else
                    tick_counter++;
            }

            msgBox->ShowMsg("amplitude too low test, gain should go low");
            Task::DelayMs(3000);

            while (true) {
                amplifierController->ForwardAmplitudeValueToAGCIfEnabled(amplitudeHigh + 1);

                if (gain_lvl != amplifierController->GetGainLevel()) {
                    gain_lvl = amplifierController->GetGainLevel();

                    msgBox->ShowMsg("gain decrease triggered at tickNo:" + std::to_string(tick_counter) +
                                    "gain lvl=" + std::to_string(amplifierController->GetGainLevel()));

                    Task::DelayMs(2000);
                    tick_counter = 0;

                    if (gain_lvl <= amplifierController->GetMinGain()) {
                        msgBox->ShowMsg("min gain reached, amplitude high test passed!");
                        Task::DelayMs(4000);
                        break;
                    }
                }
                else if (tick_counter >= tickOverflowLimit) {
                    TickOverflowCallback();
                    tick_counter = 0;
                    break;
                }
                else
                    tick_counter++;
            }

            msgBox->ShowMsg("all test passed!");
            Task::DelayMs(5000);

        }
    }
    void StandardGainChangeCallback() noexcept
    {
        msgBox->ShowMsg("Gain change requested, gLvl = " + std::to_string(amplifierController->GetGainLevel()));
        Task::DelayMs(3000);
    }

    void TickOverflowCallback() noexcept
    {
        msgBox->ShowMsg("tick overflow! Test failed");

        Task::DelayMs(5000);
    }

  private:
    std::unique_ptr<AmplifierController> amplifierController;
    std::shared_ptr<MenuModelDialog>     msgBox;
    Task                                 testTask;

    ValueT static constexpr amplitudeLow      = 1e4f;
    ValueT static constexpr amplitudeHigh     = 3e6f;
    auto static constexpr stackSize           = 500;
    auto static constexpr taskPriority        = 3;
    size_t static constexpr tickOverflowLimit = 501;
};