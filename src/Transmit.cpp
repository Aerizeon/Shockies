#include <Transmit.h>
#include <Arduino.h>

Transmitter::Transmitter(unsigned char transmitPin)
{
    TransmitPin = transmitPin;
    pinMode(transmitPin, OUTPUT);
}

void Transmitter::Transmit(Protocol protocol, unsigned long long data, unsigned char length, unsigned char repeat)
{
    for (unsigned char packetRepeat = 0; packetRepeat < repeat; packetRepeat++)
    {
        TransmitPulse(protocol.Sync);
        for (signed char bitOffset = length-1; bitOffset >= 0; bitOffset--)
        {
            if (data & (1LL << bitOffset))
                TransmitPulse(protocol.One);
            else
                TransmitPulse(protocol.Zero);
        }
    }
    delayMicroseconds(10);
}

inline void Transmitter::TransmitPulse(Pulse pulse)
{
    digitalWrite(TransmitPin, HIGH);
    delayMicroseconds(pulse.High);
    digitalWrite(TransmitPin, LOW);
    delayMicroseconds(pulse.Low);
}
