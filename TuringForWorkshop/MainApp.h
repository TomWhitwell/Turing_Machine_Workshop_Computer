// MainApp.h
#pragma once
#define COMPUTERCARD_NOIMPL
#include "ComputerCard.h"
#include "Clock.h"
#include "UI.h"
#include "Turing.h"
#include "Config.h"

class MainApp : public ComputerCard
{
    Config::Data *settings = nullptr;

public:
    MainApp();
    void ProcessSample() override;

    void PulseLed1(bool status);
    void PulseLed2(bool status);
    bool PulseOutput1(bool status);
    bool PulseOutput2(bool status);
    bool PulseInConnected1();
    bool PulseInConnected2();
    bool tapReceived();
    bool extPulse1Received();
    bool extPulse2Received();

    uint16_t KnobMain();
    uint16_t KnobX();
    uint16_t KnobY();
    bool ModeSwitch();

    void divideKnobChanged(uint8_t step);
    void lengthKnobChanged(uint8_t length);

    void updateMainTuring();
    void updateDivTuring();

    uint32_t MemoryCardID();

    void Housekeeping();

    void LoadSettings();

    uint64_t processTime;     // TESTING
    uint64_t lastProcessTime; // TESTING
    uint64_t processStepTime; // TESTING

    void blink(uint core, uint32_t interval_ms); // TESTING blinks LED related to core at given freq

    void updateLedState();

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

    volatile uint16_t CurrentBPM10 = 1200; // 10x bpm default
    volatile uint16_t newBPM10 = 0;        // 10x bpm default

    uint32_t lastTap = 0;
    uint32_t debounceTimeout = 480; // 10ms in 48khz clock ticks
    uint64_t lastChangeTimeUs;
    volatile bool pendingSave;

    bool pulseLed1_status = 0;
    bool pulseLed2_status = 0;

    enum LedMode
    {
        STATIC_PATTERN,
        DYNAMIC_PWM
    };
    LedMode ledMode = DYNAMIC_PWM;
    uint64_t lengthChangeStart = 0;
    void showLengthPattern(int length);
};
