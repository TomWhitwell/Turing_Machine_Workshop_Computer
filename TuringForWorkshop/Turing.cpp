#include "Turing.h"

Turing::Turing(int length, uint32_t seed)
{
    _length = length;
    randomSeed(seed);
    _sequence = next() & 0xFFFF;
}

// Call this each time the clock 'ticks'
// Pick a random number 0 to maxRange
// if the number is below the pot reading: bitRotateLflip, otherwise bitRotateL
// set _outputValue as
void Turing::Update(int pot, int maxRange)
{
    int safeZone = maxRange >> 5;
    int sample = safeZone + random(maxRange - (safeZone * 2)); // add safe zones at top and bottom

    if (sample >= pot)
    {
        _sequence = bitRotateLflip(_sequence, _length);
    }

    else
    {
        _sequence = bitRotateL(_sequence, _length);
    }
}

// returns the full current sequence value as 16 bit number 0 to 65535
uint16_t Turing::DAC_16()
{
    return _sequence;
}

// returns the current sequence value as 8 bit number 0 to 255 = ignores the last 8 binary digits
uint8_t Turing::DAC_8()
{

    //  return _sequence >> 8 ; // left hand 8 bits
    return _sequence & 0xFF; // right hand 8 bits
}

void Turing::updateLength(int newLen)
{
    _length = newLen;
}

int Turing::returnLength()
{
    return _length;
}

uint32_t Turing::next()
{
    constexpr uint32_t a = 1103515245u;
    constexpr uint32_t c = 12345u;
    _seed = a * _seed + c;
    return static_cast<uint32_t>(_seed >> 1); // 31-bit positive
}

void Turing::randomSeed(uint32_t seed)
{
    if (seed != 0)
        _seed = seed; // ignore zero (matches Arduino)
}

uint32_t Turing::random(uint32_t max) // [0, max)
{
    if (max <= 0)
        return 0;
    return next() % max;
}

int Turing::MidiNote(int low_note, int high_note, int scale_type, int sieve_type)
{
    const int MAX_NOTES = 128;

    // Scale intervals in semitones
    const int chromatic[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    const int major[] = {0, 2, 4, 5, 7, 9, 11};
    const int minor[] = {0, 2, 3, 5, 7, 8, 10};
    const int minor_pent[] = {0, 3, 5, 7, 10};
    const int dorian[] = {0, 2, 3, 5, 7, 9, 10};
    const int pelog[] = {0, 1, 3, 7, 10};
    const int wholetone[] = {0, 2, 4, 6, 8, 10};

    const int *scales[] = {chromatic, major, minor, minor_pent, dorian, pelog, wholetone};
    const int scale_sizes[] = {
        sizeof(chromatic) / sizeof(int),
        sizeof(major) / sizeof(int),
        sizeof(minor) / sizeof(int),
        sizeof(minor_pent) / sizeof(int),
        sizeof(dorian) / sizeof(int),
        sizeof(pelog) / sizeof(int),
        sizeof(wholetone) / sizeof(int)};

    // Degree sieves
    const int degrees_scale[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    const int degrees_1_3_5[] = {1, 3, 5};
    const int degrees_1_5[] = {1, 5};
    const int degrees_1_3_5_6[] = {1, 3, 5, 6};
    const int degrees_1_3_5_7[] = {1, 3, 5, 7};

    const int *sieves[] = {degrees_scale, degrees_1_3_5, degrees_1_5, degrees_1_3_5_6, degrees_1_3_5_7};
    const int sieve_sizes[] = {
        sizeof(degrees_scale) / sizeof(int),
        sizeof(degrees_1_3_5) / sizeof(int),
        sizeof(degrees_1_5) / sizeof(int),
        sizeof(degrees_1_3_5_6) / sizeof(int),
        sizeof(degrees_1_3_5_7) / sizeof(int)};

    // Bounds checking
    if (scale_type < 0 || scale_type >= 7)
        scale_type = 0;
    if (sieve_type < 0 || sieve_type >= 5)
        sieve_type = 0;

    const int *scale = scales[scale_type];
    int scale_size = scale_sizes[scale_type];

    const int *sieve = sieves[sieve_type];
    int sieve_size = sieve_sizes[sieve_type];

    // Filter scale by sieve
    int sieved_intervals[12];
    int sieved_size = 0;
    for (int i = 0; i < sieve_size; ++i)
    {
        int degree = sieve[i];
        if (degree >= 1 && degree <= scale_size)
        {
            sieved_intervals[sieved_size++] = scale[degree - 1];
        }
    }

    // Build list of valid MIDI notes in the range
    int possible_notes[MAX_NOTES];
    int num_notes = 0;

    for (int note = low_note; note <= high_note; ++note)
    {
        int note_in_octave = note % 12;
        for (int i = 0; i < sieved_size; ++i)
        {
            if (note_in_octave == sieved_intervals[i])
            {
                possible_notes[num_notes++] = note;
                break;
            }
        }
    }

    if (num_notes == 0)
        return -1;

    // Map 8-bit sequence value to index
    uint8_t rand_val = DAC_8();
    int index = (rand_val * num_notes) / 256;
    if (index >= num_notes)
        index = num_notes - 1;

    return possible_notes[index];
}
