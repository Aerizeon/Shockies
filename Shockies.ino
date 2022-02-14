#include "Shockies.h"
#include "HTMLContent.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <EEPROM.h>

WebSocketsServer webSocket(81);
WebServer webServer(80);
DNSServer dnsServer;
 
void setup()
{
  int wifiConnectTime = 0;
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("Device is booting...");
  EEPROM.begin(sizeof(settings));
  EEPROM.get(0, settings);
  if(settings.CurrentBuild != SHOCKIES_BUILD){
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
    memset(settings.DeviceID, 0, UUID_STR_LEN);
    settings.RequireDeviceID = false;
    settings.AllowRemoteAccess = false;
    settings.CurrentBuild = SHOCKIES_BUILD;
    uuid_t deviceID;
    UUID_Generate(deviceID);
    UUID_ToString(deviceID, settings.DeviceID);
    EEPROM.put(0, settings);
    EEPROM.commit();
  }

  pinMode(4, OUTPUT);
  
  if(strlen(settings.WifiName) > 0)
  {
    Serial.println("Connecting to Wi-Fi..."); 
    Serial.printf("SSID: %s\n", settings.WifiName);
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.WifiName, settings.WifiPassword);
    wifiConnectTime = millis();
    while(WiFi.status() != WL_CONNECTED && millis() - wifiConnectTime < 10000)
    {
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
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
  }
  
  
  currentProtocol = { 125, {  6, 5 }, {  2,  6 }, {  2,  12 }};
  xTaskCreatePinnedToCore(WebHandlerTask, "Web Handler Task", 10000, NULL, 1, &webHandlerTask, 0);
}

void loop()
{
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

void WebHandlerTask(void* parameter)
{
  bool mDNSFailed = false;

  Serial.println("Starting mDNS...");
  
  if(!MDNS.begin("shockies"))
  {
    mDNSFailed = true;
    Serial.println("Error starting mDNS");
  }
  
  Serial.println("Starting HTTP Server on port 80...");
  webServer.begin();
  webServer.on("/", HTTP_GET, HTTP_GET_Index);
  webServer.on("/fwlink", HTTP_GET, HTTP_GET_Index);
  webServer.on("/generate_204", HTTP_GET, HTTP_GET_Index);
  webServer.on("/wificonfig", HTTP_GET, HTTP_GET_WifiConfig);
  webServer.on("/submit", HTTP_POST, HTTP_POST_Submit);
  Serial.println("Starting WebSocket Server on port 81...");
  webSocket.begin();
  webSocket.onEvent(WS_HandleEvent);
  
  Serial.println("Connect to one of the following to configure settings:");
  if(!mDNSFailed)
    Serial.println("http://shockies.local");
  Serial.print("http://");
  if(WiFi.status() == WL_CONNECTED)
    Serial.println(WiFi.localIP());
  else
    Serial.println(WiFi.softAPIP());

  while(true)
  {
    webServer.handleClient();
    webSocket.loop();
    dnsServer.processNextRequest();
  }
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
  data |= (((unsigned long long) ((uint8_t)commandFlag | 0x80)) << 32);
  data |= (((unsigned long long) id) << 16);
  data |= (((unsigned long long) strength) << 8);
  data |= reverse(((uint8_t)commandFlag | 0x80) ^ 0xFF);
  
  for (uint8_t repeat = 0; repeat < 2; repeat++)
  {
    TransmitPulse(currentProtocol.Sync);
    for (int8_t i = 39; i >= 0; i--)
    {
      if (data & (1LL << i))
        TransmitPulse(currentProtocol.One);
      else
        TransmitPulse(currentProtocol.Zero);
    }
  }
}

void HTTP_GET_Index()
{
  if (webServer.hostHeader() != "shockies.local" && webServer.hostHeader() != WiFi.softAPIP().toString() && webServer.hostHeader() != WiFi.localIP().toString())
  {
    webServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString());
    webServer.sendHeader("Cache-Control", "no-cache");
    webServer.send(301);
    return;
  }

  // If Wi-Fi is connected to an AP, send the default configuration page
  if(WiFi.status() == WL_CONNECTED)
  {
    sprintf(htmlBuffer,
    HTML_IndexDefault,
    settings.DeviceID,
    
    settings.Devices[0].DeviceId,
    settings.Devices[0].FeatureEnabled(CommandFlag::Beep) ? "checked" : "",
    settings.Devices[0].FeatureEnabled(CommandFlag::Vibrate) ? "checked" : "",
    settings.Devices[0].FeatureEnabled(CommandFlag::Shock) ? "checked" : "",
    settings.Devices[0].ShockIntensity,
    settings.Devices[0].ShockDuration,
    settings.Devices[0].ShockInterval,
    settings.Devices[0].VibrateIntensity,
    settings.Devices[0].VibrateDuration,

    settings.Devices[1].DeviceId,
    settings.Devices[1].FeatureEnabled(CommandFlag::Beep) ? "checked" : "",
    settings.Devices[1].FeatureEnabled(CommandFlag::Vibrate) ? "checked" : "",
    settings.Devices[1].FeatureEnabled(CommandFlag::Shock) ? "checked" : "",
    settings.Devices[1].ShockIntensity,
    settings.Devices[1].ShockDuration,
    settings.Devices[1].ShockInterval,
    settings.Devices[1].VibrateIntensity,
    settings.Devices[1].VibrateDuration,

    settings.Devices[2].DeviceId,
    settings.Devices[2].FeatureEnabled(CommandFlag::Beep) ? "checked" : "",
    settings.Devices[2].FeatureEnabled(CommandFlag::Vibrate) ? "checked" : "",
    settings.Devices[2].FeatureEnabled(CommandFlag::Shock) ? "checked" : "",
    settings.Devices[2].ShockIntensity,
    settings.Devices[2].ShockDuration,
    settings.Devices[2].ShockInterval,
    settings.Devices[2].VibrateIntensity,
    settings.Devices[2].VibrateDuration,
    
    settings.RequireDeviceID ? "checked" : "",
    settings.AllowRemoteAccess ? "checked" : "",
    settings.RequireDeviceID ? settings.DeviceID : "");
    webServer.send(200, "text/html", htmlBuffer); 
  }
  // Otherwise, assume we're in SoftAP mode, and send the Wi-Fi configuration page
  else
  {
    sprintf(htmlBuffer,
    HTML_IndexConfigureSSID,
    settings.WifiName,
    settings.WifiPassword);
    webServer.send(200, "text/html", htmlBuffer); 
  }
}

