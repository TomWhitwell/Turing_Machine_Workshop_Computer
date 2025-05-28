#pragma once
#include <stdint.h>

class MainApp; // This lets us use MainApp* without needing the full definition
class Clock;   // also forward declare Clock

class UI
{
public:
    void Tick();
    void init(MainApp *app, Clock *clock); // Now takes both

private:
    void SlowUI();
    int ui_counter = 0;
    int threshold = 48;
    MainApp *app = nullptr;
    Clock *clk = nullptr; // pointer to Clock
};
