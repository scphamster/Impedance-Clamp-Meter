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

    Task(TaskFunctor &&new_functor, size_t stacksize, size_t priority, std::string new_name)
      : task{ std::move(new_functor) }
      , name{ std::move(new_name) }
    {
        auto task_creation_result = xTaskCreate(TaskCppTaskWrapper, name.c_str(), stacksize, this, priority, &taskHandle);
        configASSERT(task_creation_result == pdPASS);
    }

    Task(Task const &)            = delete;
    Task &operator=(Task const &) = delete;

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

    ~Task() noexcept
    {
        if (taskHandle != nullptr)
            vTaskDelete(taskHandle);
    }

  private:
    TaskHandle_t taskHandle;
    TaskFunctor  task;
    std::string  name;
};