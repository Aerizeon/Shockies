#include "Shockies.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <Update.h>
#include <esp32-hal.h>
#include <memory>

AsyncWebServer webServer(80);
AsyncWebSocket *webSocket;
AsyncWebSocket *webSocketId;
DNSServer dnsServer;

void setup()
{
  int wifiConnectTime = 0;
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("Device is booting...");

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  EEPROM.begin(sizeof(EEPROMData));
  EEPROM.get(0, EEPROMData);

  if (EEPROMData.SettingsVersion != SHOCKIES_SETTINGS_VERSION)
  {
    for (int dev = 0; dev < 3; dev++)
    {
      EEPROMData.Devices[dev].Type = Model::Petrainer;
      EEPROMData.Devices[dev].Features = Command::None;
      EEPROMData.Devices[dev].ShockIntensity = 30;
      EEPROMData.Devices[dev].ShockDuration = 5;
      EEPROMData.Devices[dev].ShockInterval = 5;
      EEPROMData.Devices[dev].VibrateIntensity = 50;
      EEPROMData.Devices[dev].VibrateDuration = 5;
      EEPROMData.Devices[dev].DeviceId = (uint16_t)(esp_random() % 65536);
    }

    memset(EEPROMData.WifiName, 0, 33);
    memset(EEPROMData.WifiPassword, 0, 65);
    memset(EEPROMData.DeviceId, 0, UUID_STR_LEN);
    EEPROMData.RequireDeviceId = false;
    EEPROMData.AllowRemoteAccess = false;
    EEPROMData.SettingsVersion = SHOCKIES_SETTINGS_VERSION;
    uuid_t deviceID;
    UUID_Generate(deviceID);
    UUID_ToString(deviceID, EEPROMData.DeviceId);
    EEPROM.put(0, EEPROMData);
    EEPROM.commit();
  }

  DeviceTransmitter = std::make_shared<Transmitter>();
  UpdateDevices();

  pinMode(4, OUTPUT);

  if (strlen(EEPROMData.WifiName) > 0)
  {
    Serial.println("Connecting to Wi-Fi...");
    Serial.printf("SSID: %s\n", EEPROMData.WifiName);
    Serial.printf("Password: %s\n", EEPROMData.WifiPassword);

    WiFi.mode(WIFI_STA);
    WiFi.begin(EEPROMData.WifiName, EEPROMData.WifiPassword);
    wifiConnectTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiConnectTime < 10000)
    {
      delay(1000);
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wi-Fi Connected!");
  }
  else
  {
    Serial.println("Failed to connect to Wi-Fi");
    Serial.println("Creating temporary Access Point for configuration...");

    WiFi.mode(WIFI_AP);

    if (WiFi.softAP("ShockiesConfig", "zappyzap"))
    {
      Serial.println("Access Point created!");
      Serial.println("SSID: ShockiesConfig");
      Serial.println("Pass: zappyzap");
    }
    else
    {
      Serial.println("Failed to create Acces Point");
    }

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
  }

  webSocket = new AsyncWebSocket("/websocket/");
  webSocketId = new AsyncWebSocket("/websocket/" + String(EEPROMData.DeviceId));
  webSocket->onEvent(WS_HandleEvent);
  webSocketId->onEvent(WS_HandleEvent);

  Serial.println("Starting HTTP Server on port 80...");

  // Main configuration page
  webServer.on("/", HTTP_GET, HTTP_GET_Index);
  // Captive Portal
  webServer.on("/fwlink", HTTP_GET, HTTP_GET_Index);
  webServer.on("/generate_204", HTTP_GET, HTTP_GET_Index);
  // Configuration changes
  webServer.on("/submit", HTTP_POST, HTTP_POST_Submit);
  // Updater
  webServer.on("/update", HTTP_GET, HTTP_GET_Update);
  webServer.on("/update", HTTP_POST, HTTP_POST_Update, HTTP_FILE_Update);
  // Global stylesheet
  webServer.serveStatic("/styles.css", SPIFFS, "/styles.css");

  webServer.begin();
  webServer.addHandler(webSocket);
  // webServer.addHandler(webSocketId);

  Serial.println("Starting mDNS...");

  bool mDNSFailed = false;

  if (!MDNS.begin("shockies"))
  {
    mDNSFailed = true;
    Serial.println("Error starting mDNS");
  }

  Serial.println("Connect to one of the following to configure settings:");
  if (!mDNSFailed)
    Serial.println("http://shockies.local");
  Serial.print("http://");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println(WiFi.localIP());
  else
    Serial.println(WiFi.softAPIP());
}

