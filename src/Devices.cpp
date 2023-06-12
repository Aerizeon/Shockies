#include <Devices.h>
#include <Arduino.h>

Device::Device(shared_ptr<Transmitter> transmitter, const Model deviceModel, const Settings deviceSettings, const Protocol deviceProtocol)
    : DeviceTransmitter(transmitter), DeviceModel(deviceModel), DeviceSettings(deviceSettings), DeviceProtocol(deviceProtocol)
{
    DeviceCommandStart = DeviceCommandEnd = millis();
}

void Device::SetCommand(Command targetCommand, unsigned char value)
{
    unsigned int currentTime = millis();
    int commandMaxInterval = static_cast<int>(DeviceSettings.ShockInterval);
    int commandMaxDuration = static_cast<int>(DeviceCommand == Command::Shock ? DeviceSettings.ShockDuration : DeviceSettings.VibrateDuration);
    int commandMaxValue = static_cast<int>(DeviceCommand == Command::Shock ? DeviceSettings.ShockIntensity : DeviceSettings.VibrateIntensity);

    if (targetCommand == Command::None)
    {
        DeviceHasCommand = false;
        DeviceCommandEnd = min(currentTime, DeviceCommandStart + commandMaxDuration);
    }
    else
    {
        DeviceHasCommand = true;
        DeviceCommand = targetCommand;
        DeviceCommandValue = min(static_cast<int>(value), commandMaxValue);

        // Check if we're trying to start a new command before the previous interval has expired
        if (currentTime - DeviceCommandEnd < max(1, commandMaxInterval) * 1000)
        {
            // If so, check if we have any duration leftover from the previous command.
            if ((DeviceCommandEnd - DeviceCommandStart) < commandMaxDuration * 1000)
            {
                // If there is time left, offset the start time by the amount of time used
                // so that we can use the remaining time (for quick bursts, for example)
                DeviceCommandStart = currentTime - (DeviceCommandEnd - DeviceCommandStart);
            }
            // If not, leave the start time where it was.
        }
        else
        {
            Serial.println("NewStart");
            DeviceCommandStart = currentTime;
        }
    }
}

void Device::ResetWatchdog(unsigned int currentTime)
{
    WatchdogTime = millis();
}

bool Device::CheckWatchdog(unsigned int currentTime)
{
    return currentTime - WatchdogTime < WatchdogTimeout;
}

bool Device::ShouldTransmit(unsigned int currentTime)
{
    if (DeviceCommand == Command::None || !DeviceHasCommand)
        return false;
    if (DeviceCommand == Command::Shock && (currentTime - DeviceCommandStart) > (DeviceSettings.ShockDuration * 1000))
        return false;
    if (DeviceCommand == Command::Vibrate && (currentTime - DeviceCommandStart) > (DeviceSettings.VibrateDuration * 1000))
        return false;
    return true;
}

Petrainer::Petrainer(shared_ptr<Transmitter> transmitter, Settings deviceSettings)
    : Device(transmitter, Model::Petrainer, deviceSettings, {{750, 625}, {250, 750}, {250, 1500}})
{
}

void Petrainer::TransmitCommand(unsigned int currentTime)
{
    // N = Channel ID (1 or 2)
    // C = Command
    // I = Collar ID
    // S = Strength
    // R = Reverse(Command | UFlags) ^ 0xFF

    // NNNNCCCC IIIIIIII IIIIIIII SSSSSSSS RRRRRRRR
    // 10000001 00000010 00000011 01100100 01111110 1S100
    // 10000010 00000010 00000011 01100100 10111110 1V100
    // 10000100 00000010 00000011 00000000 11011110 1B000
    // 10001000 00010010 10101111 00000000 11101110 1F000
    // 11111000 00010010 10101111 00000000 11100000 2F000
    // Channel 1 = 0b1000
    // Channel 2 = 0b1111

    /**
     * Channel ID is fairly meaningless, since all collars
     * can be reprogrammed to accept commands from either
     * channel, and any ID number - so we don't bother
     * providing a configuration option for it here.
     *
     * To reprogram a collar, long-press the power button
     * until it beeps, and then send a command with the
     * desired Channel ID and Collar ID
     */
    if (!ShouldTransmit(currentTime))
        return;

    unsigned long long data = 0LL;
    data |= (((unsigned long long)(MapCommand(DeviceCommand) | 0x80)) << 32);
    data |= (((unsigned long long)DeviceSettings.DeviceId) << 16);
    data |= (((unsigned long long)DeviceCommandValue) << 8);
    data |= reverse((MapCommand(DeviceCommand) | 0x80) ^ 0xFF);
    DeviceTransmitter->Transmit(DeviceProtocol, data, 40);
}

unsigned char Petrainer::MapCommand(Command targetCommand)
{
    switch (targetCommand)
    {
    case Command::Shock:
        return 0b00000001;
    case Command::Vibrate:
        return 0b00000010;
    case Command::Beep:
        return 0b00000100;
    case Command::Light:
        return 0b00001000;
    default:
        return 0;
    }
}

Funnipet::Funnipet(shared_ptr<Transmitter> transmitter, Settings deviceSettings)
    : Device(transmitter, Model::Funtrainer, deviceSettings, {{1375, 810}, {290, 800}, {775, 325}})
{
}

void Funnipet::TransmitCommand(unsigned int currentTime)
{
    if (!ShouldTransmit(currentTime))
        return;

    unsigned long long data = 0LL;
    data |= (((unsigned long long)DeviceSettings.DeviceId) << 24);
    data |= (((unsigned long long)MapCommand(DeviceCommand)) << 16);
    data |= (((unsigned long long)DeviceCommandValue) << 8);
    data |= (((data & 0xFF00) >> 8) + ((data & 0xFF0000) >> 16) + ((data & 0xFF000000) >> 24) + ((data & 0xFF00000000) >> 32)) % 256;
    data <<= 3; // Shift for padding.
    Serial.println(data);
    DeviceTransmitter->Transmit(DeviceProtocol, data, 43);
}

unsigned char Funnipet::MapCommand(Command targetCommand)
{
    switch (targetCommand)
    {
    case Command::Shock:
        return 0b00000001;
    case Command::Vibrate:
        return 0b00000010;
    case Command::Beep:
        return 0b00001000;
    case Command::Light:
        return 0b00000100;
    default:
        return 0;
    }
}
