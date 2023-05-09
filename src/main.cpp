#define BOARD_FREQ_SLCK_XTAL     32768UL
#define BOARD_FREQ_SLCK_BYPASS   32768UL
#define BOARD_FREQ_MAINCK_XTAL   20000000UL
#define BOARD_FREQ_MAINCK_BYPASS 20000000UL
#define BOARD_OSC_STARTUP_US     15625UL

#include "system_init.h"
#include "tasks_controller.hpp"

int
main()
{
    system_init();
    tasks_setup();
}
