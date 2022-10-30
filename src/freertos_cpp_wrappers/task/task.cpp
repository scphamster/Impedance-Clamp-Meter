#include "task.hpp"

extern "C" void
TaskCppTaskWrapper(void *param)
{
    static_cast<Task *>(param)->Run();
}

[[noreturn]] void
TaskLoopBreachHook(TaskHandle_t task, std::string name)
{
    (void)task;
    (void)name;

    while (true)
        ;
}

[[noreturn]] void
TaskFunctorIsNullHook()
{
    for (;;)
        ;
}