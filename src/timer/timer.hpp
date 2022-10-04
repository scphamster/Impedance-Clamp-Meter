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
    using TickT        = TickType_t;
    using CallbackType = std::function<void()>;

    TimerFreeRTOS(TickT period);

    void SetNewCallback(CallbackType &&new_callback);

  protected:
  private:
    TimerHandle_t freertosTimer = static_cast<TimerHandle_t>(nullptr);
    CallbackType  callback;
};
