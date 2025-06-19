// MainApp.cpp
#include "MainApp.h"
#include <cstdio>
#include "pico/time.h" // temporary for testing

MainApp::MainApp()

    // Initialise the Turing machines with variations of the memory card ID, unique but not random
    : turingDAC1(8, MemoryCardID()),
      turingDAC2(8, MemoryCardID() * 2),
      turingPWM1(8, MemoryCardID() * 3),
      turingPWM2(8, MemoryCardID() * 4),
      turingPulseLength1(8, MemoryCardID() * 5),
      turingPulseLength2(8, MemoryCardID() * 6)
{

    // Load or initialise config
    cfg.load(0); // 1 = force reset
    // auto &settings = cfg.get();
    settings = &cfg.get();

    // printf("Range 0: %d, Divide: %d\n", settings.preset[0].range, settings.divide);
    // printf("Range 1: %d, Divide: %d\n", settings.preset[1].range, settings.divide);
    printf("BPM %f\n", ((float)settings->bpm / 10.0));

    settings->preset[1].range = 23;
    cfg.save();

    CurrentBPM10 = settings->bpm; // load bpm from settings file NB bpm always 10x i.e 1200 = 120.0 bpm.
    clk.setBPM10(CurrentBPM10);
    // clk.SetPhaseIncrement(178957);
    ui.init(this, &clk);
}

void(MainApp::ProcessSample)()
{
    // Call tap before ui.tick and before clk.tick, so that reset triggered tap is tapped make it to ui.
    if (tapReceived())
    {
        uint32_t now = clk.GetTicks();
        if (now - lastTap > debounceTimeout)
        {
            newBPM10 = clk.TapTempo(now);
            lastTap = now;
        }
        else
        {
            // Do nothing, debounced click
        }
    }

    if (extPulse1Received())
    {
        uint32_t now = clk.GetTicks();
        clk.TapTempo(now);
        clk.ExtPulse1();
    }

    if (extPulse2Received())
    {

        clk.ExtPulse2();
    }

    clk.Tick();
    ui.Tick();

    // CVOut1((clk.GetPhase() >> 20) - 2048); // just for debugging, remove
    // CVOut2((clk.TEST_subclock_phase >> 20) - 2048);
}

void MainApp::Housekeeping()
{
    // runs at

    static uint8_t taskIndex = 0;

    switch (taskIndex)
    {
    case 0:

        // Task 0 If BPM10 changed, save to flash
        if (newBPM10 > 0 && newBPM10 != CurrentBPM10)
        {

            settings->bpm = newBPM10;
            CurrentBPM10 = newBPM10;
            newBPM10 = 0;
            printf("starting to save\n");
            uint64_t start_time = time_us_64(); // TESTING
            cfg.save();

            uint64_t end_time = time_us_64();              // TESTING
            uint64_t duration_us = end_time - start_time;  // TESTING
            printf("Saving time: %llu µs\n", duration_us); // TESTING
        }
        break;
    case 1:
        // Task 1:
        printf("Housekeeping firing task %d on core %d\n", taskIndex, get_core_num());
        break;
    case 2:
        // Task 2:
        printf("Housekeeping firing task %d on core %d\n", taskIndex, get_core_num());
        break;
    }

    taskIndex++;
    if (taskIndex > 2) // Update this to match your last case number
        taskIndex = 0;
}

void MainApp::PulseLed1(bool status)
{
    LedOn(4, status);
}

void MainApp::PulseLed2(bool status)
{
    LedOn(5, status);
}

void MainApp::PulseOutput1(bool status)
{
    PulseOut1(status);
}

void MainApp::PulseOutput2(bool status)
{
    PulseOut2(status);
}

bool MainApp::PulseInConnected1()
{
    return Connected(Pulse1);
}

bool MainApp::PulseInConnected2()
{
    return Connected(Pulse2);
}

bool(MainApp::tapReceived)()
{
    if (PulseInConnected1())
    {
        return false;
    }
    else
    {
        clk.setExternalClock1(false);
        return (SwitchChanged() && SwitchVal() == Down);
    }
}

bool MainApp::extPulse1Received()
{
    if (PulseInConnected1() && PulseIn1RisingEdge())
    {
        clk.setExternalClock1(true);
        return true;
    }
    else
    {
        return false;
    }
}

bool MainApp::extPulse2Received()
{
    if (PulseInConnected2() && PulseIn2RisingEdge())
    {

        clk.setExternalClock2(true);
        return true;
    }
    else
    {
        return false;
    }
}

uint16_t MainApp::KnobMain()
{
    return KnobVal(Main);
}
uint16_t MainApp::KnobX()
{
    return KnobVal(X);
}
uint16_t MainApp::KnobY()
{
    return KnobVal(Y);
}

void MainApp::divideKnobChanged(uint8_t step)
{
    clk.UpdateDivide(step);
};

void MainApp::lengthKnobChanged(uint8_t length)
{
    turingDAC1.updateLength(length);
    turingDAC2.updateLength(length);
    turingPWM1.updateLength(length);
    turingPWM2.updateLength(length);
    turingPulseLength1.updateLength(length);
    turingPulseLength2.updateLength(length);
}

void MainApp::updateMainTuring()
{

    // Update Turing Machines
    turingDAC1.Update(KnobVal(Main), maxRange);
    turingPWM1.Update(KnobVal(Main), maxRange);
    turingPulseLength1.Update(KnobVal(Main), maxRange);

    AudioOut1(turingDAC1.DAC_8() << 4);

    // test settings
    int low_note = 48;  // c3
    int high_note = 84; // c6
    int scale = 3;      // minor pent
    int sieve = 0;      // scale
    int midi_note = turingPWM1.MidiNote(low_note, high_note, scale, sieve);
    CVOut1MIDINote(midi_note);
}

void MainApp::updateDivTuring()
{
    turingDAC2.Update(KnobVal(Main), maxRange);
    turingPWM2.Update(KnobVal(Main), maxRange);
    turingPulseLength2.Update(KnobVal(Main), maxRange);

    AudioOut2(turingDAC2.DAC_8() << 4);

    // test settings
    int low_note = 48;  // c3
    int high_note = 84; // c6
    int scale = 3;      // minor pent
    int sieve = 0;      // scale
    int midi_note = turingPWM2.MidiNote(low_note, high_note, scale, sieve);
    CVOut2MIDINote(midi_note);
}

uint32_t MainApp::MemoryCardID()
{
    return static_cast<uint32_t>(UniqueCardID());
}