#ifndef _Devices_h
#define _Devices_h

#include <Transmit.h>
#include <memory>
#include <string>
#include <Arduino.h>

using std::shared_ptr;

enum class Command : unsigned char
{
    None = 0b00000000,
    Shock = 0b00000001,
    Vibrate = 0b00000010,
    Beep = 0b00000100,
    Light = 0b00001000,
    All = 0b00001111,
    Invalid = 0b11111111
};

enum class Model : unsigned char
{
    Petrainer = 0, // The Petrainer 998DR. This was commonly used with first generation pishock devices.
    Funtrainer = 1 // FunniPets FunTrainer (Patent zl201730008120.0). This can also be found on aliexpress as a generic. Commonly used with second generation pishock devices.
};

struct Settings
{
    Model Type = Model::Petrainer;
    /// Determines which features are enabled
    Command Features = Command::None;
    /// The ID for this collar
    unsigned short DeviceId = 65535;
    /// Determines max shock strength (0 - 100)
    unsigned char ShockIntensity = 0;
    /// Determines how long a single shock command can last
    unsigned char ShockDuration = 0;
    /// Determines how long to wait between shocks of maximum length
    unsigned char ShockInterval = 0;
    /// Determines max vibrate strength (0 - 100)
    unsigned char VibrateIntensity = 0;
    /// Determines how long a single vibrate command can last
    unsigned char VibrateDuration = 0;
    /**
     * Enable the specified feature(s)
     *
     * @param feature Which features to enable
     */
    void EnableFeature(Command feature)
    {
        Features = static_cast<Command>(static_cast<unsigned char>(Features) | static_cast<unsigned char>(feature));
    }

    /**
     * Checks if a given feature is enabled
     *
     * @param feature The feature to check against currently enabled features
     * @return 'True' if this feature is enabled, 'False' if not.
     */
    bool FeatureEnabled(Command feature)
    {
        return (static_cast<unsigned char>(Features) & static_cast<unsigned char>(feature)) == static_cast<unsigned char>(feature);
    }
};

class Device
{
public:
    static constexpr unsigned int WatchdogTimeout = 1500;
    const Model DeviceModel;
    const Protocol DeviceProtocol;
    const Settings DeviceSettings;

    Device(shared_ptr<Transmitter> transmitter, const Model deviceModel, const Settings deviceSettings, const Protocol deviceProtocol);
    void SetCommand(Command targetCommand = Command::None, unsigned char value = 0);
    void ResetWatchdog(unsigned int currentTime);
    bool CheckWatchdog(unsigned int currentTime);
    bool ShouldTransmit(unsigned int currentTime);
    virtual void TransmitCommand(unsigned int currentTime) = 0;
    virtual unsigned char MapCommand(Command targetCommand) = 0;
    ~Device()
    {
        Serial.println("Device disposed");
    }

protected:
    const shared_ptr<Transmitter> DeviceTransmitter;
    bool DeviceHasCommand = false;
    Command DeviceCommand;
    unsigned char DeviceCommandValue;
    unsigned int DeviceCommandStart;
    unsigned int DeviceCommandEnd;
    unsigned int WatchdogTime;
};

class Petrainer : public Device
{
public:
    Petrainer(shared_ptr<Transmitter> transmitter, Settings deviceSettings);
    void TransmitCommand(unsigned int currentTime) override;
    unsigned char MapCommand(Command targetCommand) override;
    ~Petrainer()
    {
        Serial.println("Petrainer disposed");
    }
};

class Funnipet : public Device
{
public:
    Funnipet(shared_ptr<Transmitter> transmitter, Settings deviceSettings);
    void TransmitCommand(unsigned int currentTime) override;
    unsigned char MapCommand(Command targetCommand) override;
    ~Funnipet()
    {
        Serial.println("Funtrainer disposed");
    }
};
#endif