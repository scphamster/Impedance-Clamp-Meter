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

class AbstractPeripheral {
  public:
    enum class Status : bool {
        Enabled,
        Disabled
    };

    virtual void Initialize() noexcept = 0;
    virtual void Activate() noexcept   = 0;
    virtual void Deactivate() noexcept = 0;
    Status       GetStatus() const noexcept { return status; }
    void         SetStatus(Status new_status) noexcept { status = new_status; }

  private:
    Status status{ Status::Disabled };
};
