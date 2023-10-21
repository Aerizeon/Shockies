#ifndef _Shockies_h
#define _Shockies_h
#include <ESPAsyncWebServer.h>
#include <Devices.h>
#include <Transmit.h>
#include <mutex>
#include <DNSServer.h>

using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using std::mutex;
using std::lock_guard;

#define UUID_STR_LEN 37
#define SHOCKIES_SETTINGS_VERSION 8
#define SHOCKIES_VERSION "1.4.0"

typedef uint8_t uuid_t[16];

/// Reboot the ESP32
bool rebootDevice = false;

/// Software update is available
bool updateAvailable = false;

/// Stops transmitting all commands, and locks device.
bool emergencyStop = false;

/// When the last ping was sent from a given controller. 
uint32_t lastWatchdogTime = 0;

/// Last time an update check was performed
uint32_t lastUpdateCheck = 0;

mutex DevicesMutex;
vector<unique_ptr<Device>> Devices;
shared_ptr<Transmitter> DeviceTransmitter;
AsyncWebServer webServer(80);
AsyncWebSocket *webSocket;
AsyncWebSocket *webSocketId;
DNSServer dnsServer;

/**
 * Stored EEPROM Settings
 */
struct EEPROM_Settings
{
  uint16_t SettingsVersion;
  /// Name of the Wi-Fi SSID to connect to on boot
  char WifiName[33];
  /// Password for the Wi-Fi network
  char WifiPassword[65];
  /// Device UUID for websocket endpoint.
  char DeviceId[UUID_STR_LEN];
  /// Require DeviceID to be part of the local websocket URI (ws://shockies.local/<deviceID>)
  bool RequireDeviceId = false;
  /// Allow the device to be controlled from shockies.dev. This only works for me at the moment.
  bool AllowRemoteAccess = false;
  /// Allow up to 3 devices to be configured
  Settings Devices[3];
} EEPROMData;

/// Task loop for WebServer, WebSockets and DNS handlers.
void WebHandlerTask(void* parameter);

/// HTTP Handler for '/' 
void HTTP_GET_Index(AsyncWebServerRequest *request);

/// HTTP Handler for '/Update'
void HTTP_GET_Update(AsyncWebServerRequest *request);

/// HTTP Handler for '/Submit'
void HTTP_POST_Submit(AsyncWebServerRequest *request);

/// HTTP Handler for '/Update'
void HTTP_POST_Update(AsyncWebServerRequest *request);

/// File Handler for '/Update'
void HTTP_FILE_Update(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

/// Handler for WebSocket events.
void WS_HandleEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void WS_SendConfig();

void SR_HandleConnected();

void SR_HandleCommand(char* data, size_t len);

const char* HandleCommand(char* data);

void UpdateDevices();

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
