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

#include <vector>
#include <functional>
#include <map>
#include "pin.hpp"

class MCP23016Button {
  public:
    using EventId  = int;
    using Callback = std::function<void()>;
    using IsInitialized = bool;

    enum class ButtonState : bool{
        Pushed   = static_cast<bool>(Pin::PinState::Low),
        Released = static_cast<bool>(Pin::PinState::High)
    };

    [[nodiscard]] ButtonState GetCurrentState() const noexcept;

    void SetState(ButtonState state) noexcept;
    void SetEventCallback(EventId event_id, Callback &&callback) noexcept;
    void SetEventCallback(std::vector<std::pair<EventId, Callback>> &&callbacks) noexcept;
    void ClearEventCallback(const std::vector<EventId> &event_ids) noexcept;
    void ClearEventCallback() noexcept;
    void InvokeEventCallback(EventId wanted_event_id) noexcept;

  private:
    ButtonState currentState{ ButtonState::Released };
    std::map<EventId, Callback> eventCallbacks;
};
