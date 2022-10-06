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

extern int g_testvar;

class TimerFreeRTOS {
  public:
    using TickT        = TickType_t;
    using CallbackType = std::function<void()>;
    enum {
        TicksToWait = 0
    };

    explicit TimerFreeRTOS(TickT period) noexcept;
    ~TimerFreeRTOS() noexcept;

    void SetNewCallback(CallbackType &&new_callback, int call_delay_ms);
    void InvokeCallback();

  protected:
  private:
    TimerHandle_t freertosTimer = static_cast<TimerHandle_t>(nullptr);
    CallbackType  callback;
};
