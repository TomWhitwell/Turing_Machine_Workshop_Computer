#include "Clock.h"

void Clock::Tick() {
    uint32_t prev = phase;
    phase += phase_increment;
    rising_edge = (prev > phase); // detect wraparound
}

void Clock::Reset() {
    phase = 0;
    rising_edge = true;
}

void Clock::SetPhaseIncrement(uint32_t increment) {
    phase_increment = increment;
}

uint32_t Clock::GetPhase() const {
    return phase;
}

bool Clock::IsRisingEdge() const {
    return rising_edge;
}
