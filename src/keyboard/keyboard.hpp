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

template<typename KeyboardType>
concept KeyboardDriver = requires { 1 + 1; };

template<typename ButtonIdT, KeyboardDriver Driver, typename TimeT, typename TimerT>
class Button {
    using ButtonState = typename Driver::State;

    Button(ButtonIdT                        id,
           std::shared_ptr<Driver>          button_driver,
           std::function<void()>          &&timeChecker,
           std::function<void(ButtonState)> state_change_callback)
      : driver{ button_driver }
      , myId{ id }
      , timeCheckFn{ std::move(timeChecker) }
    {
        driver->SetCallback(std::function<void(ButtonState)>{
          std::bind(&Button<ButtonIdT, Driver, TimeT, TimerT>::StateChangeSlot, this, std::placeholders::_1) });
    }

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
              std::bind(&Button<ButtonIdT, Driver, TimeT, TimerT>::StateChangeDebounced, this, newstate), debounceTime);
        }
    }

  private:
    // std::string name;
    std::shared_ptr<Driver>          driver;
    ButtonIdT                        myId{};
    TimeT                            lastChangeTime{};
    TimeT                            debounceTime{};
    std::function<void()>            timeCheckFn;
    ButtonState                      state{};
    ButtonState                      debouncedState{};
    TimerT                           timer;
    std::function<void(ButtonState)> stateChangeCallback;
};

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

    using Callback    = std::function<void(KeyIdent)>;
    using ButtonState = Pin::PinState;
    using ButtonIdT   = int;
//
//    Keyboard()
//      : driver{ std::make_unique<Driver>() }
//    {
//        driver->SetPinChangeCallback(std::move(std::function<void(const Pin &)>{
//          std::bind(&Keyboard<Driver>::ButtonChangedStateCallback, this, std::placeholders::_1) }));
//    }
//
//    void AddButton(Button<ButtonIdT> &&new_button) { }
//
//    void ButtonChangedStateCallback(const Pin &);
//    void SetCallbackForKeys(std::vector<KeyIdent> keys, Callback callback);
//
//  protected:
//    void NotifyOwnerButtonPressed(const Button &pressed_button) { ownerButtonPressNotificationCallback(pressed_button); }
//
//  private:
//    std::vector<std::pair<KeyIdent, Callback>> callbacks;
//    std::unique_ptr<Driver>                    driver;
//
//    TickType_t lastPinStateChangeTime = 0;
//
//    // prototype
//    std::function<void(const Button<ButtonIdT> &)> ownerButtonPressNotificationCallback;
//    std::vector<Button<ButtonIdT>>                 buttons;
};
//
//template<KeyboardDriver Driver>
//void
//Keyboard<Driver>::ButtonChangedStateCallback(const Pin &)
//{ }
//
//template<KeyboardDriver Driver>
//void
//Keyboard<Driver>::SetCallbackForKeys(std::vector<KeyIdent> keys, Keyboard::Callback callback)
//{
//    for (const auto &key : keys) {
//        // search in callbacks if key is present, if not -> add ... else -> exchange --->>
//        auto key_function_It = std::find_if(callbacks.begin(), callbacks.end(), [wanted_key = key](const auto &key_func) {
//            if (key_func.first == wanted_key)
//                return true;
//            else
//                return false;
//        });
//
//        if (key_function_It == callbacks.end()) {
//            callbacks.push_back({ key, callback });
//        }
//        else {
//            key_function_It->second = callback;
//        }
//    }
//
//    std::sort(callbacks.begin(), callbacks.end(), [](const auto &key_func_left, const auto &key_func_right) {
//        return key_func_left.first < key_func_right.first;
//    });
//}
