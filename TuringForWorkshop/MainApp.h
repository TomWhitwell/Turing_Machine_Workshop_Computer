// MainApp.h
#pragma once
#define COMPUTERCARD_NOIMPL
#include "ComputerCard.h"
#include "Clock.h"
#include "UI.h"

class MainApp : public ComputerCard
{
public:
    MainApp();
    void ProcessSample() override;

    void PulseLed1(bool status);
    void PulseLed2(bool status);
    void PulseOutput1(bool status);
    void PulseOutput2(bool status);
    bool PulseInConnected1();
    bool PulseInConnected2();
    bool tapReceived();
    bool extPulse1Received();
    bool extPulse2Received();

    uint16_t KnobMain();
    uint16_t KnobX();
    uint16_t KnobY();

    void divideKnobChanged(uint8_t step);

private:
    Clock clk;
    UI ui;
};
