#include "Shockies.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <Update.h>

AsyncWebServer webServer(80);
AsyncWebSocket *webSocket;
AsyncWebSocket *webSocketId;
DNSServer dnsServer;
 
void setup()
{
  int wifiConnectTime = 0;
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("Device is booting...");

  if(!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  EEPROM.begin(sizeof(settings));
  EEPROM.get(0, settings);

  if(settings.SettingsVersion != SHOCKIES_SETTINGS_VERSION){
    for(int dev = 0; dev < 3; dev++)
    {
      if(dev == 0)
      {
        settings.Devices[dev].Features = CommandFlag::All;
        settings.Devices[dev].ShockIntensity = 30;
        settings.Devices[dev].ShockDuration = 5;
        settings.Devices[dev].ShockInterval = 5;
        settings.Devices[dev].VibrateIntensity = 50;
        settings.Devices[dev].VibrateDuration = 5;
        settings.Devices[dev].DeviceId = (uint16_t)(esp_random() % 65536);
        sprintf(settings.Devices[dev].Name, "Device %u", dev + 1);
      }
      else
      {
        settings.Devices[dev].Features = CommandFlag::None;
        settings.Devices[dev].ShockIntensity = 1;
        settings.Devices[dev].ShockDuration = 1;
        settings.Devices[dev].ShockInterval = 1;
        settings.Devices[dev].VibrateIntensity = 1;
        settings.Devices[dev].VibrateDuration = 1;
        settings.Devices[dev].DeviceId = (uint16_t)(esp_random() % 65536);
        sprintf(settings.Devices[dev].Name, "Device %u", dev + 1);
      }
    }
    
    memset(settings.WifiName, 0, 33);
    memset(settings.WifiPassword, 0, 65);
    memset(settings.DeviceId, 0, UUID_STR_LEN);
    settings.RequireDeviceId = false;
    settings.AllowRemoteAccess = false;
    settings.SettingsVersion = SHOCKIES_SETTINGS_VERSION;
    uuid_t deviceID;
    UUID_Generate(deviceID);
    UUID_ToString(deviceID, settings.DeviceId);
    EEPROM.put(0, settings);
    EEPROM.commit();
  }

  pinMode(4, OUTPUT);
  
  if(strlen(settings.WifiName) > 0)
  {
    Serial.println("Connecting to Wi-Fi..."); 
    Serial.printf("SSID: %s\n", settings.WifiName);
    Serial.printf("Password: %s\n", settings.WifiPassword);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.WifiName, settings.WifiPassword);
    wifiConnectTime = millis();
    while(WiFi.status() != WL_CONNECTED && millis() - wifiConnectTime < 10000)
    {
      yield();
      delay(1000);
    }
  }
  
  if(WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wi-Fi Connected!");
  }
  else
  {
    Serial.println("Failed to connect to Wi-Fi");
    Serial.println("Creating temporary Access Point for configuration...");
    
    WiFi.mode(WIFI_AP);

    if(WiFi.softAP("ShockiesConfig", "zappyzap"))
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
  
  
  //currentProtocol = { 125, {  6, 5 }, {  2,  6 }, {  2,  12 }};
  currentProtocol = { 125, {  11, 6 }, {  2, 6 }, {  6,  2 }};

  webSocket = new AsyncWebSocket("/websocket/");
  webSocketId = new AsyncWebSocket("/websocket/" + String(settings.DeviceId));
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
  webServer.addHandler(webSocketId);
  
  Serial.println("Starting mDNS...");
  
  bool mDNSFailed = false;
  
  if(!MDNS.begin("shockies"))
  {
    mDNSFailed = true;
    Serial.println("Error starting mDNS");
  }

  Serial.println("Connect to one of the following to configure settings:");
  if(!mDNSFailed)
    Serial.println("http://shockies.local");
  Serial.print("http://");
  if(WiFi.status() == WL_CONNECTED)
    Serial.println(WiFi.localIP());
  else
    Serial.println(WiFi.softAPIP());
}

void loop()
{
  webSocket->cleanupClients();
  webSocketId->cleanupClients();
  dnsServer.processNextRequest();
  if(rebootDevice)
  {
    delay(100);
    ESP.restart();
  }

  uint32_t currentTime = millis();
  uint32_t commandTimeout = ((currentCommand.Command == CommandFlag::Shock) ? settings.Devices[currentCommand.DeviceIndex].ShockDuration : settings.Devices[currentCommand.DeviceIndex].VibrateDuration) * 1000;
  // If E-Stop has been triggered
  if(emergencyStop)
    return;
  // If we don't have a currently active command
  if(currentCommand.Command == CommandFlag::None)
    return;
  // If the in-game controller hasn't responded in 1.5 seconds
  if(currentTime - lastWatchdogTime > 1500)
    return;
  // If the current command has been executing for longer than the maximum duration
  if(currentTime - currentCommand.StartTime > commandTimeout)
    return;

  // Transmit the current command to the collar
  SendPacket(settings.Devices[currentCommand.DeviceIndex].DeviceId, currentCommand.Command, currentCommand.Intensity);

}

inline void TransmitPulse(Pulse pulse)
{
  digitalWrite(4, HIGH);
  delayMicroseconds(currentProtocol.PulseLength * pulse.High);
  digitalWrite(4, LOW);
  delayMicroseconds(currentProtocol.PulseLength * pulse.Low);
}

void SendPacket(uint16_t id, CommandFlag commandFlag, uint8_t strength)
{
  //N = Channel ID (1 or 2)
  //C = Command
  //I = Collar ID
  //S = Strength
  //R = Reverse(Command | UFlags) ^ 0xFF
  
  //NNNNCCCC IIIIIIII IIIIIIII SSSSSSSS RRRRRRRR
  //10000001 00000010 00000011 01100100 01111110 1S100
  //10000010 00000010 00000011 01100100 10111110 1V100
  //10000100 00000010 00000011 00000000 11011110 1B000
  //10001000 00010010 10101111 00000000 11101110 1F000
  //11111000 00010010 10101111 00000000 11100000
  
  //11111000 00010010 10101111 00000000 11100000 2F000
  //Channel 1 = 0b1000
  //Channel 2 = 0b1111

  /**
   * Channel ID is fairly meaningless, since all collars
   * can be reprogrammed to accept commands from either
   * channel, and any ID number - so we don't bother
   * providing a configuration option for it here.
   * 
   * To reprogram a PET998DR, long-press the power button
   * until it beeps, and then send a command with the
   * desired Channel ID and Collar ID
   */

  unsigned long long data = 0LL;
  data |= (((unsigned long long)id) << 24);
  data |= (((unsigned long long) (uint8_t)commandFlag) << 16);
  data |= (((unsigned long long) strength) << 8);
  data |= ((data & 0xFF00) >> 8) + ((data & 0xFF0000) >> 16) +((data & 0xFF000000) >> 24) + +((data & 0xFF00000000) >> 32);
  
  data <<= 3; //Shift for padding.
  //data |= (((unsigned long long) ((uint8_t)commandFlag | 0x80)) << 32);
  //data |= (((unsigned long long) id) << 16);
  //data |= (((unsigned long long) strength) << 8);
  //data |= reverse(((uint8_t)commandFlag | 0x80) ^ 0xFF);
  
  for (uint8_t repeat = 0; repeat < 2; repeat++)
  {
    TransmitPulse(currentProtocol.Sync);
    for (int8_t i = 42; i >= 0; i--)
    {
      if (data & (1LL << i))
        TransmitPulse(currentProtocol.One);
      else
        TransmitPulse(currentProtocol.Zero);
    }
  }
}

String templateProcessor(const String& var)
{
  if(var == "DeviceId")
    return settings.RequireDeviceId ? String(settings.DeviceId) : "";
  else if(var == "RequireDeviceId")
    return settings.RequireDeviceId ? "checked" : "";
  else if(var == "AllowRemoteAccess")
    return settings.AllowRemoteAccess ? "checked" : "";
  else if(var == "WifiName")
    return settings.WifiName;
  else if(var == "WifiPassword")
    return settings.WifiPassword;
  else if(var == "VersionString")
    return SHOCKIES_VERSION;

  // Device 0
  else if(var == "Device0.DeviceId")
    return String(settings.Devices[0].DeviceId);
  else if(var == "Device0.BeepEnabled")
    return settings.Devices[0].FeatureEnabled(CommandFlag::Beep) ? "checked" : "";
  else if(var == "Device0.VibrateEnabled")
    return settings.Devices[0].FeatureEnabled(CommandFlag::Vibrate) ? "checked" : "";
  else if(var == "Device0.ShockEnabled")
    return settings.Devices[0].FeatureEnabled(CommandFlag::Shock) ? "checked" : "";
  else if(var == "Device0.ShockIntensity")
    return String(settings.Devices[0].ShockIntensity);
  else if(var == "Device0.ShockDuration")
    return String(settings.Devices[0].ShockDuration);
  else if(var == "Device0.ShockInterval")
  return String(settings.Devices[0].ShockInterval);
  else if(var == "Device0.VibrateIntensity")
    return String(settings.Devices[0].VibrateIntensity);
  else if(var == "Device0.VibrateDuration")
    return String(settings.Devices[0].VibrateDuration);
  
  //Device 1
  else if(var == "Device1.DeviceId")
    return String(settings.Devices[1].DeviceId);
  else if(var == "Device1.BeepEnabled")
    return settings.Devices[1].FeatureEnabled(CommandFlag::Beep) ? "checked" : "";
  else if(var == "Device1.VibrateEnabled")
    return settings.Devices[1].FeatureEnabled(CommandFlag::Vibrate) ? "checked" : "";
  else if(var == "Device1.ShockEnabled")
    return settings.Devices[1].FeatureEnabled(CommandFlag::Shock) ? "checked" : "";
  else if(var == "Device1.ShockIntensity")
    return String(settings.Devices[1].ShockIntensity);
  else if(var == "Device1.ShockDuration")
    return String(settings.Devices[1].ShockDuration);
  else if(var == "Device1.ShockInterval")
  return String(settings.Devices[1].ShockInterval);
  else if(var == "Device1.VibrateIntensity")
    return String(settings.Devices[1].VibrateIntensity);
  else if(var == "Device1.VibrateDuration")
    return String(settings.Devices[1].VibrateDuration);

  //Device 2
  else if(var == "Device2.DeviceId")
    return String(settings.Devices[2].DeviceId);
  else if(var == "Device2.BeepEnabled")
    return settings.Devices[2].FeatureEnabled(CommandFlag::Beep) ? "checked" : "";
  else if(var == "Device2.VibrateEnabled")
    return settings.Devices[2].FeatureEnabled(CommandFlag::Vibrate) ? "checked" : "";
  else if(var == "Device2.ShockEnabled")
    return settings.Devices[2].FeatureEnabled(CommandFlag::Shock) ? "checked" : "";
  else if(var == "Device2.ShockIntensity")
    return String(settings.Devices[2].ShockIntensity);
  else if(var == "Device2.ShockDuration")
    return String(settings.Devices[2].ShockDuration);
  else if(var == "Device2.ShockInterval")
  return String(settings.Devices[2].ShockInterval);
  else if(var == "Device2.VibrateIntensity")
    return String(settings.Devices[2].VibrateIntensity);
  else if(var == "Device2.VibrateDuration")
    return String(settings.Devices[2].VibrateDuration);
  else
  return String();
}

void HTTP_GET_Index(AsyncWebServerRequest *request)
{
  // If Wi-Fi is connected to an AP, send the default configuration page
  if(WiFi.status() == WL_CONNECTED)
  {
    request->send(SPIFFS, "/index.html", String(),false, templateProcessor);
  }
  //Otherwise we're in SoftAP mode
  else
  {
    if(request->host() == "shockies.local" || request->host() == String(WiFi.softAPIP()))
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
  if(!request->authenticate("admin", settings.WifiPassword))
  {
    return request->requestAuthentication();
  }
  request->send(SPIFFS, "/update.html");
}

void HTTP_POST_Submit(AsyncWebServerRequest *request)
{
  if(request->hasParam("configure_features", true))
  {
    for(int devId = 0; devId < 2; devId++)
    {
      settings.Devices[devId].Features = CommandFlag::None;
      if(request->hasParam("feature_beep" + String(devId), true))
        settings.Devices[devId].EnableFeature(CommandFlag::Beep);
      if(request->hasParam("feature_vibrate" + String(devId), true))
        settings.Devices[devId].EnableFeature(CommandFlag::Vibrate);
      if(request->hasParam("feature_shock" + String(devId), true))
        settings.Devices[devId].EnableFeature(CommandFlag::Shock);
      if(request->hasParam("device_id" + String(devId), true))
        settings.Devices[devId].DeviceId = request->getParam("device_id" + String(devId), true)->value().toInt();
      if(request->hasParam("shock_max_intensity" + String(devId), true))
        settings.Devices[devId].ShockIntensity = request->getParam("shock_max_intensity" + String(devId), true)->value().toInt();
      if(request->hasParam("shock_max_duration" + String(devId), true))
        settings.Devices[devId].ShockDuration = request->getParam("shock_max_duration" + String(devId), true)->value().toInt();
      if(request->hasParam("shock_interval" + String(devId), true))
        settings.Devices[devId].ShockInterval = request->getParam("shock_interval" + String(devId), true)->value().toInt();
      if(request->hasParam("vibrate_max_intensity" + String(devId), true))
        settings.Devices[devId].VibrateIntensity = request->getParam("vibrate_max_intensity" + String(devId), true)->value().toInt();
      if(request->hasParam("vibrate_max_duration" + String(devId), true))
        settings.Devices[devId].VibrateDuration = request->getParam("vibrate_max_duration" + String(devId), true)->value().toInt();
    }
    settings.RequireDeviceId = request->hasParam("require_device_id", true);
    webSocket->enable(!settings.RequireDeviceId);
    if(settings.RequireDeviceId)
      webSocket->closeAll();    
    settings.AllowRemoteAccess = request->hasParam("allow_remote_access", true);
    EEPROM.put(0, settings);
    EEPROM.commit();
    WS_SendConfig();
  }
  else if(request->hasParam("configure_wifi", true))
  {
    if(request->hasParam("wifi_ssid", true))
      request->getParam("wifi_ssid", true)->value().toCharArray(settings.WifiName, 33);
    if(request->hasParam("wifi_password", true))
      request->getParam("wifi_password", true)->value().toCharArray(settings.WifiPassword, 65);
    EEPROM.put(0, settings);
    EEPROM.commit();
    Serial.println("Wi-Fi configuration changed, rebooting...");
    rebootDevice = true;
  }
  request->redirect("http://shockies.local");
}

void HTTP_POST_Update(AsyncWebServerRequest *request)
{
  if(!request->authenticate("admin", settings.WifiPassword))
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
  if(!request->authenticate("admin", settings.WifiPassword))
  {
    return request->requestAuthentication();
  }

  if(index == 0)
  {
    Serial.println(fileName);
    if(fileName == "Shockies.ino.bin")
    {
      if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH))
      {
        return request->send(400, "text/plain", "Update Unable to start");
      }
    }
    else if(fileName == "Shockies.spiffs.bin")
    {
      if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS))
      {
        return request->send(400, "text/plain", "Update Unable to start");
      }
    }
  }

  if(len > 0)
  {
      if(!Update.write(data, len))
      {
        return request->send(400, "text/plain", "Update Unable to write");
      }    
  }

  if(final)
  {
    if(!Update.end(true))
    {
      return request->send(400, "text/plain", "Update Unable to finish update");
    }       
  }
}

