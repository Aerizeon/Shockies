#ifndef _HTMLContent_h
#define _HTMLContent_h

static const char* HTML_IndexDefault = R"HTML(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Shockies - Safety Configuration</title>
    <style>
    html { font-family: Helvetica; display: inline-block; margin: 0px auto;}
    body{margin-top: 20px; display:flex; align-items:center; flex-direction: column;}
    h1 {color: #444444;margin: 50px auto 30px;}
    h3 {color: #444444;margin-bottom: 50px;}
    input { margin: 5px;}
    .button {display: block; background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
    .button:active {background-color: #2980b9;}
    #intensity_settings { display:flex; align-items:flex-end; flex-direction:column; }
    p {font-size: 14px;color: #888;margin-bottom: 10px;}
    
    .tab-wrap {
      -webkit-transition: 0.3s box-shadow ease;
      transition: 0.3s box-shadow ease;
      border-radius: 1px;
      width: 35em;
      display: -webkit-box;
      display: -ms-flexbox;
      display: flex;
      -ms-flex-wrap: wrap;
        flex-wrap: wrap;
      position: relative;
      list-style: none;
      background-color: #fff;
      margin: 40px 0;
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.12), 0 1px 2px rgba(0, 0, 0, 0.24);
      width: 100dp;
    }


    .tab {
      display: none;
    }
    .tab:checked:nth-of-type(1) ~ .tab__content:nth-of-type(1) {
      opacity: 1;
      position: relative;
      top: 0;
      z-index: 100;
      text-shadow: 0 0 0;
    }
    .tab:checked:nth-of-type(2) ~ .tab__content:nth-of-type(2) {
      opacity: 1;
      position: relative;
      top: 0;
      z-index: 100;
      text-shadow: 0 0 0;
    }
    .tab:checked:nth-of-type(3) ~ .tab__content:nth-of-type(3) {
      opacity: 1;
      position: relative;
      top: 0;
      z-index: 100;
      text-shadow: 0 0 0;
    }
    .tab:checked:nth-of-type(4) ~ .tab__content:nth-of-type(4) {
      opacity: 1;
      position: relative;
      top: 0;
      z-index: 100;
      text-shadow: 0 0 0;
    }

    .tab:checked:nth-of-type(5) ~ .tab__content:nth-of-type(5) {
      opacity: 1;
      position: relative;
      top: 0;
      z-index: 100;
      text-shadow: 0 0 0;
    }

    .tab:first-of-type:not(:last-of-type) + label {
      border-top-right-radius: 0;
      border-bottom-right-radius: 0;
    }
    .tab:not(:first-of-type):not(:last-of-type) + label {
      border-radius: 0;
    }
    .tab:last-of-type:not(:first-of-type) + label {
      border-top-left-radius: 0;
      border-bottom-left-radius: 0;
    }
    .tab:checked + label {
      background-color: #fff;
      box-shadow: 0 -1px 0 #fff inset;
      cursor: default;
    }
    .tab:checked + label:hover {
      box-shadow: 0 -1px 0 #fff inset;
      background-color: #fff;
    }
    .tab + label {
      box-shadow: 0 -1px 0 #eee inset;
      border-radius: 1px 1px 0 0;
      cursor: pointer;
      display: block;
      text-decoration: none;
      color: #333;
      -webkit-box-flex: 3;
      -ms-flex-positive: 3;
      flex-grow: 3;
      text-align: center;
      background-color: #f2f2f2;
      -webkit-user-select: none;
       -moz-user-select: none;
        -ms-user-select: none;
          user-select: none;
      text-align: center;
      -webkit-transition: 0.3s background-color ease, 0.3s box-shadow ease;
      transition: 0.3s background-color ease, 0.3s box-shadow ease;
      height: 50px;
      box-sizing: border-box;
      padding: 15px;
    }
    .tab + label:hover {
      background-color: #f9f9f9;
      box-shadow: 0 1px 0 #f4f4f4 inset;
    }
    .tab__content {
      padding: 10px 25px;
      background-color: transparent;
      position: absolute;
      width: 100%%;
      z-index: -1;
      opacity: 0;
      left: 0;
      border-radius: 1px;
    }
  </style>
  <script type="text/javascript">
    document.addEventListener("DOMContentLoaded", function() { 
      var webSocket = null;
      var button = document.querySelector('#sendButton');
      
      if(window.location.protocol == 'file:') {
        webSocket = new WebSocket("ws://shockies.local:81");
      }
      else {
        webSocket = new WebSocket("ws://" + window.location.hostname + ":81/%s");
      }
      webSocket.addEventListener('open', function (event) {
        button.disabled = false;
      });
      
      button.addEventListener('mousedown', e => {
        if(webSocket != null && webSocket.readyState == WebSocket.OPEN) {
          var mode = document.querySelector('input[name="mode"]:checked').value;
          var intensity = document.querySelector('input[name="intensity"]').value;
          webSocket.send(mode + " 0 " + intensity);
        }
      });
      button.addEventListener('mouseup', e => {
        if(webSocket != null && webSocket.readyState == WebSocket.OPEN) {
          webSocket.send("R");
        }
      });
      window.setInterval(function(){
        if(webSocket != null && webSocket.readyState == WebSocket.OPEN) {
          webSocket.send("P");
        }
      }, 1000);
      console.log("Loaded");
    });
  </script>
  </head>
  <body>
    <h1>Shockies</h1>
    <h3>Configuration</h3>
  <form action="/submit" method="post">
    <div class="tab-wrap">
      <input type="radio" id="tab1" name="tabGroup1" class="tab" checked>
      <label for="tab1">Device 1</label>
      <input type="radio" id="tab2" name="tabGroup1" class="tab">
      <label for="tab2">Device 2</label>
      <input type="radio" id="tab3" name="tabGroup1" class="tab">
      <label for="tab3">Device 3</label>
      <input type="radio" id="tab4" name="tabGroup1" class="tab">
      <label for="tab4">Security</label>
      <input type="radio" id="tab5" name="tabGroup1" class="tab">
      <label for="tab5">Local Control</label>
      <div class="tab__content">  
        <div>
          <label for="device_id0">Device ID:</label>
          <input type="number" id="device_id0" name="device_id0" min="0" max="65535" value="%u"><br/>
        </div>
        <fieldset>
        <legend>Enabled Features</legend>
        <input type="checkbox" id="feature_beep0" name="feature_beep0" value="true" %s>
        <label for="feature_beep0">Beep</label><br/>
        <input type="checkbox" id="feature_vibrate0" name="feature_vibrate0" value="true" %s>
        <label for="feature_vibrate0">Vibrate</label><br/>
        <input type="checkbox" id="feature_shock0" name="feature_shock0" value="true" %s>
        <label for="feature_shock0">Shock</label><br/>
        </fieldset>
        <br>
        <fieldset id="intensity_settings">
        <legend>Maximum Intensity Settings</legend>
        <div>
          <label for="shock_max_intensity0">Max Shock Intensity:</label>
          <input type="number" id="shock_max_intensity0" name="shock_max_intensity0" min="1" max="100" value="%u"><br/>
        </div>
        <div>
          <label for="shock_max_duration0">Max Shock Duration:</label>
          <input type="number" id="shock_max_duration0" name="shock_max_duration0" min="1" max="10" value="%u"><br/>
        </div>
        <div>
          <label for="shock_interval0">Shock Interval:</label>
          <input type="number" id="shock_interval0" name="shock_interval0" min="1" max="10" value="%u"><br/>
        </div>
        <div>
          <label for="vibrate_max_intensity0">Max Vibrate Intensity:</label>
          <input type="number" id="vibrate_max_intensity0" name="vibrate_max_intensity0" min="1" max="100" value="%u"><br/>
        </div>
        <div>
          <label for="vibrate_max_duration0">Max Vibrate Duration:</label>
          <input type="number" id="vibrate_max_duration0" name="vibrate_max_duration0" min="1" max="10" value="%u"><br/>
        </div>
        </fieldset>
      </div>
      <div class="tab__content">
        <div>
          <label for="device_id1">Device ID:</label>
          <input type="number" id="device_id1" name="device_id1" min="0" max="65535" value="%u"><br/>
        </div>
        <fieldset>
        <legend>Enabled Features</legend>
        <input type="checkbox" id="feature_beep1" name="feature_beep1" value="true" %s>
        <label for="feature_beep1">Beep</label><br/>
        <input type="checkbox" id="feature_vibrate1" name="feature_vibrate1" value="true" %s>
        <label for="feature_vibrate1">Vibrate</label><br/>
        <input type="checkbox" id="feature_shock1" name="feature_shock1" value="true" %s>
        <label for="feature_shock1">Shock</label><br/>
        </fieldset>
        <br>
        <fieldset id="intensity_settings">
        <legend>Maximum Intensity Settings</legend>
        <div>
          <label for="shock_max_intensity1">Max Shock Intensity:</label>
          <input type="number" id="shock_max_intensity1" name="shock_max_intensity1" min="1" max="100" value="%u"><br/>
        </div>
        <div>
          <label for="shock_max_duration1">Max Shock Duration:</label>
          <input type="number" id="shock_max_duration1" name="shock_max_duration1" min="1" max="10" value="%u"><br/>
        </div>
        <div>
          <label for="shock_interval1">Shock Interval:</label>
          <input type="number" id="shock_interval1" name="shock_interval1" min="1" max="10" value="%u"><br/>
        </div>
        <div>
          <label for="vibrate_max_intensity1">Max Vibrate Intensity:</label>
          <input type="number" id="vibrate_max_intensity1" name="vibrate_max_intensity1" min="1" max="100" value="%u"><br/>
        </div>
        <div>
          <label for="vibrate_max_duration1">Max Vibrate Duration:</label>
          <input type="number" id="vibrate_max_duration1" name="vibrate_max_duration1" min="1" max="10" value="%u"><br/>
        </div>
        </fieldset>
      </div>
      <div class="tab__content">
        <div>
          <label for="device_id2">Device ID:</label>
          <input type="number" id="device_id2" name="device_id2" min="0" max="65535" value="%u"><br/>
        </div>
        <fieldset>
        <legend>Enabled Features</legend>
        <input type="checkbox" id="feature_beep2" name="feature_beep2" value="true" %s>
        <label for="feature_beep2">Beep</label><br/>
        <input type="checkbox" id="feature_vibrate2" name="feature_vibrate2" value="true" %s>
        <label for="feature_vibrate2">Vibrate</label><br/>
        <input type="checkbox" id="feature_shock2" name="feature_shock2" value="true" %s>
        <label for="feature_shock2">Shock</label><br/>
        </fieldset>
        <br>
        <fieldset id="intensity_settings">
        <legend>Maximum Intensity Settings</legend>
        <div>
          <label for="shock_max_intensity2">Max Shock Intensity:</label>
          <input type="number" id="shock_max_intensity2" name="shock_max_intensity2" min="1" max="100" value="%u"><br/>
        </div>
        <div>
          <label for="shock_max_duration2">Max Shock Duration:</label>
          <input type="number" id="shock_max_duration2" name="shock_max_duration2" min="1" max="10" value="%u"><br/>
        </div>
        <div>
          <label for="shock_interval2">Shock Interval:</label>
          <input type="number" id="shock_interval2" name="shock_interval2" min="1" max="10" value="%u"><br/>
        </div>
        <div>
          <label for="vibrate_max_intensity2">Max Vibrate Intensity:</label>
          <input type="number" id="vibrate_max_intensity2" name="vibrate_max_intensity2" min="1" max="100" value="%u"><br/>
        </div>
        <div>
          <label for="vibrate_max_duration2">Max Vibrate Duration:</label>
          <input type="number" id="vibrate_max_duration2" name="vibrate_max_duration2" min="1" max="10" value="%u"><br/>
        </div>
        </fieldset>
      </div>
      <div class="tab__content">
        <input type="checkbox" id="require_device_id" name="require_device_id" title="Require DeviceID to be part of the local websocket URI" %s>
        <label for="require_device_id">Require device ID for local control</label><br/>
        <input type="checkbox" id="allow_remote_access" name="allow_remote_access" title="Allow this device to be controlled via shockies.dev" %s>
        <label for="allow_remote_access">Allow Remote Access</label><br/>
      </div>
      <div class="tab__content">
        <fieldset>
          <legend>Mode</legend>
          <input type="radio" id="mode_beep" name="mode" value="B">
          <label for="mode_beep">Beep</label><br/>
          <input type="radio" id="mode_vibrate" name="mode" value="V" checked >
          <label for="mode_vibrate">Vibrate</label><br/>
          <input type="radio" id="mode_shock" name="mode" value="S">
          <label for="mode_shock">Shock</label><br/>
        </fieldset>
        <label for="intensity">Intensity:</label>
        <input type="number" id="intensity" name="intensity" min="0" max="100" value="10" >
        <input id="sendButton" class="button" type="button" Value="Send" disabled />
      </div>
    </div>
    <input type="hidden" name="configure_features" value="true"/>
    <input class="button" type="submit" value="Save"/>
  </form>
  <pre>ws://shockies.local:81/%s</pre>
  </body>
</html>
  )HTML";

static const char* HTML_IndexConfigureSSID = R"HTML(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Shockies - Wi-Fi Configuration</title>
    <style>
      html { font-family: Helvetica; display: inline-block; margin: 0px auto;}
      body{margin-top: 20px; display:flex; align-items:center; flex-direction: column;}
      h1 {color: #444444;margin: 50px auto 30px;}
      h3 {color: #444444;margin-bottom: 50px;}
      input { margin: 5px;}
      .button {display: block; background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
      .button:active {background-color: #2980b9;}
      #intensity_settings { display:flex; align-items:flex-end; flex-direction:column; }
      p {font-size: 14px;color: #888;margin-bottom: 10px;}
    </style>
  </head>
  <body>
    <h1>Shockies</h1>
    <h3>Wi-Fi Configuration</h3>
    <form action="/submit" method="post">
      <fieldset id="wifi_settings">
        <legend>Wi-Fi Settings</legend>
        <div>
          <label for="wifi_ssid">SSID:</label>
          <input type="text" id="wifi_ssid" name="wifi_ssid" minlength="1" maxlength="32" value="%s"><br/>
        </div>
        <div>
          <label for="wifi_password">Password:</label>
          <input type="text" id="wifi_password" name="wifi_password" minlength="8" maxlength="64" value="%s"><br/>
        </div>
      </fieldset>
      <br>
      <input type="hidden" name="configure_wifi" value="true"/>
      <input class="button" type="submit" Value="Save"/>
    </form>
  </body>
</html>
  )HTML";
#endif
