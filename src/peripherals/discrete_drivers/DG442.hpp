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
#include <memory>
#include <algorithm>

#include <functional>

class DG442 {
  public:
    class Switch {
      public:
        using PinNum            = int;
        using PinControlFunctor = std::function<void()>;

        Switch(PinNum pin_num, PinControlFunctor &&enabler, PinControlFunctor &&disabler)
          : controllingPinNumber{ pin_num }
          , controlFunctors{ std::forward<PinControlFunctor>(enabler), std::forward<PinControlFunctor>(disabler) }
        { }

        using Identificator = int;
        enum class State {
            Enabled = 0,
            Disabled
        };

        void SetState(State new_state) noexcept
        {   // todo: implement
            controlFunctors.at(static_cast<int>(new_state))();
            state = new_state;
        }

        [[nodiscard]] State GetState() const noexcept { return state; }

      private:
        PinNum controllingPinNumber;
        State  state{ State::Disabled };

        std::array<PinControlFunctor, 2> controlFunctors;
    };

    enum class SwitchId {
        A = 0,
        B,
        C,
        D
    };

    using State = Switch::State;

    [[nodiscard]] bool Contains(SwitchId id) noexcept
    {
        return std::any_of(switches.begin(), switches.end(), [id](auto const &id_switch_pair) {
            if (id_switch_pair.first == id)
                return true;
            else
                return false;
        });
    }

    void InsertSwitch(SwitchId id_of_switch, Switch &&sw) noexcept
    {
        if (switches.size() > maxNumberOfSwitches)
            return;

        for (auto &id_switch_pair : switches) {
            if (id_switch_pair.first == id_of_switch) {
                id_switch_pair.second = std::forward<Switch>(sw);
                return;
            }
        }

        switches.push_back({ id_of_switch, std::move(sw) });
        std::sort(switches.begin(), switches.end(), [](auto const &lh, auto const &rh) { return lh.first < rh.first; });
    }

    void SetSwitchState(SwitchId id, State new_state) noexcept
    {
        for (auto &id_switch_pair : switches) {
            if (id_switch_pair.first == id)
                id_switch_pair.second.SetState(new_state);
        }
    }
    [[nodiscard]] State GetSwitchState(SwitchId id_of_switch) noexcept
    {
        for (auto const &id_switch_pair : switches) {
            if (id_switch_pair.first == id_of_switch)
                return id_switch_pair.second.GetState();
        }

        return State::Disabled;
    }

  protected:
  private:
    auto constexpr static maxNumberOfSwitches = 4;
    std::vector<std::pair<SwitchId, Switch>> switches;
};
