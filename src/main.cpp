#define BOARD_FREQ_SLCK_XTAL     32768UL
#define BOARD_FREQ_SLCK_BYPASS   32768UL
#define BOARD_FREQ_MAINCK_XTAL   20000000UL
#define BOARD_FREQ_MAINCK_BYPASS 20000000UL
#define BOARD_OSC_STARTUP_US     15625UL

// compiler ARM-GCC 10.3  error workaround
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include "system_init.h"
#include "tasks_controller.hpp"
#include "io.h"

int
main()
{
    system_init();
    tasks_setup2();
}
