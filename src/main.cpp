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

#include <asf.h>
#include "system_init.h"
#include "DSP_functions.h"
#include "MCP23016.h"
#include "signal_conditioning.h"
#include "ILI9486_public.h"
#include "menu_ili9486_kbrd_mngr.h"
#include "menu_ili9486.h"

#include "FreeRTOS.h"
#include "task.h"
#include "maintask.hpp"
#include "hclamp_meter.hpp"

int
main()
{
    system_init();
    tasks_setup2();
}
