#pragma once

#include "compiler_compatibility_workaround.hpp"
#include "project_configs.hpp"

#include "FreeRTOS.h"

#include "task.hpp"
#include "pio.h"
#include "pmc.h"
#include "tc.h"

class BuzzerTask {
  public:
    using RegisterSubtype = uint32_t;
    using FreqT           = float;

    BuzzerTask()
      : task{ [this]() { BuzzTask(); },
              ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::Buzzer),
              ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::Buzzer),
              "buzzer" }
    {
        Init();
        Stop();
    }

    void Start() noexcept { task.Resume(); }
    void Stop() noexcept
    {
        task.Suspend();
        SuspendMainTone();
    }

    void Init() noexcept
    {
        // todo: make configurable
        timerInternals.timerInstance   = TC0;
        timerInternals.min             = 50;
        timerInternals.max             = 5000;
        timerInternals.mainToneChannel = 2;

        minimalPeriod =
          2 * 128 / 120e6f;   // todo: cleanup magic numbers || 128-  prescaller, 120e6 - core clock, 2 - updown toggle
        maxToneFrequency  = 1 / minimalPeriod;
        minToneFrequency  = 50;
        maxModulationFreq = 10;
        minModulaitonFreq = 0.5;

        uint32_t clock_source = (3 << 0);
        uint32_t wavsel       = (2 << 13);
        uint32_t wavemode     = (1 << 15);
        uint32_t ACPC_val     = (3 << 18);

        pmc_enable_periph_clk(ID_TC2);
        tc_init(timerInternals.timerInstance,
                timerInternals.mainToneChannel,
                clock_source /*| burst*/ | wavsel | wavemode | ACPC_val);
        tc_write_rc(timerInternals.timerInstance, timerInternals.mainToneChannel, 5000);
    }
    void SetFrequency(FreqT tone, FreqT modulation) noexcept
    {
        if (tone > maxToneFrequency)
            newToneFreq = maxToneFrequency;
        else if (tone < minToneFrequency)
            newToneFreq = minToneFrequency;
        else
            newToneFreq = tone;

        if (modulation > maxModulationFreq)
            modulation = maxModulationFreq;
        else if (modulation < minModulaitonFreq)
            modulation = minModulaitonFreq;

        newModulationPeriodMs = 1000 / modulation;
    }

  protected:
    void SetMainToneFrequency(FreqT frequency)
    {
        auto new_rc = FindRCFromFrequency(frequency);

        tc_write_rc(timerInternals.timerInstance, timerInternals.mainToneChannel, new_rc);
        tc_start(timerInternals.timerInstance, timerInternals.mainToneChannel);
    }

    [[nodiscard]] RegisterSubtype FindRCFromFrequency(FreqT frequency) const noexcept
    {
        configASSERT(frequency < maxToneFrequency);
        return static_cast<RegisterSubtype>(1 / (minimalPeriod * frequency));
    }
    void ResumeMainTone() noexcept
    {
        pio_set_peripheral(PIOA, PIO_PERIPH_B, (1 << usedPinNumber));
        tc_start(timerInternals.timerInstance, timerInternals.mainToneChannel);
        isEnabled = true;
    }
    void SuspendMainTone() noexcept
    {
        pio_set_input(PIOA, (1 << usedPinNumber), 0);
        tc_stop(timerInternals.timerInstance, timerInternals.mainToneChannel);
        isEnabled = false;
    }

    [[noreturn]] void BuzzTask() noexcept
    {
        while (true) {
            if (newModulationPeriodMs != modulationPeriodMs)
                modulationPeriodMs = newModulationPeriodMs;
            if (newToneFreq != toneFrequency) {
                toneFrequency = newToneFreq;
                SetMainToneFrequency(toneFrequency);
            }

            if (isEnabled) {
                SuspendMainTone();
            }
            else {
                ResumeMainTone();
            }

            Task::DelayMsUntil(static_cast<RegisterSubtype>(modulationPeriodMs));
        }
    }

  private:
    struct TimerInternals {
        Tc *timerInstance   = nullptr;
        int mainToneChannel = 0;

        RegisterSubtype mainToneRC{ 0 };
        RegisterSubtype min{};
        RegisterSubtype max{};
    };

    TimerInternals timerInternals;
    Task           task;

    FreqT maxModulationFreq;
    FreqT minModulaitonFreq;
    FreqT minimalPeriod;
    FreqT maxToneFrequency;
    FreqT minToneFrequency;

    FreqT modulationPeriodMs{ 100 };
    FreqT toneFrequency{1000};
    FreqT newToneFreq{1000};
    FreqT newModulationPeriodMs{100};
    bool  isEnabled{ false };

    auto static constexpr usedPinNumber = 26;
};