void loop()
{
  webSocket->cleanupClients();
  webSocketId->cleanupClients();
  dnsServer.processNextRequest();

  if (rebootDevice)
  {
    delay(100);
    ESP.restart();
  }

  // If E-Stop has been triggered
  if (emergencyStop)
    return;

  std::lock_guard<std::mutex> guard(DevicesMutex);
  for (auto &device : Devices)
  {
    device->TransmitCommand();
  }
}

String templateProcessor(const String &var)
{

  if (var == "DeviceId")
    return EEPROMData.RequireDeviceId ? String(EEPROMData.DeviceId) : "";
  else if (var == "RequireDeviceId")
    return EEPROMData.RequireDeviceId ? "checked" : "";
  else if (var == "AllowRemoteAccess")
    return EEPROMData.AllowRemoteAccess ? "checked" : "";
  else if (var == "WifiName")
    return EEPROMData.WifiName;
  else if (var == "WifiPassword")
    return EEPROMData.WifiPassword;
  else if (var == "VersionString")
    return SHOCKIES_VERSION;

  if (var.startsWith("Device"))
  {
    int splitIndex = var.indexOf('.');
    int deviceIndex = var.substring(6, splitIndex).toInt();
    String deviceVar = var.substring(splitIndex + 1);
    if (deviceVar == "DeviceType")
      return String(static_cast<uint8_t>(EEPROMData.Devices[deviceIndex].Type));
    else if (deviceVar == "DeviceId")
      return String(EEPROMData.Devices[deviceIndex].DeviceId);
    else if (deviceVar == "LightEnabled")
      return EEPROMData.Devices[deviceIndex].FeatureEnabled(Command::Light) ? "checked" : "";
    else if (deviceVar == "BeepEnabled")
      return EEPROMData.Devices[deviceIndex].FeatureEnabled(Command::Beep) ? "checked" : "";
    else if (deviceVar == "VibrateEnabled")
      return EEPROMData.Devices[deviceIndex].FeatureEnabled(Command::Vibrate) ? "checked" : "";
    else if (deviceVar == "ShockEnabled")
      return EEPROMData.Devices[deviceIndex].FeatureEnabled(Command::Shock) ? "checked" : "";
    else if (deviceVar == "ShockIntensity")
      return String(EEPROMData.Devices[deviceIndex].ShockIntensity);
    else if (deviceVar == "ShockDuration")
      return String(EEPROMData.Devices[deviceIndex].ShockDuration);
    else if (deviceVar == "ShockInterval")
      return String(EEPROMData.Devices[deviceIndex].ShockInterval);
    else if (deviceVar == "VibrateIntensity")
      return String(EEPROMData.Devices[deviceIndex].VibrateIntensity);
    else if (deviceVar == "VibrateDuration")
      return String(EEPROMData.Devices[deviceIndex].VibrateDuration);
    else
      return String();
  }
  else
    return String();
}

void HTTP_GET_Index(AsyncWebServerRequest *request)
{
  // If Wi-Fi is connected to an AP, send the default configuration page
  if (WiFi.status() == WL_CONNECTED)
  {
    request->send(SPIFFS, "/index.html", String(), false, templateProcessor);
  }
  // Otherwise we're in SoftAP mode
  else
  {
    if (request->host() == "shockies.local" || request->host() == String(WiFi.softAPIP()))
    {
      request->send(SPIFFS, "/setup.html", String(), false, templateProcessor);
    }
    else
    {
      request->redirect("http://" + String(WiFi.softAPIP()));
    }
  }
}
void HTTP_GET_Update(AsyncWebServerRequest *request)
{
  if (!request->authenticate("admin", EEPROMData.WifiPassword))
  {
    return request->requestAuthentication();
  }
  request->send(SPIFFS, "/update.html");
}

