#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif
#include "tasks_controller.hpp"
#include "main_task.hpp"

#include "task.h"
#include "display_drawer.hpp"
#include "ili9486_driver.hpp"

using Drawer                      = DisplayDrawer<ILI9486Driver>;
using Clamp                       = TasksControllerImplementation<Drawer>;
using KeyboardT                   = Keyboard<MCP23016_driver, TimerFreeRTOS, MCP23016Button>;
bool ILI9486Driver::isInitialized = false;

[[noreturn]] void
tasks_setup2()
{

    static volatile auto clamp_meter =
      Clamp{ std::make_unique<DisplayDrawer<ILI9486Driver>>(std::make_shared<ILI9486Driver>()),
             std::make_unique<KeyboardT>() };

    //    measurement_start();
    vTaskStartScheduler();
    while (true) { }
}
