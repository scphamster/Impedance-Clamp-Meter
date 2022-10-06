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
                             keyboard.SetPinStateChangeCallback(std::function<void()>{});
                         };

template<typename ButtonIdT, KeyboardDriver Driver, typename TimeT, typename TimerT>
class Button {
    enum class ButtonState : bool {
        Pushed   = Pin::PinState::Low,
        Released = Pin::PinState::High
    };

    using EventCallbackT              = std::function<void()>;
    using StateChangeCallbackT        = std::function<void(ButtonState)>;
    using EventWasAlreadyInvokedFlagT = bool;

    Button(ButtonIdT                          new_id,
           std::shared_ptr<Driver>            button_driver,
           std::function<void()>            &&timeChecker,
           std::function<void(ButtonState)> &&state_change_callback)
      : driver{ button_driver }
      , id{ new_id }
      , timeCheckFn{ std::move(timeChecker) }
      , stateChangeCallbackToKeyboard{ std::move(state_change_callback) }
    {
        driver->SetPinStateChangeCallback(
          std::function<void(ButtonState)>{ [this](ButtonState newstate) { StateChangeSlot(newstate); } });
    }

    TimeT       GetPrevStateChangeTime() const noexcept;
    ButtonState GetCurrentState() const noexcept;

    void SetNewEventCallback(EventCallbackT &&event_callback, int event_id);
    void InvokeEventCallback(int event_id);

  protected:
    TimeT GetCurrentTime() { return timeCheckFn(); }
    void  DiscardPendingStateChangeNotification() { timer.DiscardPendingCalls(); }

    void StateChangeDebounced(ButtonState newstate) noexcept
    {
        debouncedState = newstate;
        stateChangeCallbackToKeyboard(newstate);
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
    std::shared_ptr<Driver> driver;
    ButtonIdT               id{};

    ButtonState state{};
    ButtonState debouncedState{};

    TimeT lastChangeTime{};
    TimeT debounceTime{};
    TimeT prevStateChangeTime{};

    TimerT                timer;
    std::function<void()> timeCheckFn;

    StateChangeCallbackT stateChangeCallbackToKeyboard;
    std::vector<std::pair<int, std::pair<EventCallbackT, EventWasAlreadyInvokedFlagT>>>
      eventCallbacksToClient;   // todo: try to use std::map
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

template<typename ButtonIdT, KeyboardDriver Driver, typename TimeT, typename TimerT>
void
Button<ButtonIdT, Driver, TimeT, TimerT>::SetNewEventCallback(Button::EventCallbackT &&event_callback, int event_id)
{
    eventCallbacksToClient.emplace_back(event_id, std::move(event_callback));
}

template<typename ButtonIdT, KeyboardDriver Driver, typename TimeT, typename TimerT>
void
Button<ButtonIdT, Driver, TimeT, TimerT>::InvokeEventCallback(int event_id)
{
    std::find_if(eventCallbacksToClient.begin(),
                 eventCallbacksToClient.end(),
                 [id = event_id](const auto &id__callback_wasalreadyInvokedFlag) {
                     if (id__callback_wasalreadyInvokedFlag.first == id) {
                         if (id__callback_wasalreadyInvokedFlag.second.second) {
                             return;
                         }
                         else {
                             id__callback_wasalreadyInvokedFlag.second.first();
                             id__callback_wasalreadyInvokedFlag.second.second = true;
                         }
                     }
                     else {
                         id__callback_wasalreadyInvokedFlag.second.second = false;
                     }
                 }

    );
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
     * quick button push event consists of such ButtonEventT events sent in order : Pushed, Released, PushedAndReleased.
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

    explicit Keyboard(std::shared_ptr<Driver> new_driver)
      : driver{ new_driver }
    {
        buttons.push_back(ButtonT{ static_cast<ButtonIdT>(KeyIdent::Enter),
                                   driver,
                                   [this]() { xTaskGetTickCount(); },
                                   [this](ButtonState state) { ButtonStateChangeCallback(state); } });
        // todo: add all buttons
    }

    Keyboard() { Keyboard{ std::make_shared<Driver>() }; }

    const ButtonT &GetButtonWithId(ButtonIdT) noexcept;
    void           InvokeButtonEventCallback(const ButtonT &button, ButtonEventT event);
    void           SetLongPressThr(int new_threshold) noexcept;
    // slots:
    void ButtonStateChangeCallback(ButtonIdT id) noexcept;

  private:
    std::shared_ptr<Driver> driver;
    std::vector<ButtonT>    buttons;
    int                     longPressThr{ 50 };   // todo: make configurable
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
    button.InvokeEventCallback(static_cast<int>(event));
}

template<KeyboardDriver Driver, typename ButtonT, typename TimeT, typename TimerT>
void
Keyboard<Driver, ButtonT, TimeT, TimerT>::ButtonStateChangeCallback(Keyboard::ButtonIdT id) noexcept
{
    auto button        = GetButtonWithId(id);
    auto current_state = button.GetCurrentState();

    InvokeButtonEventCallback(button, static_cast<ButtonEventT>(current_state));

    auto current_time = xTaskGetTickCount();
    if (current_state == ButtonState::Pushed) {
        if (current_time - button.GetLastStateChangeTime() > longPressThr /* && callback was not invoked yet*/) {
            button.InvokeEventCallback(static_cast<int>(ButtonEventT::PushedAndHold));
        }
    }
    else {   // ButtonState::Released
        button.InvokeEventCallback(static_cast<int>(ButtonEventT::PushedAndReleased));
    }
}

template<KeyboardDriver Driver, typename ButtonT, typename TimeT, typename TimerT>
void
Keyboard<Driver, ButtonT, TimeT, TimerT>::SetLongPressThr(int new_threshold) noexcept
{
    longPressThr = new_threshold;
}
