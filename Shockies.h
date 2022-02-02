#ifndef _Shockies_h
#define _Shockies_h
#include <WebSocketsServer.h>

/// Stops transmitting all commands, and locks device.
bool emergencyStop = false;

/// When the last ping was sent from a given controller. 
uint32_t lastWatchdogTime = 0;

/// TaskHandle for WebHandlerTask
TaskHandle_t webHandlerTask;

/// Temporary buffer for HTML that requires post-processing
static char htmlBuffer[4096];

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

/**
 * Stored EEPROM Settings
 */
struct EEPROM_Settings
{
  /// Determines which features are enabled
  CommandFlag Features = CommandFlag::None;
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
  /// Name of the Wi-Fi SSID to connect to on boot
  char WifiName[33];
  /// Password for the Wi-Fi network
  char WifiPassword[65];
  
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
} featureSettings;

/**
 * Command Attributes
 */
struct CommandState
{
  /// Current collar ID
  uint32_t CollarId = 0;
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
    CollarId = 0;
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
  void Set(uint32_t collarId, CommandFlag commandFlag, uint8_t intensity)
  {
    CollarId = collarId;
    Intensity  = min(intensity, commandFlag == CommandFlag::Shock ? featureSettings.ShockIntensity : featureSettings.VibrateIntensity);
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
#endif
