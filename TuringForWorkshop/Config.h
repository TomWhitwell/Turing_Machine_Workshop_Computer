#pragma once

#include <cstdint>
#include <cstddef>

class Config
{
public:
    static const uint32_t MAGIC;
    static const size_t FLASH_SIZE;
    static const size_t BLOCK_SIZE;
    static const size_t OFFSET;

    struct Preset
    {
        uint8_t scale;
        uint8_t notes;
        uint8_t range;
        uint8_t length;
        uint8_t looplen;
        uint8_t pulseMode1;
        uint8_t pulseMode2;
    };

    struct Data
    {
        uint32_t magic = MAGIC;

        uint8_t bpm_lo = 120;
        uint8_t bpm_hi = 0;
        uint8_t divide = 5;
        uint8_t cvRange = 0;

        Preset preset[2] = {
            {3, 0, 2, 5, 1, 0, 0}, // Preset 0
            {3, 1, 1, 5, 1, 0, 1}  // Preset 1
        };
    };

    void load(bool forceReset = false);
    void save();
    Data &get();

private:
    Data config;
};
