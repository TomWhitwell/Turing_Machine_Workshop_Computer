// MainApp.cpp
#include "MainApp.h"
#include <cstdio>
#include "pico/time.h" // temporary for testing
#include <inttypes.h>  // temporary for testing
#include "tusb.h"

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

    // blink(1, 50); // show that Core 1 is alive
}

void MainApp::Housekeeping()
{

    // LedOn(2, pendingSave);
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

    // blink(0, 250); // show that Core 0 is alive

    ui.SlowUI(); // call knob checking etc

    updateLedState();

    // MIDI processing (Core 0 only)
    static uint8_t packet[64];

    while (tud_midi_available())
    {
        size_t len = tud_midi_stream_read(packet, sizeof(packet));
        handleSysExMessage(packet, len);
    }
}

void MainApp::PulseLed1(bool status)
{
    pulseLed1_status = status;
}

void MainApp::PulseLed2(bool status)
{

    pulseLed2_status = status;
}

bool MainApp::PulseOutput1(bool requested)
{
    bool emit = false;
    bool isTuringMode = settings->preset[ModeSwitch()].pulseMode1;

    if (isTuringMode && requested)
    {
        emit = (turingPWM1.DAC_8() & 0x01);
    }
    else
    {
        emit = requested;
    }

    PulseOut1(emit);
    return emit;
}

bool MainApp::PulseOutput2(bool requested)
{
    bool emit = false;
    bool isTuringMode = settings->preset[ModeSwitch()].pulseMode2;

    if (isTuringMode && requested)
    {
        emit = (turingPWM2.DAC_8() & 0x01);
    }
    else
    {
        emit = requested;
    }

    PulseOut2(emit);
    return emit;
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

    // This is where to place the LED animation for length changes
    showLengthPattern(length);
}

void MainApp::updateMainTuring()
{

    // Update Turing Machines
    turingDAC1.Update(KnobVal(Main), maxRange);
    turingPWM1.Update(KnobVal(Main), maxRange);
    turingPulseLength1.Update(KnobVal(Main), maxRange);

    AudioOut1(turingDAC1.DAC_8() << 4);

    bool p = ModeSwitch();
    int base_note = 48; // C3
    int range = settings->preset[p].range;
    int low_note = base_note;
    int high_note = base_note + range * 12 + 12; // covers (range + 1) octaves

    int midi_note = turingPWM1.MidiNote(
        low_note,
        high_note,
        settings->preset[p].scale,
        settings->preset[p].notes);

    CVOut1MIDINote(midi_note);
}

void MainApp::updateDivTuring()
{
    turingDAC2.Update(KnobVal(Main), maxRange);
    turingPWM2.Update(KnobVal(Main), maxRange);
    turingPulseLength2.Update(KnobVal(Main), maxRange);

    AudioOut2(turingDAC2.DAC_8() << 4);

    bool p = ModeSwitch();
    int base_note = 48; // C3
    int range = settings->preset[p].range;
    int low_note = base_note;
    int high_note = base_note + range * 12 + 12; // covers (range + 1) octaves

    int midi_note = turingPWM2.MidiNote(
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

void MainApp::showLengthPattern(int length)
{
    struct PatternEntry
    {
        int length;
        uint8_t bitmask;
    };

    const PatternEntry patternTable[] = {
        {2, 0b110000},
        {3, 0b111000},
        {4, 0b111100},
        {5, 0b111110},
        {6, 0b111111},
        {8, 0b001111},
        {12, 0b000011},
        {16, 0b110011}};

    uint8_t mask = 0;

    ledMode = STATIC_PATTERN;
    lengthChangeStart = time_us_64();

    for (const auto &entry : patternTable)
    {
        if (entry.length == length)
        {
            mask = entry.bitmask;
            break;
        }
    }

    for (int i = 0; i < 6; ++i)
    {
        if (mask & (1 << (5 - i)))
        {
            LedOn(i);
        }
        else
        {
            LedOff(i);
        }
    }
}

void MainApp::updateLedState()
{

    if (ledMode == DYNAMIC_PWM)
    {

        LedBrightness(0, turingDAC1.DAC_8() << 4);
        LedBrightness(1, turingDAC2.DAC_8() << 4);
        LedBrightness(2, turingPWM1.DAC_8() << 4);
        LedBrightness(3, turingPWM2.DAC_8() << 4);
        LedOn(4, pulseLed1_status);
        LedOn(5, pulseLed2_status);
    }
    else if (ledMode == STATIC_PATTERN)
    {

        if (time_us_64() - lengthChangeStart > 1500000)
        { // 1.5 seconds in µs
            ledMode = DYNAMIC_PWM;
        }
    }
}

// Encodes binary data into 7-bit-safe format (MIDI-safe)
// Each input byte is split if it has MSB set
size_t encode7bit(const uint8_t *input, uint8_t *output, size_t len)
{
    size_t outIndex = 0;
    for (size_t i = 0; i < len; ++i)
    {
        uint8_t b = input[i];
        output[outIndex++] = b & 0x7F;
        if (b > 0x7F)
            output[outIndex++] = (b >> 7) & 0x01; // Keep only the 8th bit as a flag
    }
    return outIndex;
}

void MainApp::sysexRespond()
{
    const uint8_t sysExStart = 0xF0;
    const uint8_t sysExEnd = 0xF7;
    const uint8_t manufacturerId = 0x7D; // Non-commercial use
    const uint8_t deviceId = 0x01;
    const uint8_t messageType = 0x02; // Config dump

    const uint8_t *raw = reinterpret_cast<const uint8_t *>(settings);
    const size_t rawLen = sizeof(Config::Data);

    uint8_t msg[4 + rawLen + 1];
    size_t i = 0;

    msg[i++] = sysExStart;
    msg[i++] = manufacturerId;
    msg[i++] = deviceId;
    msg[i++] = messageType;

    memcpy(&msg[i], raw, rawLen);
    i += rawLen;

    msg[i++] = sysExEnd;

    tud_midi_stream_write(0, msg, i);
}

void MainApp::handleSysExMessage(const uint8_t *data, size_t len)
{
    // Basic validation
    if (len < 5 || data[0] != 0xF0 || data[len - 1] != 0xF7)
        return;

    const uint8_t manufacturerId = data[1];
    const uint8_t deviceId = data[2];
    const uint8_t command = data[3];

    if (manufacturerId != 0x7D || deviceId != 0x01)
        return;

    const uint8_t *payload = &data[4];
    const size_t payloadLen = len - 5; // exclude F0, 7D, 01, cmd, F7

    switch (command)
    {
    case 0x01: // "Hello" → send config dump
        sysexRespond();
        break;

    case 0x03: // Apply config
        if (payloadLen == sizeof(Config::Data))
        {
            memcpy(settings, payload, sizeof(Config::Data));
            cfg.save(); // save to flash
        }
        break;

    // TODO: Add more cases here
    // case 0x04: send firmware version
    // case 0x05: apply partial settings
    default:
        break;
    }
}
