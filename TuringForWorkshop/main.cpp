/************************************************************
 *  Core-split bootstrap for ComputerCard on RP2040
 *
 *  • Core 0 – USB-CDC stdio + flash-save service
 *  • Core 1 – ComputerCard audio engine (48 kHz ISR)
 *
 *  Requires:
 *    - CMakeLists.txt links pico_stdlib & pico_multicore
 *    - PICO_COPY_TO_RAM 1  (so flash stalls don’t hurt audio)
 ***********************************************************/

#include "pico/version.h"
#pragma message("SDK = " PICO_SDK_VERSION_STRING)

#include "ComputerCard.h"
#include "pico/multicore.h"
#include "Clock.h"
#include "UI.h"
#include "MainApp.h"
#include "pico/stdlib.h"
#include <cstdio>

/* Global handle published by Core 1 after MainApp is constructed */
static MainApp *volatile gApp = nullptr;

static void core1_entry()
{
    multicore_lockout_victim_init();
    // prinft("Core 1 running on core %d\n", get_core_num());

    static MainApp app; // all ComputerCard work lives here
    gApp = &app;        // publish pointer for Core 0
    multicore_fifo_push_blocking(reinterpret_cast<uintptr_t>(gApp));
    // prinft("MainApp constructed, pushed pointer\n");

    app.LoadSettings();
    // prinft("Settings loaded\n");
    app.EnableNormalisationProbe();
    app.Run(); // never returns
}

int main()
{

    stdio_usb_init(); // Initialize USB serial // Claims for Core 0
    sleep_ms(1000);

    // prinft("Main firing on core %d\n", get_core_num());

    // extern uint8_t __flash_binary_end;
    // // prinft("Flash ends at: 0x%08x\n", (uint32_t)&__flash_binary_end);

    /* 2 )  launch the audio engine on core 1 */
    // prinft("About to launch Core 1 from core %d\n", get_core_num());
    multicore_launch_core1(core1_entry);
    // prinft("Core 1 launched from core %d\n", get_core_num());

    /* 3 )  wait until Core 1 has published MainApp* (rarely more than 100 µs) */
    // while (!gApp)
    //     tight_loop_contents();

    if (!multicore_fifo_rvalid())
    {
        // prinft("Core 0: FIFO is empty!\n");
    }
    else
    {
        // prinft("I guess fifo is ok? \n");
    }

    uintptr_t ptr = multicore_fifo_pop_blocking();
    gApp = reinterpret_cast<MainApp *>(ptr);

    // prinft("MainApp successfully launched, watching from core %d\n", get_core_num());

    // Core 0 loads settings from flash
    gApp->LoadSettings();
    // prinft("Loaded settings on core %d\n", get_core_num());
    absolute_time_t next = make_timeout_time_ms(1);

    while (true)
    {
        gApp->Housekeeping();

        // ------------- pace the loop  ---------------
        sleep_until(next); // keeps 1-ms period
        next = delayed_by_ms(next, 1);
    }
}