void HTTP_GET_WifiConfig()
{
  sprintf(htmlBuffer,
    HTML_IndexConfigureSSID,
    settings.WifiName,
    settings.WifiPassword);
    webServer.send(200, "text/html", htmlBuffer); 
}

void HTTP_POST_Submit()
{
  int wifiConnectTime = 0;
  if(webServer.hasArg("configure_features"))
  {
    // Yes this is ugly, but there's only support for 3 devices at the moment,
    // and for some reason the string concatenation wasn't working when I tried doing this in a loop.
    
    settings.Devices[0].Features = CommandFlag::None;
    if(webServer.hasArg("feature_beep0"))
      settings.Devices[0].EnableFeature(CommandFlag::Beep);
    if(webServer.hasArg("feature_vibrate0"))
      settings.Devices[0].EnableFeature(CommandFlag::Vibrate);
    if(webServer.hasArg("feature_shock0"))
      settings.Devices[0].EnableFeature(CommandFlag::Shock);
    settings.Devices[0].DeviceId = webServer.arg("device_id0").toInt();
    settings.Devices[0].ShockIntensity = webServer.arg("shock_max_intensity0").toInt();
    settings.Devices[0].ShockDuration = webServer.arg("shock_max_duration0").toInt();
    settings.Devices[0].ShockInterval = webServer.arg("shock_interval0").toInt();
    settings.Devices[0].VibrateIntensity = webServer.arg("vibrate_max_intensity0").toInt();
    settings.Devices[0].VibrateDuration = webServer.arg("vibrate_max_duration0").toInt();
    
    settings.Devices[1].Features = CommandFlag::None;
    if(webServer.hasArg("feature_beep1"))
      settings.Devices[1].EnableFeature(CommandFlag::Beep);
    if(webServer.hasArg("feature_vibrate1"))
      settings.Devices[1].EnableFeature(CommandFlag::Vibrate);
    if(webServer.hasArg("feature_shock1"))
      settings.Devices[1].EnableFeature(CommandFlag::Shock);
    settings.Devices[1].DeviceId = webServer.arg("device_id1").toInt();
    settings.Devices[1].ShockIntensity = webServer.arg("shock_max_intensity1").toInt();
    settings.Devices[1].ShockDuration = webServer.arg("shock_max_duration1").toInt();
    settings.Devices[1].ShockInterval = webServer.arg("shock_interval1").toInt();
    settings.Devices[1].VibrateIntensity = webServer.arg("vibrate_max_intensity1").toInt();
    settings.Devices[1].VibrateDuration = webServer.arg("vibrate_max_duration1").toInt();

    settings.Devices[2].Features = CommandFlag::None;
    if(webServer.hasArg("feature_beep2"))
      settings.Devices[2].EnableFeature(CommandFlag::Beep);
    if(webServer.hasArg("feature_vibrate2"))
      settings.Devices[2].EnableFeature(CommandFlag::Vibrate);
    if(webServer.hasArg("feature_shock2"))
      settings.Devices[2].EnableFeature(CommandFlag::Shock);
    settings.Devices[2].DeviceId = webServer.arg("device_id2").toInt();
    settings.Devices[2].ShockIntensity = webServer.arg("shock_max_intensity2").toInt();
    settings.Devices[2].ShockDuration = webServer.arg("shock_max_duration2").toInt();
    settings.Devices[2].ShockInterval = webServer.arg("shock_interval2").toInt();
    settings.Devices[2].VibrateIntensity = webServer.arg("vibrate_max_intensity2").toInt();
    settings.Devices[2].VibrateDuration = webServer.arg("vibrate_max_duration2").toInt();

    settings.RequireDeviceID = webServer.hasArg("require_device_id");
    settings.AllowRemoteAccess = webServer.hasArg("allow_remote_access");
      
    WS_SendConfig();
  }
  
  if(webServer.hasArg("configure_wifi"))
  {
    if(webServer.hasArg("wifi_ssid"))
       webServer.arg("wifi_ssid").toCharArray(settings.WifiName, 33);
    if(webServer.hasArg("wifi_password"))
       webServer.arg("wifi_password").toCharArray(settings.WifiPassword, 65);
  }

  EEPROM.put(0, settings);
  EEPROM.commit();

  webServer.sendHeader("Location", "http://shockies.local/");
  webServer.sendHeader("Cache-Control", "no-cache");
  webServer.send(301);

  // If we were updating the Wi-Fi configuration, reboot the controller.
  if(webServer.hasArg("configure_wifi"))
  {
    Serial.println("Wi-Fi configuration changed, rebooting...");
    delay(1000);
    ESP.restart();
  }
}


