#ifndef _Shockies_h
#define _Shockies_h
#include <WebSocketsServer.h>

#define UUID_STR_LEN 37
#define SHOCKIES_BUILD 4
typedef uint8_t uuid_t[16];

/// Stops transmitting all commands, and locks device.
bool emergencyStop = false;

/// When the last ping was sent from a given controller. 
uint32_t lastWatchdogTime = 0;

/// TaskHandle for WebHandlerTask
TaskHandle_t webHandlerTask;

/// Temporary buffer for HTML that requires post-processing
static char htmlBuffer[20000];

/// Lookup Table for Byte Reversal code
static unsigned char reverseLookup[16] = { 0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

/// BitFlags for commands that can be sent to the collar, or enabled as features.
enum class CommandFlag : uint8_t
{
  None    = 0b00000000,
  Shock   = 0b00000001,
  Vibrate = 0b00000010,
  Beep    = 0b00000100,
  Light   = 0b00001000,
  All     = 0b00001111,
  Invalid = 0b11111111
};

struct DeviceSettings
{
   /// Determines which features are enabled
  CommandFlag Features = CommandFlag::None;
  /// The ID for this collar
  uint16_t DeviceId = 65535;
  /// Determines max shock strength (0 - 100)
  uint8_t ShockIntensity = 0;
  /// Determines how long a single shock command can last
  uint8_t ShockDuration = 0;
  ///Determines how long to wait between shocks of maximum length
  uint8_t ShockInterval = 0;
  /// Determines max vibrate strength (0 - 100)
  uint8_t VibrateIntensity = 0;
  /// Determines how long a single vibrate command can last
  uint8_t VibrateDuration = 0;
  /// Device name for webpage display
  char Name[32];
    
  /**
   * Enable the specified feature(s)
   * 
   * @param feature Which features to enable
   */
  void EnableFeature(CommandFlag feature)
  {
    Features = static_cast<CommandFlag>(static_cast<uint8_t>(Features) | static_cast<uint8_t>(feature));
  }

  /**
   * Checks if a given feature is enabled
   * 
   * @param feature The feature to check against currently enabled features
   * @return 'True' if this feature is enabled, 'False' if not.
   */
  bool FeatureEnabled(CommandFlag feature)
  {
    return (static_cast<uint8_t>(Features) & static_cast<uint8_t>(feature)) == static_cast<uint8_t>(feature);
  }
};

/**
 * Stored EEPROM Settings
 */
struct EEPROM_Settings
{
  uint16_t CurrentBuild;
  /// Name of the Wi-Fi SSID to connect to on boot
  char WifiName[33];
  /// Password for the Wi-Fi network
  char WifiPassword[65];
  /// Device UUID for websocket endpoint.
  char DeviceID[UUID_STR_LEN];
  /// Require DeviceID to be part of the local websocket URI (ws://shockies.local/<deviceID>)
  bool RequireDeviceID = false;
  /// Allow the device to be controlled from shockies.dev (not yet implemented - TODO)
  bool AllowRemoteAccess = false;
  /// Allow up to 3 devices to be configured
  DeviceSettings Devices[3];
} settings;

/**
 * Command Attributes
 */
struct CommandState
{
  /// DeviceId
  uint32_t DeviceIndex = 0;
  /// Current command
  CommandFlag Command = CommandFlag::None;
  /// Current intensity
  uint8_t Intensity = 0;
  /// Time command was issued
  uint32_t StartTime = 0;
  uint32_t EndTime = 0;

  /// Reset all attributes on this CommandState
  void Reset()
  {
    DeviceIndex = 0;
    Command = CommandFlag::None;
    Intensity = 0;
    StartTime = 0;
    EndTime = 0;
  }

  /**
   * Set all attributes on this CommandState
   * 
   * @param collarId Target collar ID
   * @param commandFlag CommandFlag containing the desired command(s) 
   * @param intensity Strength of the Vibration or Shock command (0 - 100)
   */
  void Set(uint32_t deviceIndex, CommandFlag commandFlag, uint8_t intensity)
  {
    DeviceIndex = deviceIndex;
    Intensity  = min(intensity, commandFlag == CommandFlag::Shock ? settings.Devices[DeviceIndex].ShockIntensity : settings.Devices[DeviceIndex].VibrateIntensity);
    StartTime = millis();
    lastWatchdogTime = StartTime;
    Command = commandFlag;
  }
} currentCommand;

CommandState lastCommand;

/**
 * Pulse specification for individual OOK/ASK codes
 */
struct Pulse
{
  /// Length of high pulse, in PulseLength intervals
  uint8_t High;
  /// Length of low pulse, in PulseLength intervals
  uint8_t Low;
};

/**
 * OOK/ASK Protocol Specifications
 */
struct Protocol
{
  /// Length of pulse, in microseconds.
  uint16_t PulseLength;
  /// Pulse specifications for the Sync code
  Pulse Sync;
  /// Pulse specifications for the Zero code
  Pulse Zero;
  /// Pulse Specifications for the One code
  Pulse One;
} currentProtocol;



/**
 * Transmits an individual OOK pulse based on the specified protocol
 * 
 * @param pulse Pulse length specification for this pulse
 */
void TransmitPulse(Pulse pulse);

/**
 * Transmits a packet using 433.9Mhz ASK/OOK Transmitter connected to Pin 4
 *  
 * @param id Collar ID Code that will be transmitted
 * @param commandFlag Bitflags. Type of command (or commands) to be sent to the collar
 * @param strength Strength of Vibrate or Shock command (0 - 100)
 */
void SendPacket(uint16_t id, CommandFlag commandFlag, uint8_t strength);

/// Task loop for WebServer, WebSockets and DNS handlers.
void WebHandlerTask(void* parameter);

/// HTTP Handler for '/' 
void HTTP_GET_Index();

/// HTTP Handler for '/wificonfig'
void HTTP_GET_WifiConfig();

/// HTTP Handler for '/Control'
void HTTP_GET_Control();

/// HTTP Handler for '/Submit'
void HTTP_POST_Submit();

/// Handler for WebSocket events.
void WS_HandleEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

void WS_SendConfig();


/**
 * Reverse the bits in an unsigned byte
 * 
 * @param n Unsigned byte to be reversed.
 */
inline uint8_t reverse(uint8_t n) {
   // Reverse the top and bottom nibble then swap them.
   return (reverseLookup[n&0b1111] << 4) | reverseLookup[n>>4];
}

/**
 * Generate a UUID
 * From https://github.com/typester/esp32-uuid/
 */
inline void UUID_Generate(uuid_t uuid)
{
  esp_fill_random(uuid, sizeof(uuid_t));
  
  /* uuid version */
  uuid[6] = 0x40 | (uuid[6] & 0xF);
  
  /* uuid variant */
  uuid[8] = (0x80 | uuid[8]) & ~0x40;
}

inline void UUID_ToString(uuid_t uuid, char* string)
{
  snprintf(string, UUID_STR_LEN,
  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
  uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
  uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}
#endif
