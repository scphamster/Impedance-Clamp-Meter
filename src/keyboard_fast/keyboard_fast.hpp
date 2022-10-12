#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <memory>

#include "pin.hpp"
#include "button.hpp"
#include "clamp_meter_concepts.hpp"

template<IODriver Driver, Timer TimerT, ButtonC ButtonT>
class Keyboard {
  public:
    using TimeT               = typename TimerT::TimeT;
    using ButtonState         = typename ButtonT::ButtonState;
    using ButtonEventCallback = std::function<void()>;
    using ButtonId            = int;

    enum {
        NumberOfButtons = 16
    };
    enum class ButtonEvent {
        Push,
        Release,
        LongPush
    };

    explicit Keyboard(std::shared_ptr<Driver> new_driver, std::shared_ptr<TimerT> new_timer)
      : ioDriver{ new_driver }
      , timer{ new_timer }
    { }

    Keyboard()
      : Keyboard{ std::make_shared<Driver>([this](const Pin &pin) { ButtonsStateChangeCallback(pin); }),
                  std::make_shared<TimerT>(10) }
    { }

    Keyboard(const Keyboard &other)          = default;
    Keyboard &operator=(const Keyboard &rhs) = default;
    Keyboard(Keyboard &&other)               = default;
    Keyboard &operator=(Keyboard &&rhs)      = default;
    ~Keyboard() { }
    void SetButtonEventCallback(int button_number, ButtonEvent event, ButtonEventCallback &&callback)
    {
        buttons.at(button_number).SetEventCallback(static_cast<int>(event), std::forward<decltype(callback)>(callback));
    }

    void ClearButtonsCallbacks(std::vector<ButtonId> buttons) {

    }


  protected:
    // slots:
    void ButtonsStateChangeCallback(const Pin &pin) noexcept
    {
        if (static_cast<ButtonState>(pin.GetPinState()) == ButtonT::ButtonState::Released) {
            if (timer->GetCurrentTime() - lastChangeTime > debounceDelay) {
                lastChangeTime = timer->GetCurrentTime();
                buttons.at(pin.GetNumber()).InvokeEventCallback(static_cast<int>(ButtonEvent::Release));
            }
        }
    }

  private:
    std::shared_ptr<Driver>              ioDriver;
    std::shared_ptr<TimerT>              timer;
    std::array<ButtonT, NumberOfButtons> buttons;
    TimeT                                lastChangeTime{ 0 };

    TimeT debounceDelay = 5;
};
