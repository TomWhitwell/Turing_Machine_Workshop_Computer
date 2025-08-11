// MainApp.cpp
#include "MainApp.h"
#include <cstdio>
#include "pico/time.h" // temporary for testing
#include <inttypes.h>  // temporary for testing
#include "tusb.h"

// Config variables in HEX
const int CARD_NUMBER = 0x03;
const int MAJOR_VERSION = 0x04;
const int MINOR_VERSION = 0x00;
const int POINT_VERSION = 0x00;

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

void MainApp::UpdateNotePools()
{
    // create note pools for PWM precision CV outputs
    bool p = ModeSwitch();
    int base_note = 48; // C3
    int range = settings->preset[p].range;
    int scale = settings->preset[p].scale;
    turingPWM1.UpdateNotePool(base_note, range, scale);
    turingPWM2.UpdateNotePool(base_note, range, scale);
}

void MainApp::UpdatePulseLengths()
{

    bool p = ModeSwitch();
    uint8_t lengthMode = settings->preset[p].length;

    switch (lengthMode)
    {
    case 0:
        ui.SetPulseLength(1);
        ui.SetPulseMod(0);
        break;
    case 1:
        ui.SetPulseLength(25);
        ui.SetPulseMod(0);
        break;
    case 2:
        ui.SetPulseLength(50);
        ui.SetPulseMod(0);
        break;
    case 3:
        ui.SetPulseLength(75);
        ui.SetPulseMod(0);
        break;
    case 4:
        ui.SetPulseLength(99);
        ui.SetPulseMod(0);
        break;
    case 5:
        ui.SetPulseLength(15);
        ui.SetPulseMod(12);
        break;
    case 6:
        ui.SetPulseLength(50);
        ui.SetPulseMod(30);
        break;
    default:
        // Optional: Handle out-of-range values for `setting`
        ui.SetPulseLength(1);
        ui.SetPulseMod(0);
        break;
    }
}

void MainApp::LoadSettings(bool reset)
{
    // Load or initialise config
    cfg.load(reset); // 1 = force reset
    settings = &cfg.get();
    CurrentBPM10 = settings->bpm; // load bpm from settings file NB bpm always 10x i.e 1200 = 120.0 bpm.
    clk.setBPM10(CurrentBPM10);

    UpdateNotePools();
    UpdatePulseLengths();
    UpdateCh2Lengths();
    UpdateCVRange();
}

void __not_in_flash_func(MainApp::ProcessSample)()
{

    // TEST_write_to_Pulse(0, true); // pulse to test on oscilloscope

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

    // TEST_write_to_Pulse(0, false); // oscilloscope test
}

