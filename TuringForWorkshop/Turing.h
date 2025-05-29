#pragma once
#include <stdint.h>

class Turing

{

#define bitRotateL(value, len) ((((value) >> ((len) - 1)) & 0x01) | ((value) << 1))
#define bitRotateLflip(value, len) (((~((value) >> ((len) - 1)) & 0x01) | ((value) << 1)))

public:
    Turing(int length, uint32_t seed);
    void Update(int pot, int maxRange);
    void updateLength(int newLen);
    int returnLength();
    int DAC_16();
    int DAC_8();
    void DAC_print8();
    void DAC_print16();
    void randomSeed(uint32_t seed);
    int MidiNote(int low_note, int high_note, int scale_type, int sieve_type);

private:
    uint16_t _sequence = 0; // randomise on initialisation
    int _length = 16;
    inline static uint32_t _seed = 1;
    uint32_t next();
    uint32_t random(uint32_t max);
};
