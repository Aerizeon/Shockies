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

/*
 * TODO: Add Duty Cycle limits for shocking, so we 
 * can stop the transmission if people are spamming the
 * button
 */

 
void setup()
{
  int wifiConnectTime = 0;
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("Device is booting...");
  EEPROM.begin(sizeof(featureSettings));
  EEPROM.get(0, featureSettings);
  if(featureSettings.Features == CommandFlag::Invalid){
    featureSettings.Features = CommandFlag::All;
    featureSettings.ShockIntensity = 50;
    featureSettings.ShockDuration = 5;
    featureSettings.VibrateIntensity = 100;
    featureSettings.VibrateDuration = 5;
    memset(featureSettings.WifiName, 0, 33);
    memset(featureSettings.WifiPassword, 0, 65);
    EEPROM.put(0, featureSettings);
    EEPROM.commit();
  }

  pinMode(4, OUTPUT);
  
  if(strlen(featureSettings.WifiName) > 0)
  {
    Serial.println("Connecting to Wi-Fi..."); 
    Serial.printf("SSID: %s\n", featureSettings.WifiName);
    WiFi.mode(WIFI_STA);
    WiFi.begin(featureSettings.WifiName, featureSettings.WifiPassword);
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
  uint32_t commandTimeout = ((currentCommand.Command == CommandFlag::Shock) ? featureSettings.ShockDuration : featureSettings.VibrateDuration) * 1000;
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
  if(currentTime - currentCommand.Time > commandTimeout)
    return;

  // Transmit the current command to the collar
  SendPacket(currentCommand.CollarId, currentCommand.Command, currentCommand.Intensity);
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
  webServer.on("/control", HTTP_GET, HTTP_GET_Control);
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
  //U = Unknown Flags
  //C = Command
  //I = ID?
  //S = Strength
  //R = Reverse(Command | UFlags) ^ 0xFF
  
  //UUUUUCCC IIIIIIII IIIIIIII SSSSSSSS RRRRRRRR
  //10000001 00000010 00000011 01100100 01111110 S100
  //10000010 00000010 00000011 01100100 10111110 V100
  //10000100 00000010 00000011 01100100 11011110 B100
  
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
    featureSettings.FeatureEnabled(CommandFlag::Beep) ? "checked" : "",
    featureSettings.FeatureEnabled(CommandFlag::Vibrate) ? "checked" : "",
    featureSettings.FeatureEnabled(CommandFlag::Shock) ? "checked" : "",
    featureSettings.ShockIntensity,
    featureSettings.ShockDuration,
    featureSettings.VibrateIntensity,
    featureSettings.VibrateDuration);
    webServer.send(200, "text/html", htmlBuffer); 
  }
  // Otherwise, assume we're in SoftAP mode, and send the Wi-Fi configuration page
  else
  {
    sprintf(htmlBuffer,
    HTML_IndexConfigureSSID,
    featureSettings.WifiName,
    featureSettings.WifiPassword);
    webServer.send(200, "text/html", htmlBuffer); 
  }
}

void HTTP_GET_Control()
{
  webServer.send(200, "text/html", HTML_Control); 
}

void HTTP_POST_Submit()
{
  int wifiConnectTime = 0;
  if(webServer.hasArg("configure_features"))
  {
    featureSettings.Features = CommandFlag::None;
    if(webServer.hasArg("feature_beep"))
      featureSettings.EnableFeature(CommandFlag::Beep);
    if(webServer.hasArg("feature_vibrate"))
      featureSettings.EnableFeature(CommandFlag::Vibrate);
    if(webServer.hasArg("feature_shock"))
      featureSettings.EnableFeature(CommandFlag::Shock);
    featureSettings.ShockIntensity = webServer.arg("shock_max_intensity").toInt();
    featureSettings.ShockDuration = webServer.arg("shock_max_duration").toInt();
    featureSettings.VibrateIntensity = webServer.arg("vibrate_max_intensity").toInt();
    featureSettings.VibrateDuration = webServer.arg("vibrate_max_duration").toInt();
  }
  
  if(webServer.hasArg("configure_wifi"))
  {
    if(webServer.hasArg("wifi_ssid"))
       webServer.arg("wifi_ssid").toCharArray(featureSettings.WifiName, 33);
    if(webServer.hasArg("wifi_password"))
       webServer.arg("wifi_password").toCharArray(featureSettings.WifiPassword, 65);
  }

  EEPROM.put(0, featureSettings);
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
            // send message to client
            webSocket.sendTXT(num, "OK: CONNECTED");
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
              if(*command == 'R'){
                currentCommand.Reset();
                webSocket.sendTXT(num, "OK: R");
                break;
              }
              //Triggers an emergency stop. This will require the ESP-32 to be rebooted.
              else if(*command == 'X'){
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

              if(id_str == 0 || intensity_str == 0) {
                Serial.printf("[%u] Text Error: Invalid Message\n", num);
                webSocket.sendTXT(num, "ERROR: INVALID FORMAT");
                break;
              }

              uint16_t id = atoi(id_str);
              uint8_t intensity = atoi(intensity_str);

              //Beep - Intensity is not used.
              if(*command == 'B' && featureSettings.FeatureEnabled(CommandFlag::Beep)){
                currentCommand.Set(id, CommandFlag::Beep, intensity);
                webSocket.sendTXT(num, "OK: B");
                break;
              }
              //Vibrate
              else if(*command == 'V' && featureSettings.FeatureEnabled(CommandFlag::Vibrate)) {
                currentCommand.Set(id, CommandFlag::Vibrate, intensity);
                webSocket.sendTXT(num, "OK: V");
                break;
              }
              //Shock
              else if(*command == 'S' && featureSettings.FeatureEnabled(CommandFlag::Shock)) {
                currentCommand.Set(id, CommandFlag::Shock, intensity);
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
