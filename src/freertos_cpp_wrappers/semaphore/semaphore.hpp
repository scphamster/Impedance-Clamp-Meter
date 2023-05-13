#pragma once
#include "misc/compiler_compatibility_workaround.hpp"

#include <memory>
#include <functional>
#include <type_traits>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/semphr.h"
#include "queue.h"

class Semaphore {
  public:
    using TimeT            = TickType_t;
    using ReceiveCallbackT = std::function<void()>;

    Semaphore() noexcept
      : semaphore{ xSemaphoreCreateBinary() }
    {
        if (semaphore == nullptr)
            std::terminate();
    }
    explicit Semaphore(ReceiveCallbackT &&callback)
      : Semaphore()
    {
        onReceiveCallback = std::move(callback);
    }
    Semaphore(Semaphore const &other)         = delete;
    Semaphore &operator=(Semaphore const &rh) = delete;
    Semaphore(Semaphore &&ohter)              = delete;
    Semaphore &operator=(Semaphore &&rh)      = delete;
    ~Semaphore() noexcept
    {
        if (semaphore)
            vSemaphoreDelete(semaphore);
    }

    void Give() noexcept { xSemaphoreGive(semaphore); }
    void SetOnReceiveCallback(ReceiveCallbackT &&callback) { onReceiveCallback = std::move(callback); }
    bool TakeWithTimeoutTicks(TimeT timeout_ticks) noexcept
    {
        return (xSemaphoreTake(semaphore, timeout_ticks) == pdTRUE) ? true : false;
    }
    bool TakeWithTimeoutMs(TimeT timeout_ms) noexcept { return TakeWithTimeoutTicks(pdMS_TO_TICKS(timeout_ms)); }
    bool TakeBlockInfinitely() noexcept { return TakeWithTimeoutTicks(portMAX_DELAY); }
    bool TakeImmediate() noexcept { return TakeWithTimeoutMs(0); }

  protected:
    void InvokeOnReceiveCallback() noexcept
    {
        if (onReceiveCallback)
            onReceiveCallback();
    }

  private:
    SemaphoreHandle_t semaphore = nullptr;
    ReceiveCallbackT  onReceiveCallback;
};