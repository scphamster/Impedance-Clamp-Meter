#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <cstdint>
#include <memory>

#include "task.hpp"

class Buzzer {
  public:
    using RegisterSubtype = uint32_t;
    using FreqT           = float;

    static std::shared_ptr<Buzzer> Get() noexcept;

    Buzzer(Buzzer const &other)            = delete;
    Buzzer &operator=(Buzzer const &other) = delete;

    void Init() noexcept;
    bool SetFrequency(float tone_frequency, float modulation_frequency) noexcept;
    void Enable() const noexcept;
    void Disable() const noexcept;

    [[nodiscard]] std::pair<FreqT, FreqT> GetFrequencies() const noexcept;

  protected:
    RegisterSubtype FindRCFromFrequency(float frequency) noexcept;
    void            SetChanelFrequency(int channel, FreqT frequency) noexcept;

  private:
    static std::shared_ptr<Buzzer> _this;

    //    Task task;

    Buzzer() = default;

    bool isInitialized = false;
    bool isEnabled     = false;

    FreqT toneFrequency       = 0;
    FreqT modulationFrequency = 0;

    struct TimerInternals {
        RegisterSubtype mainToneRC{ 0 };
        RegisterSubtype modulatorRC{ 0 };

        RegisterSubtype min{};
        RegisterSubtype max{};

        RegisterSubtype modulatorMax;
        RegisterSubtype modulatorMin;

        int mainToneChannel;
        int modulatorChannel;
    };

    TimerInternals timerInternals;

    float static constexpr k     = 372.5246f;
    float static constexpr offst = 6.6439f;

    FreqT maxToneFrequency;
    FreqT minToneFrequency;
    FreqT minModulationFrequency;
    FreqT maxModulationFrequency;

    FreqT      minimalPeriod;
    TickType_t lastEntry{};
    TickType_t delayBetweenFrequencyChangesMs = 100;
    auto static constexpr usedPinNumber       = 26;
    auto static constexpr timerPeriodChangeMargin = 15;
};
