#include "timer.hpp"

extern "C" void
TimerFreeRTOS_CallbacksWrapper(TimerHandle_t called_from_timer)
{ }

TimerFreeRTOS::TimerFreeRTOS(TimerFreeRTOS::TickT period)
  : freertosTimer{ xTimerCreate("cpptimer", period, false, nullptr, TimerFreeRTOS_CallbacksWrapper) }
{

}

void
TimerFreeRTOS::SetNewCallback(TimerFreeRTOS::CallbackType &&new_callback)
{
    callback = std::move(new_callback);
}
