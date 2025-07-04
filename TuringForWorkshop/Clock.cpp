#include "Clock.h"

void Clock::Tick()

{

    const uint32_t prev = phase;  // store old phase
    phase += phase_increment;     // increment phase
    rising_edge = (prev > phase); // detect wraparound for main clock

    uint32_t prev_prod = (uint64_t)prev * subclockMultiplier;  // multiply old phase
    uint32_t curr_prod = (uint64_t)phase * subclockMultiplier; // multiply new phase

    if (prev_prod > curr_prod) 
        ++subclockCount;

    totalTicks++;

    if (!isExternalClock1)
    // Internal Clock
    {
        if (subclockCount >= subclockDividor) // subclockDividor is set by knob Y
        {
            rising_edge_mult = true; // This is the line that creates the output pulse
            subclockCount = 0;
            subclockSync = true;
        }
        else
        {
            rising_edge_mult = false;
        }
    }
    else
    {
        // External Clock
        if (subclockCount >= subclockDividor) // subclockDividor is set by knob Y
        {
            rising_edge_mult = true; // This is the line that creates the output pulse
            subclockCount = 0;
            subclockSync = true;
        }
        else
        {
            rising_edge_mult = false;
        }
    }

    if (subclockSync && rising_edge)
    {
        subclockCount = 0;
        subclockSync = false;
    }
}

void Clock::Reset()
{

    if (isExternalClock1)
    {
        if (phase > PHASE_WRAP_THRESHOLD)
        {
            // If we're near the top already, don't reset to avoid double trigger
            return;
        }
    }
    phase = 0;

    rising_edge = true;
    subclockSync = false;
}

void Clock::SetPhaseIncrement(uint32_t increment)
{
    phase_increment = increment;
}

uint32_t Clock::GetPhase() const
{
    return phase;
}

bool Clock::IsRisingEdge() const
{
    return rising_edge;
}

bool Clock::IsRisingEdgeMult() const
{
    return rising_edge_mult;
}

uint32_t Clock::GetTicks() const
{
    return totalTicks;
}

void Clock::SetPhaseIncrementFromTicks(uint32_t ticks_per_beat)
{
    if (ticks_per_beat == 0)
        return;
    phase_increment = (uint64_t(1) << 32) / ticks_per_beat;
}

void Clock::SetPhaseIncrementFromBPM10(uint16_t BPM10)
{
    if (BPM10 == 0)
        return;

    // BPM10 = BPM * 10
    // Beats per second = BPM10 / 600
    // ticks_per_beat = clockSpeed / (BPM10 / 600)
    // Simplified to: ticks_per_beat = (clockSpeed * 600) / BPM10

    uint32_t ticks_per_beat = (clockSpeed * 600UL) / BPM10;
    phase_increment = (uint64_t(1) << 32) / ticks_per_beat;
}

uint16_t Clock::GetBPM10FromPhaseIncrement()
{
    if (phase_increment == 0)
        return 0;

    // Multiply first as 64-bit to avoid overflow
    uint64_t temp = (uint64_t)phase_increment * clockSpeed * 600ULL;
    uint32_t bpm10 = temp >> 32; // equivalent to dividing by 2^32

    return (uint16_t)bpm10;
}

// uint16_t Clock::TapTempo(uint32_t tapTime)
// {

//     uint16_t localBPM10 = 0;
//     if (lastTapTime != 0)
//     {
//         uint32_t interval = tapTime - lastTapTime;

//         if (interval > minInterval && interval < maxInterval)
//         {
//             SetPhaseIncrementFromTicks(interval);
//             localBPM10 = GetBPM10FromPhaseIncrement();
//         }
//         else
//         {
//             lastTapTime = 0; // reset tap system
//             return 0;
//         }
//     }
//     lastTapTime = tapTime;
//     Reset();
//     return localBPM10;
// }

uint16_t Clock::TapTempo(uint32_t tapTime)
{
    if (lastTapTime == 0)
    {
        lastTapTime = tapTime;
        return 0; // First tap: not enough data to calculate BPM
    }

    uint32_t interval = tapTime - lastTapTime;

    if (interval < minInterval || interval > maxInterval)
    {
        lastTapTime = 0; // Reset on invalid tap
        return 0;
    }

    lastTapTime = tapTime;
    SetPhaseIncrementFromTicks(interval);
    Reset();
    return GetBPM10FromPhaseIncrement();
}

void Clock::UpdateDivide(uint8_t step)
{
    subclockDividor = subclockDivisions[step];
    subclockSync = true;

    if (isExternalClock1)
    {
        Reset();
    }
}

void Clock::setExternalClock1(bool ext)
{
    isExternalClock1 = ext;
}

void Clock::setExternalClock2(bool ext)
{
    isExternalClock2 = ext;
}

void Clock::ExtPulse1()
{
    receivedExtPulse1 = true;
}

void Clock::ExtPulse2()
{
    receivedExtPulse2 = true;
}

bool Clock::ExtPulseReceived1()
{ // returns true if ext pulse 1 received, but only once

    bool temp = receivedExtPulse1;
    receivedExtPulse1 = false;
    return temp;
}

bool Clock::ExtPulseReceived2()
{ // returns true if ext pulse 2 received, but only once
    bool temp = receivedExtPulse2;
    receivedExtPulse2 = false;
    return temp;
}

void Clock::setBPM10(uint16_t bpm10)
{
    // bypass tap tempo, set BPM directly
    // BPM is always BPMx10 ie 120.0 bpm = 1200

    SetPhaseIncrementFromBPM10(bpm10);
}

uint16_t Clock::getBPM10()
{
    return GetBPM10FromPhaseIncrement();
}
