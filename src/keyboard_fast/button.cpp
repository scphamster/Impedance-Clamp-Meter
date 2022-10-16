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

MCP23016Button::MCP23016Button(MCP23016Button::ButtonGroupIdT new_group) noexcept
  : group{ new_group }
{ }

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
MCP23016Button::SetEventCallback(MCP23016Button::EventId wanted_id, MCP23016Button::Callback &&callback) noexcept
{
    if (eventCallbacks.contains(wanted_id)) {
        eventCallbacks.at(wanted_id) = std::move(callback);
    }
    else {
        eventCallbacks.emplace(std::pair{ wanted_id, std::move(callback) });
    }
}

void
MCP23016Button::SetEventCallback(std::vector<std::pair<EventId, Callback>> &&callbacks) noexcept
{
    for (auto &&[id, callback] : callbacks) {
        eventCallbacks[id] = std::move(callback);
    }
}

void
MCP23016Button::InvokeEventCallback(MCP23016Button::EventId wanted_event_id) noexcept
{
    if (eventCallbacks.contains(wanted_event_id)) {
        eventCallbacks.at(wanted_event_id)();
    }
}
void
MCP23016Button::ClearEventCallback(const std::vector<EventId> &event_ids) noexcept
{
    for (const auto id : event_ids) {
        eventCallbacks.erase(id);
    }
}
void
MCP23016Button::ClearEventCallback() noexcept
{
    eventCallbacks.clear();
}

void
MCP23016Button::SetGroup(MCP23016Button::ButtonGroupIdT new_group) noexcept
{
    group = new_group;
}
MCP23016Button::ButtonGroupIdT
MCP23016Button::GetGroup() const noexcept
{
    return group;
}
