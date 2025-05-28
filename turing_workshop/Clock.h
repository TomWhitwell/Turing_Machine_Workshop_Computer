#pragma once
#include <stdint.h>

class Clock
{
public:
    void Tick();
    void Reset();
    void SetPhaseIncrement(uint32_t increment);
    uint32_t GetPhase() const;
    bool IsRisingEdge() const;

private:
    uint32_t phase = 0;
    uint32_t phase_increment = 0;
    bool rising_edge = false;
};
