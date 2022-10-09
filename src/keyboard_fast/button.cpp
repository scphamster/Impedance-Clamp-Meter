#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <algorithm>
#include "button.hpp"

MCP23016Button::ButtonState
MCP23016Button::GetCurrentState() const noexcept
{
    return currentState;
}

void
MCP23016Button::SetState(MCP23016Button::ButtonState state) noexcept
{
    currentState = state;
}

void
MCP23016Button::SetEventCallback(MCP23016Button::EventId wanted_id, MCP23016Button::Callback &&callback)
{
    if (eventCallbacks.contains(wanted_id)) {
        eventCallbacks.at(wanted_id) = std::move(callback);
    }
    else {
        eventCallbacks.emplace(std::pair{ wanted_id, std::move(callback) });
    }
}

void
MCP23016Button::InvokeEventCallback(MCP23016Button::EventId wanted_event_id)
{
    if (eventCallbacks.contains(wanted_event_id)) {
        eventCallbacks.at(wanted_event_id)();
    }
}
