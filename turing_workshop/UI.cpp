#include "UI.h"
#include "MainApp.h"
#include "Clock.h"

void UI::init(MainApp *a, Clock *c)
{
    app = a;
    clk = c;
}

void UI::Tick()
{

    // Trigger SlowUI every threshold ticks
    if (ui_counter++ >= threshold)
    {
        ui_counter = 0;
        SlowUI();
    }
}

void UI::SlowUI()
{
    bool ledStatus = ((clk->GetPhase()) < 0x08000000);
    app->PulseLed1(ledStatus);
}