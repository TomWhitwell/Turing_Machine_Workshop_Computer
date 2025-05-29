

#include "ComputerCard.h"
#include "Clock.h"
#include "UI.h"
#include "MainApp.h"
#include "pico/stdlib.h"
#include <cstdio>

int main()
{

    stdio_usb_init(); // Initialize USB serial
    // stdio_init_all(); // Initializes whatever stdio you’ve enabled in CMake (USB, UART, or semihosting)

    sleep_ms(2000); // Optional: wait for USB host connection

    MainApp app;
    app.EnableNormalisationProbe();
    app.Run(); // Starts the main audio loop
}
