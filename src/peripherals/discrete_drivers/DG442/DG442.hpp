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
#include <map>
#include <algorithm>

#include <functional>

class DG442 {
  public:
    enum class SwitchState {
        Enabled = 0,
        Disabled
    };
    using SwitchIdT              = int;
    using SwitchDisablingFunctor = std::function<void()>;
    using SwitchEnablingFunctor  = std::function<void()>;
    using SwitchFunctors         = std::vector<std::tuple<SwitchIdT, SwitchEnablingFunctor, SwitchDisablingFunctor>>;

    DG442(SwitchFunctors &&functors)
      : switchFunctors{ std::forward<SwitchFunctors>(functors) }
    { }

    void AttachSwitch(SwitchIdT id_of_switch) noexcept
    {
        if (not Contains(id_of_switch)) {
            attachedSwitches.push_back(id_of_switch);
            std::sort(attachedSwitches.begin(), attachedSwitches.end());
        }
    }
    void DetachSwitch(SwitchIdT id_of_switch) noexcept
    {
        if (Contains(id_of_switch)) {
            std::erase_if(attachedSwitches, [wanted_id = id_of_switch](auto const &current_switch_id) {
                return current_switch_id == wanted_id;
            });

            attachedSwitches.shrink_to_fit();
            std::sort(attachedSwitches.begin(), attachedSwitches.end());
        }
    }
    void SetSwitchState(SwitchIdT id_of_switch, SwitchState new_state) noexcept
    {
        if (not Contains(id_of_switch))
            std::terminate();

        for (auto const &[id, enabler, disabler] : switchFunctors) {
            if (id == id_of_switch) {
                if (new_state == SwitchState::Enabled)
                    enabler();
                else if (new_state == SwitchState::Disabled)
                    disabler();
            }
        }
    }
    [[nodiscard]] bool Contains(SwitchIdT wanted_id) noexcept
    {
        for (auto const &id : attachedSwitches) {
            if (id == wanted_id)
                return true;
        }

        return false;
    }
    [[nodiscard]] SwitchIdT GetMaxNumberOfSwitches() const noexcept { return maxNumberOfSwitches; }

  protected:
  private:
    auto constexpr static maxNumberOfSwitches = 4;
    std::vector<SwitchIdT> attachedSwitches;
    SwitchFunctors         switchFunctors;
};
