#include "UI.h"
#include "Clock.h"
#define COMPUTERCARD_NOIMPL
#include "ComputerCard.h"
#include "MainApp.h"

void UI::init(MainApp *a, Clock *c)
{
    app = a;
    clk = c;
}

void UI::Tick()
{

    if (app->PulseInConnected1())
    {

        // don't do anything here if external pulse
    }
    else
    {
        // Internal clock: respond to ALL overflows (natural or from reset)
        if (clk->IsRisingEdge())
        {
            TriggerPulse1();
        }
    }

    if (clk->IsRisingEdgeMult() && !app->PulseInConnected2())
    {
        TriggerPulse2();
    }

    if (clk->ExtPulseReceived1())
    {
        TriggerPulse1();
    }

    if (clk->ExtPulseReceived2())
    {
        TriggerPulse2();
    }

    EndPulse1(); // Countdown pulse timer and check if Pulse1 should be stopped
    EndPulse2();

    // Trigger SlowUI every threshold ticks
    if (ui_counter++ >= threshold)
    {
        ui_counter = 0;
        SlowUI();
    }
}

uint8_t UI::QuantiseToStep(uint32_t knobVal, uint8_t steps, uint32_t range)
{
    uint16_t step_size = range / steps;
    return knobVal / step_size;
}

void UI::SlowUI()
{

    // Check for divide knob changes
    uint16_t knobTemp = app->KnobY();
    uint16_t step = QuantiseToStep(knobTemp, numDivideSteps, 4105); // Range of 4105 to ensure 0-8 range on 4096 pot
    if (step != lastDivideStep)
    {
        app->divideKnobChanged(step);
        lastDivideStep = step;
    }
}

void UI::TriggerPulse1()
{
    app->PulseOutput1(true);
    app->PulseLed1(true);
    outputPulseTicksRemaining1 = outputPulseLength;
    ledPulseTicksRemaining1 = ledPulseLength;
    ledPulseActive1 = true;
    outputPulseActive1 = true;
}

void UI::TriggerPulse2()
{
    app->PulseOutput2(true);
    app->PulseLed2(true);
    outputPulseTicksRemaining2 = outputPulseLength;
    ledPulseTicksRemaining2 = ledPulseLength;
    ledPulseActive2 = true;
    outputPulseActive2 = true;
}

void UI::EndPulse1()
{
    if (outputPulseActive1 && --outputPulseTicksRemaining1 == 0)
    {
        outputPulseActive1 = false;
        app->PulseOutput1(false);
    }

    if (ledPulseActive1 && --ledPulseTicksRemaining1 == 0)
    {
        ledPulseActive1 = false;
        app->PulseLed1(false);
    }
}

void UI::EndPulse2()
{

    if (outputPulseActive2 && --outputPulseTicksRemaining2 == 0)
    {
        outputPulseActive2 = false;
        app->PulseOutput2(false);
    }

    if (ledPulseActive2 && --ledPulseTicksRemaining2 == 0)
    {
        ledPulseActive2 = false;
        app->PulseLed2(false);
    }
}