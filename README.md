# Shockies
ESP32 Firmware for controlling **Petrainer PET998DR** and **FunniPets** collars using a **STX882 433.9Mhz ASK Transmitter**

## Disclaimer
I am not responsible for how you use or misuse this software, or any hardware controlled by this software.

I am not responsible for any malfunctions, and make no guarantees about safety or suitability of this software for any purpose beyond proof of concept.

**Shock collars are NOT designed for human use.**

**NEVER set the shock intensity above 50%**

**NEVER use a shock collar if you have a history of heart conditions, or are using a pacemaker**

**NEVER use a shock collar on your neck or chest area**

## Hardware
### Required Items
* Petrainer PET998DR or FunniPets collars
  - PET998DR can be found on [eBay](https://www.ebay.com/itm/181705501723)
  - FunniPets collars can be found on [Amazon](https://www.amazon.com/FunniPets-Training-Collar-Accessory-Receiver/dp/B0874GMM93/)
* ESP32 (NOT ESP32-S2!)
  - [ESP32 Developer boards](https://www.amazon.com/dp/B0718T232Z) are easiest to work with
  - [ESP32-WROOM-32D](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-WROOM-32D-4MB/9381716) will also work
  - Other modules may work, but be careful
* STX882 ASK Transmitter for 433.9Mhz
  - Can be found on [Amazon](https://www.amazon.com/dp/B09KY28VH8)
* MicroUSB Cable
  - Used to connect to your PC, or to your power supply in standalone mode.
* [Male to Female Jumpers](https://www.amazon.com/HiLetgo-Breadboard-Prototype-Assortment-Raspberry/dp/B077X7MKHN)
* Soldering iron + Solder
  - Used to connect the jumpers and antenna to the STX882 Transmitter card.
## Assembly
You will need the following:
 * ESP32
 * STX882 Transmitter
 * STX882 Transmitter Antenna (looks like a coiled spring)
 * Male to Female Jumper Wire
 * Soldering Iron
 * Solder

1. Preheat Soldering iron to the temperature required for the solder you're using.
2. Separate a set of 3 jumper wires from the bundle, and set the remaining wires aside. You only need 3 Male to Female wires for this project.
3. Carefully solder the antenna to the ANT connection on the STX882 Transmitter. It should point straight up, away from the body of the transmitter for best range.
   - The ANT pin might be unlabled - it is the hole at the top of the card, opposite the DATA/VCC/GND Pins.
4. Carefully solder the Male side of each jumper to the DATA, VCC, and GND pins on the STX882 Transmitter.
   - Color doesn't really matter here - use whatever you'd like, just make sure they're visualy distinct from each other.
6. Turn off soldering iron, it is no longer needed.

Once the transmitter is sufficiently cool, make the following connections:
| ESP32 | Transmitter|
| ----- | ---------- |
| +5V   | VCC        |
| GND   | GND        |
| P4    | DATA       |

Once you verify these connections are correct, you're now ready to connect the ESP32 to your computer, and move on to **Setup**

## Setup
Setting up Shockies and your collar
### Visual Studio Code + PlatformIO & Flashing
1. Download the VSCode IDE from the [visual studio code website](https://code.visualstudio.com/download)
2. Install the PlatformIO IDE extension in VSCode
3. Open the Shockies project folder in VSCode, and allow PlatformIO time to get setup
4. Select the PlatformIO tab on the left side of the VSCode window, and select a task (such as *Upload** or *Upload and Monitor*) to build and upload the shockies code for your device. If you are using a different ESP32 board than the default, select *PIO Home > Boards* and select a different board configuration.

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

### Configuration
The configuration page found on http://shockies.local allows you to set maximum values for intensity and duration for each of the 3 mappable collars. 
Additionally, you can choose between the *PET998DR Clone* and *FunniPets* collars

#### Enabled Features
Allows the user to specified which collar features are enabled. 
* Light
* Beep
* Vibrate
* Shock
Not all features will be available on every collar, so these settings may not have any noticable effect.

#### Maximum Intensity Settings
Specifies the maximum allowable intensity and continuious duration for Shock and Vibrate commands.
* Max Shock Intensity - Default 30%
* Max Shock Duration - Default 5 Seconds
* Shock Interval - Default 3 Seconds
* Max Vibrate Intensity - Default 100%
* Max Vibrate Duration - Default 5 Seconds

Please **CAREFULLY** experiment starting with low (<5%) shock intensity, and slowly increase it until you find a suitable maximum intensity.
The **Shock Interval** setting 

## Firmware Updates
The updates page on http://shockies.local/update allows you to remotely update the firmware on your device. The username will be `admin`, and the password will be the same as your WiFi network password.
From this page, you can upload `firmware.bin` and `spiffs.bin` which can be found in the [releases](https://github.com/Aerizeon/Shockies/releases) section.



## NeosVR Control
Message Epsilion for more information.

An alpha implementation can be found in the follwing public folder:

`neosrec:///U-Epsilion/R-4af8f73a-3765-4ff7-ada0-cdd199286215`

## Web Control
*Not yet implemented*