void WS_HandleEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  switch(type) {
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
            AwsFrameInfo * info = (AwsFrameInfo*)arg;
            if(info->final && info->opcode == WS_TEXT)
            {
              data[len] = '\0';
              Serial.printf("[%u] Message: %s\r\n", client->id(), data);
              if(emergencyStop)
              {
                Serial.printf("[%u] EMERGENCY STOP\nDevice will not accept commands until rebooted.\n", client->id());
                server->textAll("ERROR: EMERGENCY STOP");
                return;        
              }

              char* command = strtok((char*)data, " ");
              char* id_str = strtok(0, " ");
              char* intensity_str = strtok(0, " ");

              if(command == 0)
              {
                Serial.printf("[%u] Text Error: Invalid Message\r\n", client->id());
                client->text("ERROR: INVALID FORMAT");
                break;
              }

              //Reset the current command status, and stop sending the command.
              if(*command == 'R')
              {
                uint32_t commandTimeout = ((currentCommand.Command == CommandFlag::Shock) ? settings.Devices[currentCommand.DeviceIndex].ShockDuration : settings.Devices[currentCommand.DeviceIndex].VibrateDuration) * 1000;
                lastCommand = currentCommand;
                lastCommand.EndTime = min((uint32_t)millis(), lastCommand.StartTime + commandTimeout);
                currentCommand.Reset();
                client->text("OK: R");
                break;
              }
              //Triggers an emergency stop. This will require the ESP-32 to be rebooted.
              else if(*command == 'X')
              {
                emergencyStop = true;
                currentCommand.Reset();
                client->text("OK: EMERGENCY STOP");
                break;
              }
              //Ping to reset the lost connection timeout.
              else if(*command == 'P')
              {
                lastWatchdogTime = millis();
                break;
              }

              if(id_str == 0 || intensity_str == 0)
              {
                Serial.printf("[%u] Text Error: Invalid Message\n", client->id());
                client->text("ERROR: INVALID FORMAT");
                break;
              }

              uint16_t id = atoi(id_str);
              uint8_t intensity = atoi(intensity_str);

              if(id > 3)
                return;

              //Beep - Intensity is not used.
              if(*command == 'B' && settings.Devices[id].FeatureEnabled(CommandFlag::Beep))
              {
                currentCommand.Set(id, CommandFlag::Beep, intensity);
                client->text("OK: B");
                break;
              }
              //Vibrate
              else if(*command == 'V' && settings.Devices[id].FeatureEnabled(CommandFlag::Vibrate))
              {
                currentCommand.Set(id, CommandFlag::Vibrate, intensity);
                client->text("OK: V");
                break;
              }
              //Shock
              else if(*command == 'S' && settings.Devices[id].FeatureEnabled(CommandFlag::Shock))
              {
                if(millis() - lastCommand.EndTime < max(1, (int)settings.Devices[id].ShockInterval) * 1000)
                {
                  if((lastCommand.EndTime - lastCommand.StartTime) < settings.Devices[id].ShockDuration * 1000)
                  {
                    currentCommand.StartTime = millis() - (lastCommand.EndTime - lastCommand.StartTime);
                  }
                  else
                  {
                    currentCommand.StartTime = lastCommand.StartTime;
                  }
                  currentCommand.DeviceIndex = id;
                  currentCommand.Intensity = intensity;
                  currentCommand.Command = CommandFlag::Shock;
                }
                else
                {
                  currentCommand.Set(id, CommandFlag::Shock, intensity);
                }
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
  settings.Devices[0].Features,
  settings.Devices[0].ShockIntensity,
  settings.Devices[0].ShockDuration,
  settings.Devices[0].VibrateIntensity,
  settings.Devices[0].VibrateDuration);
  webSocket->textAll(configBuffer);
  webSocketId->textAll(configBuffer);
}
