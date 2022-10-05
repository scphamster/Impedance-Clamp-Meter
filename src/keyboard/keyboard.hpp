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
#include <functional>
#include <vector>
#include <utility>
#include <algorithm>

#include "FreeRTOS.h"
#include "pin.hpp"
#include "timer.hpp"

template<typename KeyboardType>
concept KeyboardDriver = requires(KeyboardType keyboard) {
                             keyboard.SetCallback(std::function<void()>{});
                             typename KeyboardType::State;
                         };

template<typename ButtonIdT, KeyboardDriver Driver, typename TimeT, typename TimerT>
class Button {
    enum class ButtonState : bool {
        Pushed   = Pin::PinState::Low,
        Released = Pin::PinState::High
    };

    Button(ButtonIdT                          id,
           std::shared_ptr<Driver>            button_driver,
           std::function<void()>            &&timeChecker,
           std::function<void(ButtonState)> &&state_change_callback)
      : driver{ button_driver }
      , myId{ id }
      , timeCheckFn{ std::move(timeChecker) }
      , stateChangeCallback{ std::move(state_change_callback) }
    {
        driver->SetCallback(std::function<void(ButtonState)>{
          std::bind(&Button<ButtonIdT, Driver, TimeT, TimerT>::StateChangeSlot, this, std::placeholders::_1) });
    }

    TimeT       GetPrevStateChangeTime() const noexcept;
    ButtonState GetCurrentState() const noexcept;

  protected:
    TimeT GetCurrentTime() { return timeCheckFn(); }
    void  DiscardPendingStateChangeNotification() { timer.DiscardPendingCalls(); }

    void StateChangeDebounced(ButtonState newstate) noexcept
    {
        debouncedState = newstate;
        stateChangeCallback(newstate);
    }

    void StateChangeSlot(ButtonState newstate) noexcept
    {
        if (newstate != state) {
            state = newstate;

            if (GetCurrentTime() - lastChangeTime < debounceTime)
                DiscardPendingStateChangeNotification();

            timer.CallThisFuncAfterTimeElapsed(
              std::bind(&Button<ButtonIdT, Driver, TimeT, TimerT>::StateChangeDebounced, this, newstate),
              debounceTime);
        }
    }

  private:
    // std::string name;
    std::shared_ptr<Driver>          driver;
    ButtonIdT                        myId{};
    TimeT                            lastChangeTime{};
    TimeT                            debounceTime{};
    TimeT                            prevStateChangeTime{};
    std::function<void()>            timeCheckFn;
    ButtonState                      state{};
    ButtonState                      debouncedState{};
    TimerT                           timer;
    std::function<void(ButtonState)> stateChangeCallback;
};

template<typename ButtonIdT, KeyboardDriver Driver, typename TimeT, typename TimerT>
TimeT
Button<ButtonIdT, Driver, TimeT, TimerT>::GetPrevStateChangeTime() const noexcept
{
    return prevStateChangeTime;
}

template<typename ButtonIdT, KeyboardDriver Driver, typename TimeT, typename TimerT>
typename Button<ButtonIdT, Driver, TimeT, TimerT>::ButtonState
Button<ButtonIdT, Driver, TimeT, TimerT>::GetCurrentState() const noexcept
{
    return state;
}

class AbstractKeyboard {
  public:
};

template<KeyboardDriver Driver, typename ButtonT, typename TimeT, typename TimerT>
class Keyboard {
  public:
    enum class KeyIdent {
        Enter = 0,
        Down,
        Up,
        Left,
        Right,
        F1,
        F2,
        F3,
        F4
    };

    enum class ButtonState : bool {
        Pushed   = ButtonT::ButtonState::Low,
        Released = ButtonT::ButtonState::High
    };

    /**
     * quick button push event consists of such ButtonEventT events sent in order : Pushed, Released, PushedAndReleased
     * long button push event consists of such ButtonEventT events: Pushed, Released, PushedAndHold, PushedAndReleased
     */
    enum class ButtonEventT : int {
        Pushed   = static_cast<int>(ButtonState::Pushed),     // just after push
        Released = static_cast<int>(ButtonState::Released),   // just after release
        PushedAndReleased,                                    // after push + delay if still pushed
        PushedAndHold                                         // after push + release
    };

    using ButtonStateChangeCallbackT = std::function<void(KeyIdent)>;
    using ButtonEventCallbackT       = std::function<void()>;
    using ButtonIdT                  = int;

    Keyboard()
      : driver{ std::make_shared<Driver>() }
    {
        buttons.push_back(ButtonT{ static_cast<ButtonIdT>(KeyIdent::Enter),
                                   driver,
                                   [this]() { xTaskGetTickCount(); },
                                   [this](ButtonState state) { ButtonStateChangeCallback(state); } });
        // todo: add all buttons
    }

    const ButtonT &GetButtonWithId(ButtonIdT) noexcept;
    void InvokeButtonEventCallback(const ButtonT &button, ButtonEventT event);
    void SetLongPressThr(int new_threshold) noexcept;
    // slots:
    void ButtonStateChangeCallback(ButtonIdT id) noexcept;

  private:
    std::shared_ptr<Driver>                                            driver;
    std::vector<std::pair<ButtonT, std::vector<ButtonEventCallbackT>>> buttons;
    int longPressThr{50}; //todo: make configurable
};

template<KeyboardDriver Driver, typename ButtonT, typename TimeT, typename TimerT>
const ButtonT &
Keyboard<Driver, ButtonT, TimeT, TimerT>::GetButtonWithId(Keyboard::ButtonIdT id) noexcept
{
    return buttons.at(id);
}

template<KeyboardDriver Driver, typename ButtonT, typename TimeT, typename TimerT>
void
Keyboard<Driver, ButtonT, TimeT, TimerT>::InvokeButtonEventCallback(const ButtonT &button, Keyboard::ButtonEventT event)
{
    button.second.at(event)();
}

template<KeyboardDriver Driver, typename ButtonT, typename TimeT, typename TimerT>
void
Keyboard<Driver, ButtonT, TimeT, TimerT>::ButtonStateChangeCallback(Keyboard::ButtonIdT id) noexcept
{
    auto button        = GetButtonWithId(id);
    auto current_state = button.GetCurrentState();

    InvokeButtonEventCallback(button, static_cast<ButtonEventT>(current_state));


    auto current_time = xTaskGetTickCount();
    if (current_state = ButtonState::Pushed) {
        if (current_time - button.GetLastStateChangeTime() > longPressThr) {
            //call longpress callback
        }
    }
    else {   // ButtonState::Released
    }

    // state change processing
    // if state = released and prev state change was X time ago -> generate pushrelease event

    // event = ...

    auto event = ButtonEventT{};   // todo: implement;
    InvokeButtonEventCallback(id, event);
}

template<KeyboardDriver Driver, typename ButtonT, typename TimeT, typename TimerT>
void
Keyboard<Driver, ButtonT, TimeT, TimerT>::SetLongPressThr(int new_threshold) noexcept
{
    longPressThr = new_threshold;
}
