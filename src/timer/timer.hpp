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

#include <functional>

#include "FreeRTOS.h"
#include "FreeRTOS/include/timers.h"

class TimerFreeRTOS {
  public:
    using TimeT        = TickType_t;
    using CallbackType = std::function<void()>;
    enum {
        TicksToWait = 0
    };

    explicit TimerFreeRTOS(TimeT period) noexcept;
    TimerFreeRTOS(const TimerFreeRTOS &other);
    TimerFreeRTOS &operator=(const TimerFreeRTOS &rhs);
    TimerFreeRTOS(TimerFreeRTOS &&other);
    TimerFreeRTOS &operator=(TimerFreeRTOS &&rhs);
    ~TimerFreeRTOS() noexcept;

    void SetNewCallback(CallbackType &&new_callback, int call_delay_ms);
    void InvokeCallback();
    TimeT GetCurrentTime() const noexcept;
  protected:
  private:
    TimerHandle_t freertosTimer = static_cast<TimerHandle_t>(nullptr);
    CallbackType  callback;
};
