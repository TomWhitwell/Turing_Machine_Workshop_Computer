// MainApp.h
#pragma once
#define COMPUTERCARD_NOIMPL
#include "ComputerCard.h"
#include "Clock.h"
#include "UI.h"
#include "Turing.h"
#include "Config.h"

#define RAM_FUNC __not_in_flash_func

class MainApp : public ComputerCard
{
    Config::Data *settings = nullptr;

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
    void lengthKnobChanged(uint8_t length);

    void updateMainTuring();
    void updateDivTuring();

    uint32_t MemoryCardID();

private:
    Clock clk;
    UI ui;
    Config cfg;

    Turing turingDAC1;
    Turing turingDAC2;
    Turing turingPWM1;
    Turing turingPWM2;
    Turing turingPulseLength1;
    Turing turingPulseLength2;
    uint16_t maxRange = 4095; // maximum pot value

    uint16_t CurrentBPM10 = 1200; // 10x bpm default
    uint16_t newBPM10 = 0;        // 10x bpm default

    uint32_t lastTap = 0;
    uint32_t debounceTimeout = 480; // 10ms in 48khz clock ticks

    uint32_t hk_threshold = 48; // calls Housekeeping every 1ms
    void Housekeeping();
};
