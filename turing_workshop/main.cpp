#include "MainApp.h"
#include "UI.h"

MainApp::MainApp()
{
    ui.init(this, &clk);           // pass both MainApp and Clock*
    clk.SetPhaseIncrement(178957); // ~120 BPM at 48kHz
}

void MainApp::ProcessSample()
{
    clk.Tick();
    ui.Tick();
}

void MainApp::PulseLed1(bool status)
{
    LedOn(4, status);
}

void MainApp::PulseLed2(bool status)
{
    LedOn(5, status);
}

int main()
{
    MainApp app;
    app.EnableNormalisationProbe();
    app.Run(); // Starts the main audio loop
}
