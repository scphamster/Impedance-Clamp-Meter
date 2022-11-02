#pragma once
#include "compiler_compatibility_workaround.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include <functional>
#include <string>

extern "C" void   TaskCppTaskWrapper(void *);
[[noreturn]] void TaskLoopBreachHook(TaskHandle_t task, std::string name);
[[noreturn]] void TaskFunctorIsNullHook();

class Task {
  public:
    using TaskFunctor = std::function<void()>;
    using TimeT       = portTickType;
    using TickT       = portTickType;
    enum {
        TaskCreationSucceeded = pdPASS
    };
    Task(TaskFunctor &&new_functor, size_t stacksize, size_t new_priority, std::string new_name)
      : task{ std::move(new_functor) }
      , name{ std::move(new_name) }
      , stackSize{ stacksize }
      , priority{ new_priority }
    {
        configASSERT(Initialize());
    }

    Task(Task const &)            = delete;
    Task &operator=(Task const &) = delete;
    ~Task() noexcept { DeleteTask(); }

    void Reset() noexcept
    {
        Suspend();
        vTaskDelete(taskHandle);
        configASSERT(Initialize());
    }

    [[noreturn]] void Run() noexcept
    {
        if (not task)
            TaskFunctorIsNullHook();

        task();

        // should not get here
        TaskLoopBreachHook(taskHandle, name);
    }
    void Suspend() noexcept
    {
        if (not task)
            TaskFunctorIsNullHook();

        vTaskSuspend(taskHandle);
    }
    void Resume() noexcept
    {
        if (not task)
            TaskFunctorIsNullHook();

        vTaskResume(taskHandle);
    }

    static void DelayMs(TimeT for_ms) noexcept { vTaskDelay(pdMS_TO_TICKS(for_ms)); }
    static void DelayTicks(TickT ticks) noexcept { vTaskDelay(ticks); }
    static void DelayMsUntil(TimeT for_ms) noexcept
    {
        auto time_now = xTaskGetTickCount();
        vTaskDelayUntil(&time_now, pdMS_TO_TICKS(for_ms));
    }

  protected:
    bool Initialize() noexcept
    {
        return (xTaskCreate(TaskCppTaskWrapper, name.c_str(), stackSize, this, priority, &taskHandle) == TaskCreationSucceeded)
                 ? true
                 : false;
    }
    void DeleteTask() noexcept
    {
        if (taskHandle != nullptr)
            vTaskDelete(taskHandle);
    }

  private:
    TaskHandle_t taskHandle;
    TaskFunctor  task;

    std::string name;
    size_t      stackSize;
    size_t      priority;
};