#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <cstdint>
#include <memory>

#include "task.hpp"


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
    void OnInterruptCallback() noexcept;

  protected:
    void TogglePin() noexcept {

    }

    void BuzzerTask() noexcept {
        while(true) {

            TogglePin();
            Task::DelayMsUntil(2);

        }
    }

  private:
    static std::shared_ptr<Buzzer> _this;

//    Task task;

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

    auto static constexpr usedPinNumber = 64 + 12;
};

