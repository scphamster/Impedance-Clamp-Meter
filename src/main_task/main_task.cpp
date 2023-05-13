#include "app_controller.hpp"
#include "main_task.hpp"

#include "task.h"
#include "display_drawer.hpp"
#include "ili9486_driver.hpp"

using Drawer                      = DisplayDrawer<ILI9486Driver>;
using Controller                  = AppController<Drawer>;
using KeyboardT                   = Keyboard<MCP23016_driver, TimerFreeRTOS, MCP23016Button>;
bool ILI9486Driver::isInitialized = false;

[[noreturn]] void
tasks_setup()
{
    static volatile auto clamp_meter =
      Controller{ std::make_unique<DisplayDrawer<ILI9486Driver>>(std::make_shared<ILI9486Driver>()),
             std::make_unique<KeyboardT>() };

    vTaskStartScheduler();

    //should never get here
    while (true) { }
}
