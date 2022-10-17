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
//#include "encoder.hpp"

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
        Back = 0,
        Left,
        Right,
        Enter,
        Up,
        Down,
        F1,
        F2,
        F4,
        F3,
        EncoderPushButton
    };
    enum class ButtonEvent {
        Push,
        Release,
        LongPush
    };
    using MasterCallback = std::function<void(ButtonEvent, ButtonName)>;
    enum class ButtonGroup : ButtonGroupIdT {
        PageIndependent = 0,
        PageDependent
    };

    explicit Keyboard(std::shared_ptr<Driver> new_driver, std::shared_ptr<TimerT> new_timer)
      : ioDriver{ new_driver }
      , timer{ new_timer }
      , buttons{ ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageIndependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent) },
                 ButtonT{ static_cast<std::underlying_type_t<ButtonGroup>>(ButtonGroup::PageDependent) } }
    { }

    Keyboard()
      : Keyboard{ std::make_shared<Driver>([this](const Pin_MCP23016 &pin) { ButtonsStateChangeCallback(pin); }),
                  std::make_shared<TimerT>(KeyboardTimerPeriod) }
    { }

    Keyboard(const Keyboard &other)          = default;
    Keyboard &operator=(const Keyboard &rhs) = default;
    Keyboard(Keyboard &&other)               = default;
    Keyboard &operator=(Keyboard &&rhs)      = default;
    ~Keyboard()                              = default;

    void SetButtonEventCallback(ButtonName button_number, ButtonEvent event, ButtonEventCallback &&callback) noexcept
    {
        buttons.at(static_cast<int>(button_number))
          .SetEventCallback(static_cast<int>(event), std::forward<decltype(callback)>(callback));
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

    void SetMasterCallback(MasterCallback &&new_masterCallback) noexcept
    {
        masterCallback = std::move(new_masterCallback);
    }
    void ClearMasterCallback()
    {
        if (masterCallback)
            masterCallback = []() {};   // todo: try other clearing techniques
    }

  protected:
    // slots:
    void ButtonsStateChangeCallback(const Pin_MCP23016 &pin) noexcept
    {
        if (static_cast<ButtonState>(pin.GetPinState()) == ButtonT::ButtonState::Released) {
            if (timer->GetCurrentTime() - lastChangeTime > debounceDelay) {
                lastChangeTime = timer->GetCurrentTime();

                if (masterCallback) {
                    masterCallback(ButtonEvent::Release, static_cast<ButtonName>(pin.GetNumber()));
                }

                buttons.at(pin.GetNumber()).InvokeEventCallback(static_cast<int>(ButtonEvent::Release));
            }
        }
        else {
            lastChangeTime = timer->GetCurrentTime();
        }
    }

  private:
    std::shared_ptr<Driver>              ioDriver;
    std::shared_ptr<TimerT>              timer;
    std::array<ButtonT, NumberOfButtons> buttons;
    MasterCallback                       masterCallback;

    TimeT lastChangeTime{ 0 };
    TimeT static constexpr debounceDelay = 5;
};
