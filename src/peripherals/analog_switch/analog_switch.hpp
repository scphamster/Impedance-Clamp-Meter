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

template<typename SwitchDriver>
class AnalogSwitch {
  public:
    using Id    = typename SwitchDriver::SwitchID;
    using State = typename SwitchDriver::SwitchState;

    AnalogSwitch(Id id, State default_state, std::shared_ptr<SwitchDriver> new_driver)
      : driver{ std::move(new_driver) }
    {
        driver->AttachSwitchById(id);
        SetId(id);

        driver->SetSwitchState(id, default_state);
    }

    void SetId(Id new_id)
    {
        if (driver.GetSwitchesCount() < new_id)
            std::terminate();
        else
            myId = new_id;
    }
    //    void     SetNewDriver(std::shared_ptr<SwitchDriver> new_driver) { driver = std::move(new_driver); }

    void  SetState(State new_state) { driver.SetSwitchState(myId, new_state); }
    State GetState() const noexcept { return state; }

  private:
    std::shared_ptr<SwitchDriver> driver;
    Id                            myId;
    State                         state{ State::Disabled };
};