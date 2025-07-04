// MainApp.cpp
#include "MainApp.h"
#include <cstdio>
#include "pico/time.h" // temporary for testing
#include <inttypes.h>  // temporary for testing

MainApp::MainApp()

    // Initialise the Turing machines with variations of the memory card ID, unique but not random
    : turingDAC1(8, MemoryCardID()),
      turingDAC2(8, MemoryCardID() * 2),
      turingPWM1(8, MemoryCardID() * 3),
      turingPWM2(8, MemoryCardID() * 4),
      turingPulseLength1(8, MemoryCardID() * 5),
      turingPulseLength2(8, MemoryCardID() * 6)

{

    ui.init(this, &clk);
}

void MainApp::LoadSettings()
{

    // Load or initialise config
    cfg.load(0); // 1 = force reset
    settings = &cfg.get();
    CurrentBPM10 = settings->bpm; // load bpm from settings file NB bpm always 10x i.e 1200 = 120.0 bpm.
    clk.setBPM10(CurrentBPM10);
}

void __not_in_flash_func(MainApp::ProcessSample)()
{

    // Call tap before ui.tick and before clk.tick, so that reset triggered tap is tapped make it to ui.
    if (tapReceived())
    {
        uint32_t now = clk.GetTicks();
        uint16_t tempBPM = clk.TapTempo(now);
        if (tempBPM > 0 && newBPM10 == 0)
        {
            newBPM10 = tempBPM;
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

    blink(1, 50); // show that Core 1 is alive
}

void MainApp::Housekeeping()
{

    LedOn(2, pendingSave);
    uint64_t nowUs = time_us_64();

    // BPM changed?

    if (newBPM10 > 0 && newBPM10 < 8000 && newBPM10 != CurrentBPM10)
    {
        settings->bpm = newBPM10;
        CurrentBPM10 = newBPM10;
        newBPM10 = 0;

        lastChangeTimeUs = nowUs;
        pendingSave = true;
    }
    else if (newBPM10 > 0 && (newBPM10 >= 8000 || newBPM10 == CurrentBPM10))
    {
        // Clear invalid or duplicate BPM
        newBPM10 = 0;
    }

    // Has 2 seconds passed since last change, and save is pending?
    if (pendingSave && (nowUs - lastChangeTimeUs >= 2000000))
    {
        cfg.save();
        pendingSave = false;
    }

    blink(0, 250); // show that Core 0 is alive

    ui.SlowUI(); // call knob checking etc
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

bool MainApp::ModeSwitch()
{ // 1 = up 0 = middle (or down)
    return SwitchVal() == Up;
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
    // int low_note = 48;  // c3
    // int high_note = 84; // c6
    // int scale = 3;      // minor pent
    // int sieve = 0;      // scale
    // int midi_note = turingPWM2.MidiNote(low_note, high_note, scale, sieve);

    bool p = ModeSwitch();
    int base_note = 36; // C2
    int range = settings->preset[p].range;
    int low_note = base_note;
    int high_note = base_note + range * 12 + 12; // covers (range + 1) octaves

    int midi_note = turingPWM1.MidiNote(
        low_note,
        high_note,
        settings->preset[p].scale,
        settings->preset[p].notes);

    CVOut2MIDINote(midi_note);
}

uint32_t MainApp::MemoryCardID()
{
    return static_cast<uint32_t>(UniqueCardID());
}

void MainApp::blink(uint core, uint32_t interval_ms)
{

    // uint pin = get_core_num();
    uint pin = core;
    static absolute_time_t next_toggle_time[32]; // indexed by GPIO
    static bool led_state[32] = {false};         // indexed by GPIO

    if (absolute_time_diff_us(get_absolute_time(), next_toggle_time[pin]) < 0)
    {
        led_state[pin] = !led_state[pin];
        LedOn(pin, led_state[pin]);

        next_toggle_time[pin] = make_timeout_time_ms(interval_ms);
    }
}