void HTTP_POST_Submit(AsyncWebServerRequest *request)
{
  if (request->hasParam("configure_features", true))
  {
    for (int devId = 0; devId < 2; devId++)
    {
      EEPROMData.Devices[devId].Features = Command::None;
      if (request->hasParam("feature_light" + String(devId), true))
        EEPROMData.Devices[devId].EnableFeature(Command::Light);
      if (request->hasParam("feature_beep" + String(devId), true))
        EEPROMData.Devices[devId].EnableFeature(Command::Beep);
      if (request->hasParam("feature_vibrate" + String(devId), true))
        EEPROMData.Devices[devId].EnableFeature(Command::Vibrate);
      if (request->hasParam("feature_shock" + String(devId), true))
        EEPROMData.Devices[devId].EnableFeature(Command::Shock);
      if (request->hasParam("device_id" + String(devId), true))
        EEPROMData.Devices[devId].DeviceId = request->getParam("device_id" + String(devId), true)->value().toInt();
      if (request->hasParam("device_type" + String(devId), true))
        EEPROMData.Devices[devId].Type = (Model)request->getParam("device_type" + String(devId), true)->value().toInt();
      if (request->hasParam("shock_max_intensity" + String(devId), true))
        EEPROMData.Devices[devId].ShockIntensity = request->getParam("shock_max_intensity" + String(devId), true)->value().toInt();
      if (request->hasParam("shock_max_duration" + String(devId), true))
        EEPROMData.Devices[devId].ShockDuration = request->getParam("shock_max_duration" + String(devId), true)->value().toInt();
      if (request->hasParam("shock_interval" + String(devId), true))
        EEPROMData.Devices[devId].ShockInterval = request->getParam("shock_interval" + String(devId), true)->value().toInt();
      if (request->hasParam("vibrate_max_intensity" + String(devId), true))
        EEPROMData.Devices[devId].VibrateIntensity = request->getParam("vibrate_max_intensity" + String(devId), true)->value().toInt();
      if (request->hasParam("vibrate_max_duration" + String(devId), true))
        EEPROMData.Devices[devId].VibrateDuration = request->getParam("vibrate_max_duration" + String(devId), true)->value().toInt();
    }
    EEPROMData.RequireDeviceId = request->hasParam("require_device_id", true);
    webSocket->enable(!EEPROMData.RequireDeviceId);
    if (EEPROMData.RequireDeviceId)
      webSocket->closeAll();
    EEPROMData.AllowRemoteAccess = request->hasParam("allow_remote_access", true);
    EEPROM.put(0, EEPROMData);
    EEPROM.commit();
    UpdateDevices();
    WS_SendConfig();
  }
  else if (request->hasParam("configure_wifi", true))
  {
    if (request->hasParam("wifi_ssid", true))
      request->getParam("wifi_ssid", true)->value().toCharArray(EEPROMData.WifiName, 33);
    if (request->hasParam("wifi_password", true))
      request->getParam("wifi_password", true)->value().toCharArray(EEPROMData.WifiPassword, 65);
    EEPROM.put(0, EEPROMData);
    EEPROM.commit();
    Serial.println("Wi-Fi configuration changed, rebooting...");
    rebootDevice = true;
  }
  request->redirect("http://shockies.local");
}

void HTTP_POST_Update(AsyncWebServerRequest *request)
{
  if (!request->authenticate("admin", EEPROMData.WifiPassword))
  {
    return request->requestAuthentication();
  }
  AsyncWebServerResponse *response = request->beginResponse(Update.hasError() ? 500 : 200, "text/plain", Update.hasError() ? "Update Failed. Rebooting." : "Update Succeeded. Rebooting");
  response->addHeader("Connection", "close");
  request->send(response);
  rebootDevice = true;
}

void HTTP_FILE_Update(AsyncWebServerRequest *request, String fileName, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!request->authenticate("admin", EEPROMData.WifiPassword))
  {
    return request->requestAuthentication();
  }

  if (index == 0)
  {
    Serial.println(fileName);
    if (fileName == "Shockies.ino.bin")
    {
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH))
      {
        return request->send(400, "text/plain", "Update Unable to start");
      }
    }
    else if (fileName == "Shockies.spiffs.bin")
    {
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS))
      {
        return request->send(400, "text/plain", "Update Unable to start");
      }
    }
  }

  if (len > 0)
  {
    if (!Update.write(data, len))
    {
      return request->send(400, "text/plain", "Update Unable to write");
    }
  }

  if (final)
  {
    if (!Update.end(true))
    {
      return request->send(400, "text/plain", "Update Unable to finish update");
    }
  }
}

