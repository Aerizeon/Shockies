#ifndef _Transmit_h
#define _Transmit_h
#include <Arduino.h>

static unsigned char reverseLookup[16] = {
    0x0,
    0x8,
    0x4,
    0xc,
    0x2,
    0xa,
    0x6,
    0xe,
    0x1,
    0x9,
    0x5,
    0xd,
    0x3,
    0xb,
    0x7,
    0xf,
};

/**
 * Pulse specification for individual OOK/ASK codes
 */
struct Pulse
{
    /// Length of high pulse, in microseconds
    unsigned short High;
    /// Length of low pulse, in microseconds
    unsigned short Low;
};

/**
 * OOK/ASK Protocol Specifications
 */
struct Protocol
{
    /// Pulse specifications for the Sync code
    Pulse Sync;
    /// Pulse specifications for the Zero code
    Pulse Zero;
    /// Pulse Specifications for the One code
    Pulse One;
};

class Transmitter
{
public:
    Transmitter(unsigned char transmitPin = 4);
    void Transmit(Protocol protocol, unsigned long long data, unsigned char length, unsigned char repeat = 2);
    ~Transmitter()
    {
        Serial.println("Transmitter Disposed");
    }

private:
    inline void TransmitPulse(Pulse pulse);
    unsigned char TransmitPin;
};

inline unsigned char reverse(unsigned char n)
{
    // Reverse the top and bottom nibble then swap them.
    return (reverseLookup[n & 0b1111] << 4) | reverseLookup[n >> 4];
}

#endif