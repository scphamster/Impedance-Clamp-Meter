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
        auto task_creation_result = xTaskCreate(TaskCppTaskWrapper, name.c_str(), stacksize, this, priority, &handle);
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
        TaskLoopBreachHook(handle, name);
    }

    ~Task() noexcept
    {
        if (handle != nullptr)
            vTaskDelete(handle);
    }

  private:
    TaskHandle_t handle;
    TaskFunctor  task;
    std::string  name;
};