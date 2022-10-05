#include "timer.hpp"
#include "arm_math.h"

extern "C" void
TimerFreeRTOS_CallbacksWrapper(TimerHandle_t called_from_timer)
{
    auto cpp_timer_instance = static_cast<TimerFreeRTOS *>(pvTimerGetTimerID(called_from_timer));
    cpp_timer_instance->InvokeCallback();
}

TimerFreeRTOS::TimerFreeRTOS(TimerFreeRTOS::TickT period) noexcept
  : freertosTimer{
      xTimerCreate("cpptimer", period, pdFALSE, static_cast<void *const>(this), TimerFreeRTOS_CallbacksWrapper)
  }
{ }

TimerFreeRTOS::~TimerFreeRTOS() noexcept
{
    xTimerDelete(freertosTimer, TicksToWait);
}

void
TimerFreeRTOS::SetNewCallback(TimerFreeRTOS::CallbackType &&new_callback, int call_delay_ms)
{
    xTimerChangePeriod(freertosTimer, pdMS_TO_TICKS(call_delay_ms), TicksToWait);   // also starts the timer
    callback = std::move(new_callback);
}

void
TimerFreeRTOS::InvokeCallback()
{
    callback();
}