void WS_HandleEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch(type) {
      case WStype_DISCONNECTED:
          Serial.printf("[%u] Disconnected!\n", num);
          break;
      case WStype_CONNECTED:
          {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            if(settings.RequireDeviceID)
            {
              if(length < UUID_STR_LEN || strcmp((char*)&payload[1], settings.DeviceID) != 0)
              {
                Serial.printf("[%u] Failed validation for token, disconnecting...\n", num);
                webSocket.disconnect(num);
                return;
              }
            }
            webSocket.sendTXT(num, "OK: CONNECTED");
            WS_SendConfig();
          }
          break;
      case WStype_TEXT:
          {   
              if(emergencyStop)
              {
                Serial.printf("[%u] EMERGENCY STOP\nDevice will not accept commands until rebooted.\n", num);
                webSocket.broadcastTXT("ERROR: EMERGENCY STOP");
                return;        
              }

              char* command = strtok((char*)payload, " ");
              char* id_str = strtok(0, " ");
              char* intensity_str = strtok(0, " ");

              if(command == 0)
              {
                Serial.printf("[%u] Text Error: Invalid Message", num);
                webSocket.sendTXT(num, "ERROR: INVALID FORMAT");
                break;
              }

              //Reset the current command status, and stop sending the command.
              if(*command == 'R')
              {
                uint32_t commandTimeout = ((currentCommand.Command == CommandFlag::Shock) ? settings.Devices[currentCommand.DeviceIndex].ShockDuration : settings.Devices[currentCommand.DeviceIndex].VibrateDuration) * 1000;
                lastCommand = currentCommand;
                lastCommand.EndTime = min((uint32_t)millis(), lastCommand.StartTime + commandTimeout);
                currentCommand.Reset();
                webSocket.sendTXT(num, "OK: R");
                break;
              }
              //Triggers an emergency stop. This will require the ESP-32 to be rebooted.
              else if(*command == 'X')
              {
                emergencyStop = true;
                currentCommand.Reset();
                webSocket.sendTXT(num, "OK: EMERGENCY STOP");
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
                Serial.printf("[%u] Text Error: Invalid Message\n", num);
                webSocket.sendTXT(num, "ERROR: INVALID FORMAT");
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
                webSocket.sendTXT(num, "OK: B");
                break;
              }
              //Vibrate
              else if(*command == 'V' && settings.Devices[id].FeatureEnabled(CommandFlag::Vibrate))
              {
                currentCommand.Set(id, CommandFlag::Vibrate, intensity);
                webSocket.sendTXT(num, "OK: V");
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
                webSocket.sendTXT(num, "OK: S");
                break;
              }
              else
              {
                Serial.printf("[%u] Ignored Command %c\n", num, *command);
                break;
              }
          }
          break;
  case WStype_BIN:
  case WStype_ERROR:      
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
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
  webSocket.broadcastTXT(configBuffer);
}
