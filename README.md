# Shockies
ESP-32 Firmware for controlling **Petrainer PET998DR** collars using a **STX882 433.9Mhz ASK Transmitter**

## Disclaimer
I am not responsible for how you use or misuse this software, or any hardware controlled by this software.

I am not responsible for any malfunctions, and make no guarantees about safety or suitability of this software for any purpose beyond proof of concept.

**Shock collars are NOT designed for human use.**

**NEVER set the shock intensity above 50%**

**NEVER use a shock collar if you have a history of heart conditions, or are using a pacemaker**

**NEVER use a shock collar on your neck or chest area**

## Hardware
### Required Items
* Petrainer PET998DR
  - Can be found on [eBay](https://www.ebay.com/itm/181705501723)
* ESP32 (NOT ESP32-S2!)
  - [ESP32 Developer boards](https://www.amazon.com/dp/B0718T232Z) are easiest to work with
  - [ESP32-WROOM-32D](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-WROOM-32D-4MB/9381716) will also work
  - Other modules may work, but be careful
* STX882 ASK Transmitter for 433.9Mhz
  - Can be found on [Amazon](https://www.amazon.com/dp/B09KY28VH8)
* MicroUSB Cable
  - Used to connect to your PC, or to your power supply in standalone mode.

## Setup
Setting up Shockies and your collar
### Arduino IDE & Flashing
1. Download the arduino IDE from the [arduino.cc website](https://www.arduino.cc/en/software/) or the [Microsoft App Store](https://www.microsoft.com/store/apps/9nblggh4rsd8)
2. Follow [these instructions](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager) to install ESP32 Board support for the Arduino IDE
3. Double click **Shockies.ino** to open it in the Arduino editor
4. Configure the correct board settings (May differ if using something other than the HiLetGo ESP32 Developer Board)
   - Select the following board: Tools > Board > ESP32 Arduino > Node32s
   - Select the correct upload speed: Tools > Upload Speed > 921600
   - Select the correct port: Tools > Port 
     - The correct port will depend on your computer
     - Open device manager (on windows), expand 'Ports (COM and LPT)' and find an entry with 'Silicon Labs CP210x USB to UART Bridge'
     - If no device exists, you may need to install the [CP210x Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
 5. Select Tools > Serial Monitor to view the debug output from your board.
    - Make sure the Baud Rate is set to 9600
 6. Select Sketch > Upload to upload to your ESP32 board.
    - You may need to hold down the IO0 button (right-hand button on the HiLetGo ESP32 Developer Board) during the upload process

### Wi-Fi Setup
During the first boot, the Serial Monitor will display something similar to the following:
```
Device is booting...
Failed to connect to Wi-Fi
Creating temporary Access Point for configuration...
Access Point created!
SSID: ShockiesConfig
Pass: zappyzap
Starting mDNS...
Starting HTTP Server on port 80...
Starting WebSocket Server on port 81...
Connect to one of the following to configure settings:
http://shockies.local
http://192.168.4.1
```
1. Connect to the `ShockiesConfig` Wi-Fi Network, with the password `zappyzap`
   - Your device may complain about lack of internet - ignore it for now.
2. Navigate to http://shockies.local or http://192.168.4.1 - this will bring you to the Wi-Fi network configuration page.
   - If this doesn't work, verify that your device hasn't automatically disconnected from the `ShockiesConfig` Network
3. Input the SSID and Password of the network you want your ESP32 to connect to
4. Press **Save**, the ESP32 will now Reboot.
 
The Serial Monitor should now display something similar to the following:
```
Wi-Fi configuration changed, rebooting...
Device is booting...
Connecting to Wi-Fi...
SSID: mySSID
Wi-Fi Connected!
Starting mDNS...
Starting HTTP Server on port 80...
Starting WebSocket Server on port 81...
Connect to one of the following to configure settings:
http://shockies.local
http://192.168.2.2 
```
You can now access your device using http://shockies.local or using the IP address shown in the Serial Monitor.

### Safety Configuration
The safety configuration page found on http://shockies.local allows you to set maximum values for intensity and duration, that override values sent to the device.

#### Enabled Features
Allows the user to specified which features are enabled. 
* Beep - Default ON
* Vibrate - Default ON
* Shock - Default ON

#### Maximum Intensity Settings
Specifies the maximum allowable intensity and continuious duration for Shock and Vibrate commands.
* Max Shock Intensity - Default 50%
* Max Shock Duration - Default 5 Seconds
* Max Vibrate Intensity - Default 100%
* Max Vibrate Duration - Default 5 Seconds

Please **CAREFULLY** experiment starting with low (<5%) shock intensity, and slowly increase it until you find a suitable maximum intensity.

## NeosVR Control
Message Epsilion for more information.

## Web Control
*Not yet implemented*

