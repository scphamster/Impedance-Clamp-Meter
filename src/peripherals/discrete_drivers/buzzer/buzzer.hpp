#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <cstdint>
#include <memory>

class Buzzer {
  public:
    using RegisterSubtype = uint32_t;

    static std::shared_ptr<Buzzer> Get() noexcept;

    Buzzer(Buzzer const &other)            = delete;
    Buzzer &operator=(Buzzer const &other) = delete;

    void Init() noexcept;
    void SetFrequency(float current) noexcept;
    void Enable() noexcept;

    void Disable() noexcept;

  private:
    static std::shared_ptr<Buzzer> _this;

    Buzzer() = default;

    bool isInitialized = false;
    bool isEnabled     = false;
    int  frequency     = 0;

    struct TimerInternals {
        RegisterSubtype rc{ 0 };

        RegisterSubtype min{};
        RegisterSubtype max{};

        int channel;
    };

    TimerInternals timerInternals;

    float static constexpr k     = 372.5246f;
    float static constexpr offst = 6.6439f;

    auto static constexpr usedPinNumber = 26;
};