void MainApp::Housekeeping()
{

    static uint8_t packet[128];

    while (tud_midi_available())
    {
        size_t len = tud_midi_stream_read(packet, sizeof(packet));
        handleSysExMessage(packet, len);
    }

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

    ui.UpdatePulseMod(turingPulseLength1.DAC_8(), turingPulseLength2.DAC_8());

    UpdatePulseLengths();

    if (sendViz && tud_midi_n_mounted(0))
    {
        SendLiveStatus();
        sendViz = false;
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
    sendViz = true;
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

bool MainApp::SwitchDown()
{
    return SwitchVal() == Down;
}

bool MainApp::switchChanged()
{
    // 1 = up 0 = middle (or down)
    bool result = false;
    bool newSwitch = ModeSwitch();
    if (newSwitch != oldSwitch)
    {
        result = true;
        oldSwitch = newSwitch;
    }
    return result;
}

void MainApp::divideKnobChanged(uint8_t step)
{
    clk.UpdateDivide(step);
};

void MainApp::lengthKnobChanged(uint8_t length)
{

    bool p = ModeSwitch();

    int lengthPlus = settings->preset[p].looplen - 1; // Because 1-1 = 0, 0-1 = -1

    turingDAC1.updateLength(length);
    turingDAC2.updateLength(length + lengthPlus);
    turingPWM1.updateLength(length);
    turingPWM2.updateLength(length + lengthPlus);
    turingPulseLength1.updateLength(length);
    turingPulseLength2.updateLength(length + lengthPlus);

    // This is where to place the LED animation for length changes
    showLengthPattern(length);
    UpdatePulseLengths();
}

void MainApp::UpdateCh2Lengths()
{
    bool p = ModeSwitch();
    int lengthPlus = settings->preset[p].looplen - 1; // Because 1-1 = 0, 0-1 = -1
    uint16_t length = turingPWM1.returnLength();
    turingDAC2.updateLength(length + lengthPlus);
    turingPWM2.updateLength(length + lengthPlus);
    turingPulseLength2.updateLength(length + lengthPlus);
}

void MainApp::UpdateCVRange()
{
    bool p = ModeSwitch();
    int cvRange = settings->preset[p].cvRange;
    cv_set_mode(cvRange);
}

void MainApp::updateMainTuring()
{

    // Update Turing Machines
    turingDAC1.Update(KnobVal(Main), maxRange);
    turingPWM1.Update(KnobVal(Main), maxRange);
    turingPulseLength1.Update(KnobVal(Main), maxRange);

    // Scaled CV out on CV/Audio 1
    uint16_t dac = cv_map_u8(turingDAC1.DAC_8());
    AudioOut1(dac);

    int midi_note = turingPWM1.MidiNote();

    CVOut1MIDINote(midi_note);
}

void MainApp::updateDivTuring()
{
    turingDAC2.Update(KnobVal(Main), maxRange);
    turingPWM2.Update(KnobVal(Main), maxRange);
    turingPulseLength2.Update(KnobVal(Main), maxRange);

    // Scaled CV out on CV/Audio 2
    uint16_t dac = cv_map_u8(turingDAC2.DAC_8());
    AudioOut2(dac);

    CVOut2MIDINote(turingPWM2.MidiNote());
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

void MainApp::TEST_write_to_Pulse(int i, bool val)
{
    PulseOut(i, val);
}

void MainApp::sysexRespond()
{
    const uint8_t sysExStart = 0xF0;
    const uint8_t sysExEnd = 0xF7;
    const uint8_t manufacturerId = 0x7D;
    const uint8_t CARD_NUMBER = 0x03; // Card 03 = Turing Machine
    const uint8_t messageType = 0x02;

    const uint8_t *raw = reinterpret_cast<const uint8_t *>(settings);
    const size_t rawLen = sizeof(Config::Data);

    const size_t maxLen = 7 + ((rawLen + 6) / 7) * 8 + 1;
    uint8_t msg[maxLen];
    size_t out = 0;

    msg[out++] = sysExStart;
    msg[out++] = manufacturerId;
    msg[out++] = CARD_NUMBER;
    msg[out++] = messageType;
    msg[out++] = MAJOR_VERSION;
    msg[out++] = MINOR_VERSION;
    msg[out++] = POINT_VERSION;

    // Encodes the entire settings Data file from config.h
    // Includes:

    // Encode in 7-byte chunks with MSB prefix
    for (size_t i = 0; i < rawLen; i += 7)
    {
        uint8_t msb = 0;
        uint8_t block[7] = {0};

        for (size_t j = 0; j < 7; ++j)
        {
            size_t index = i + j;
            if (index >= rawLen)
                break;

            uint8_t byte = raw[index];
            if (byte & 0x80)
                msb |= (1 << j);

            block[j] = byte & 0x7F;
        }

        msg[out++] = msb;
        for (size_t j = 0; j < 7 && (i + j) < rawLen; ++j)
            msg[out++] = block[j];
    }

    msg[out++] = sysExEnd;
    tud_midi_stream_write(0, msg, out);
}

void MainApp::handleSysExMessage(const uint8_t *data, size_t len)
{
    if (len < 5 || data[0] != 0xF0 || data[len - 1] != 0xF7)
        return; // not a sysex message

    const uint8_t manufacturerId = data[1];
    const uint8_t deviceId = data[2];
    const uint8_t command = data[3];
    const uint8_t *payload = &data[4];
    const size_t payloadLen = len - 5;

    if (manufacturerId != 0x7D || deviceId != 0x03)
        return;

    switch (command)
    {
    case 0x01:
        sysexRespond();
        break;

    case 0x03:
    {
        uint8_t decoded[sizeof(Config::Data)] = {0};
        size_t in = 0, out = 0;

        while (in < payloadLen && out < sizeof(decoded))
        {
            uint8_t msb = payload[in++];
            for (int j = 0; j < 7 && in < payloadLen && out < sizeof(decoded); ++j)
            {
                uint8_t b = payload[in++];
                if (msb & (1 << j))
                    b |= 0x80;
                decoded[out++] = b;
            }
        }

        if (out == sizeof(Config::Data))
        {

            memcpy(settings, decoded, sizeof(Config::Data));

            // Before saving incoming config, overwrite BPM with corrrect local value

            settings->bpm = CurrentBPM10;

            cfg.save();
            LoadSettings(0);
        }
        break;
    }

    default:
        break;
    }
}

void MainApp::IdleLeds()
{
    static uint8_t tick = 0;

    // Use XOR to shuffle the pattern unpredictably
    uint8_t scrambled = tick ^ (tick << 1);
    uint8_t index = scrambled % 6;

    LedOn(index);
    sleep_us(20000); // ~20ms flash
    LedOff(index);

    tick++;
}

// Returns the high 7-bit MIDI-safe byte (bit 7 of input)
uint8_t MainApp::midiHi(uint8_t input)
{
    return (input >> 7) & 0x01;
}

// Returns the low 7-bit MIDI-safe byte (bits 0–6 of input)
uint8_t MainApp::midiLo(uint8_t input)
{
    return input & 0x7F;
}

void MainApp::SendLiveStatus()
{
    // No need for encoding, all bytes <127

    const uint8_t sysExStart = 0xF0;
    const uint8_t sysExEnd = 0xF7;
    const uint8_t manufacturerId = 0x7D;
    const uint8_t deviceId = 0x03;
    const uint8_t messageType = 0x10;

    uint8_t msg[16]; // CHECK THIS!
    size_t out = 0;

    msg[out++] = sysExStart;
    msg[out++] = manufacturerId;
    msg[out++] = deviceId;
    msg[out++] = messageType;

    msg[out++] = midiHi(turingDAC1.DAC_8());
    msg[out++] = midiLo(turingDAC1.DAC_8());

    msg[out++] = midiHi(turingDAC2.DAC_8());
    msg[out++] = midiLo(turingDAC2.DAC_8());

    msg[out++] = midiHi(turingPWM1.DAC_8());
    msg[out++] = midiLo(turingPWM1.DAC_8());

    msg[out++] = midiHi(turingPWM2.DAC_8());
    msg[out++] = midiLo(turingPWM2.DAC_8());

    msg[out++] = KnobVal(Main) >> 5; // 0-4095 down to 0-127
    msg[out++] = ModeSwitch();
    msg[out++] = turingPWM1.returnLength();

    msg[out++] = sysExEnd;

    tud_midi_stream_write(0, msg, out);
}

void MainApp::cv_map_build(int32_t low, int32_t high)
{
    const int32_t span = high - low;              // can be negative
    const int32_t lo = (low < high) ? low : high; // for safety clamp
    const int32_t hi = (low < high) ? high : low;

    for (int x = 0; x < 256; ++x)
    {
        // Exact linear map on 0..255 without overflow; signed-safe.
        // No rounding term so x=255 lands exactly on 'high'.
        int32_t y = low + (span * x) / 255;

        // Optional safety clamp to the given endpoints (supports low>high too)
        if (y < lo)
            y = lo;
        if (y > hi)
            y = hi;

        cv_lut[x] = (int16_t)y;
    }
}

void MainApp::cv_set_mode(uint8_t mode)
{
    // Matches your 4 ranges
    switch (mode)
    {
    case 0: /* ±6V  */
        cv_map_build(-2048, 2047);
        break;
    case 1: /* ±3V  */
        cv_map_build(-1024, 1024);
        break;
    case 2: /* 0..6V*/
        cv_map_build(0, 2047);
        break;
    case 3: /* 0..3V*/
        cv_map_build(0, 1024);
        break; // 0..+2.5V
    default:
        cv_map_build(-2048, 2047);
        break; // safe default
    }
}

int16_t MainApp::cv_map_u8(uint8_t x)
{
    return cv_lut[x]; // O(1) in the audio loop
}