void WS_HandleEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_DISCONNECT:
    Serial.printf("[%u] Disconnected from %s!\n", client->id(), server->url());
    break;
  case WS_EVT_CONNECT:
  {
    IPAddress ip = client->remoteIP();
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", client->id(), ip[0], ip[1], ip[2], ip[3], server->url());
    client->text("OK: CONNECTED");
    WS_SendConfig();
  }
  break;
  case WS_EVT_DATA:
  {
    std::lock_guard<std::mutex> guard(DevicesMutex);
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->opcode == WS_TEXT)
    {
      data[len] = '\0';
      Serial.printf("[%u] Message: %s\r\n", client->id(), data);
      if (emergencyStop)
      {
        Serial.printf("[%u] EMERGENCY STOP\nDevice will not accept commands until rebooted.\n", client->id());
        server->textAll("ERROR: EMERGENCY STOP");
        return;
      }

      char *command = strtok((char *)data, " ");
      char *id_str = strtok(0, " ");
      char *intensity_str = strtok(0, " ");

      if (command == 0)
      {
        Serial.printf("[%u] Text Error: Invalid Message\r\n", client->id());
        client->text("ERROR: INVALID FORMAT");
        break;
      }

      // Reset the current command status, and stop sending the command.
      if (*command == 'R')
      {
        for (auto &device : Devices)
        {
          device->SetCommand(Command::None);
        }
        client->text("OK: R");
        break;
      }
      // Triggers an emergency stop. This will require the ESP-32 to be rebooted.
      else if (*command == 'X')
      {
        emergencyStop = true;
        for (auto &device : Devices)
        {
          device->SetCommand(Command::None);
        }
        client->text("OK: EMERGENCY STOP");
        break;
      }
      // Ping to reset the lost connection timeout.
      else if (*command == 'P')
      {
        lastWatchdogTime = millis();
        break;
      }

      if (id_str == 0 || intensity_str == 0)
      {
        Serial.printf("[%u] Text Error: Invalid Message\n", client->id());
        client->text("ERROR: INVALID FORMAT");
        break;
      }

      uint16_t id = atoi(id_str);
      uint8_t intensity = atoi(intensity_str);

      if (id > 3)
        return;

      // Light
      if (*command == 'L' && EEPROMData.Devices[id].FeatureEnabled(Command::Light))
      {
        Devices[id]->SetCommand(Command::Light, intensity);
        client->text("OK: L");
        break;
      }
      // Beep
      else if (*command == 'B' && EEPROMData.Devices[id].FeatureEnabled(Command::Beep))
      {
        Devices[id]->SetCommand(Command::Beep, intensity);
        client->text("OK: B");
        break;
      }
      // Vibrate
      else if (*command == 'V' && EEPROMData.Devices[id].FeatureEnabled(Command::Vibrate))
      {
        Devices[id]->SetCommand(Command::Vibrate, intensity);
        client->text("OK: V");
        break;
      }
      // Shock
      else if (*command == 'S' && EEPROMData.Devices[id].FeatureEnabled(Command::Shock))
      {
        Devices[id]->SetCommand(Command::Shock, intensity);
        client->text("OK: S");
        break;
      }
      else
      {
        Serial.printf("[%u] Ignored Command %c\n", client->id(), *command);
        break;
      }
    }
  }
  break;
  }
}

void WS_SendConfig()
{
  char configBuffer[256];
  sprintf(configBuffer, "CONFIG:%04X%02X%02X%02X%02X%02X",
          0,
          EEPROMData.Devices[0].Features,
          EEPROMData.Devices[0].ShockIntensity,
          EEPROMData.Devices[0].ShockDuration,
          EEPROMData.Devices[0].VibrateIntensity,
          EEPROMData.Devices[0].VibrateDuration);
  webSocket->textAll(configBuffer);
  webSocketId->textAll(configBuffer);
}

void UpdateDevices()
{
  std::lock_guard<std::mutex> guard(DevicesMutex);
  Devices.clear();
  for (int i = 0; i < 3; i++)
  {
    switch (EEPROMData.Devices[i].Type)
    {
    case Model::Petrainer:
      Devices.push_back(std::unique_ptr<Petrainer>(new Petrainer(DeviceTransmitter, EEPROMData.Devices[i])));
      break;
    case Model::Funtrainer:
      Devices.push_back(std::unique_ptr<Funnipet>(new Funnipet(DeviceTransmitter, EEPROMData.Devices[i])));
      break;
    }
  }
}