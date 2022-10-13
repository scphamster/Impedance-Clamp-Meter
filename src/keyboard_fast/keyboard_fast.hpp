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
#include <type_traits>

#include "pin.hpp"
#include "button.hpp"
#include "clamp_meter_concepts.hpp"

auto constexpr KeyboardTimerPeriod = 10;

template<IODriver Driver, Timer TimerT, ButtonC ButtonT>
class Keyboard {
  public:
    using TimeT               = typename TimerT::TimeT;
    using ButtonState         = typename ButtonT::ButtonState;
    using ButtonEventCallback = std::function<void()>;
    using ButtonId            = int;
    using ButtonGroupIdT      = typename ButtonT::ButtonGroupIdT;

    enum {
        NumberOfButtons = 16
    };
    enum class ButtonName {
        Up = 0,
        Down,
        Left,
        Right,
        Enter,
        Back,
        Menu,
        F1,
        F2,
        F3,
        F4,
        EncoderPushButton
    };

    enum class ButtonEvent {
        Push,
        Release,
        LongPush
    };


    enum class ButtonGroup : ButtonGroupIdT {
        PageIndependent = 0,
        PageDependent
    };

    explicit Keyboard(std::shared_ptr<Driver> new_driver, std::shared_ptr<TimerT> new_timer)
      : ioDriver{ new_driver }
      , timer{ new_timer }
      , buttons{
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent)},
          ButtonT{static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent)}
      }
    { }
    Keyboard()
      : Keyboard{ std::make_shared<Driver>([this](const Pin &pin) { ButtonsStateChangeCallback(pin); }),
                  std::make_shared<TimerT>(KeyboardTimerPeriod) }
    { }

    Keyboard(const Keyboard &other)          = default;
    Keyboard &operator=(const Keyboard &rhs) = default;
    Keyboard(Keyboard &&other)               = default;
    Keyboard &operator=(Keyboard &&rhs)      = default;
    ~Keyboard()                              = default;

    void SetButtonEventCallback(ButtonName button_number, ButtonEvent event, ButtonEventCallback &&callback) noexcept
    {
        buttons.at(static_cast<int>(button_number)).SetEventCallback(static_cast<int>(event), std::forward<decltype(callback)>(callback));
    }
    void SetButtonEventCallback(std::vector<std::tuple<ButtonId, ButtonEvent, ButtonEventCallback>> &&callbacks) noexcept
    {
        for (auto &&[button_id, event, callback] : callbacks) {
            buttons.at(button_id).SetEventCallback(std::move(event), std::move(callback));
        }
    }
    void ClearButtonGroupCallbacks(ButtonGroup group) noexcept
    {
        for (const auto &button : buttons) {
            if (button.GetGroup() == static_cast<std::underlying_type_t<ButtonGroup>>(group))
                button.ClearEventCallback();
        }
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

    TimeT lastChangeTime{ 0 };

    TimeT debounceDelay = 5;
};
