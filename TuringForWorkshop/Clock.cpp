#include "Clock.h"

void Clock::Tick()
{
    uint32_t prev = phase;
    uint32_t prev_mult = phase * subclockMultiplier;
    phase += phase_increment;
    rising_edge = (prev > phase); // detect wraparound

    // TEST_subclock_phase = phase * subclockMultiplier;
    // TEST_subclock_phase = (uint64_t)subclockCount * UINT32_MAX / subclockDividor;

    bool rising_edge_temp = (prev_mult > (phase * subclockMultiplier));

    if (rising_edge_temp)
    {
        subclockCount++; // update clock with each wraparound for multiplied clock
    }
    totalTicks++;

    if (!isExternalClock1)
    // Internal Clock
    {
        if (subclockCount > subclockDividor - 1) // subclockDividor is set by knob Y
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
        if (subclockCount > subclockDividor - 1) // subclockDividor is set by knob Y
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

void Clock::TapTempo(uint32_t tapTime)
{
    if (lastTapTime != 0)
    {
        uint32_t interval = tapTime - lastTapTime;

        if (interval > minInterval && interval < maxInterval)
        {
            SetPhaseIncrementFromTicks(interval);
        }
        else
        {
            lastTapTime = 0; // reset tap system
            return;
        }
    }
    lastTapTime = tapTime;
    Reset();
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