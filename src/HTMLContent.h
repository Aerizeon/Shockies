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
    </style>
  </head>
  <body>
    <h1>Shockies</h1>
    <h3>Safety Configuration</h3>
    <form action="/submit" method="post">
      <fieldset>
        <legend>Enabled Features</legend>
        <input type="checkbox" id="feature_beep" name="feature_beep" value="true" %s>
        <label for="feature_beep">Beep</label><br/>
        <input type="checkbox" id="feature_vibrate" name="feature_vibrate" value="true" %s>
        <label for="feature_vibrate">Vibrate</label><br/>
        <input type="checkbox" id="feature_shock" name="feature_shock" value="true" %s>
        <label for="feature_shock">Shock</label><br/>
      </fieldset>
      <br>
      <fieldset id="intensity_settings">
        <legend>Maximum Intensity Settings</legend>
        <div>
          <label for="shock_max_intensity">Max Shock Intensity:</label>
          <input type="number" id="shock_max_intensity" name="shock_max_intensity" min="1" max="100" value="%u"><br/>
        </div>
        <div>
          <label for="shock_max_duration">Max Shock Duration:</label>
          <input type="number" id="shock_max_duration" name="shock_max_duration" min="1" max="10" value="%u"><br/>
        </div>
        <div>
          <label for="vibrate_max_intensity">Max Vibrate Intensity:</label>
          <input type="number" id="vibrate_max_intensity" name="vibrate_max_intensity" min="1" max="100" value="%u"><br/>
        </div>
        <div>
          <label for="vibrate_max_duration">Max Vibrate Duration:</label>
          <input type="number" id="vibrate_max_duration" name="vibrate_max_duration" min="1" max="10" value="%u"><br/>
        </div>
      </fieldset>
      <br>
      <input type="hidden" name="configure_features" value="true"/>
      <input class="button" type="submit" value="Save"/>
    </form>
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

static const char* HTML_Control = R"HTML(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Shockies - Manual Control</title>
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
    <h3>Manual Control</h3>
    <section>
      <fieldset>
        <legend>Mode</legend>
        <input type="radio" id="mode_beep" name="mode" value="beep">
        <label for="mode_beep">Beep</label><br/>
        <input type="radio" id="mode_vibrate" name="mode" value="vibrate">
        <label for="mode_vibrate">Vibrate</label><br/>
        <input type="radio" id="mode_shock" name="mode" value="shock">
        <label for="mode_shock">Shock</label><br/>
      </fieldset>
      <div>
        <label for="intensity">Intensity:</label>
        <input type="number" id="intensity" name="intensity" min="0" max="100" value="0"><br/>
      </div>
      <input class="button" type="button" Value="Send"/>
    </section>
  </body>
</html>
  )HTML";
#endif
