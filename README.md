# Homemade-AirTag
This was my final project for my intro to engineering course at UTD. This project utilised an ESP32 and a NEO-6M GPS module to communicate to an iPhone and compare the locations of the created device and the phone. Once the distance was great enough, a discord message would be sent via webhooks into a personal server. At the moment this is a prototype which I am hoping to improve upon. <br/>
***Please note that currently this project only works for iPhone, and I will be working on an android version as soon as I get a friend with an android willing to help.*** <br/>

# Setup

The parts necessary for this build will be very simple: an ESP32, a NEO-6M GPS module, an active buzzer, and a 9V battery. <br/>
On the NEO-6M, there should be 5 pins. We will be utilising 3 of them, although connecting a 4th is optional. The following connections between the ESP32 and NEO-6M should be made: <br/>
* Vin <--> Vcc <br/>
* GND <--> GND <br/>
* IO2 <--> RXD <br/>
* IO4 <--> TXD <br/>

These connections will be utilised as UART to allow the NEO-6M to send information to the ESP32; while the ESP itself does not require sending information to the NEO-6M at the moment, it does not hurt to connect the pins in case a future version will require somthing of the sort. <br/>
Next, connect the buzzer anode to IO27 on the ESP32, connect the cathode to another GND pin. The final circuit should look like this schematic, reference it if you get confused at any point. <br/>

<img src= "https://github.com/user-attachments/assets/6724c615-d056-4b5d-93d0-86e8fef3b8a5" width=50% height=50%> <br/>

To power the project once it is disconnected from a computer, you should connect a 9V battery (please note that most 9V batteries do not come with an adapter to USB-C, which is the connection type of the ESP32, so buying one may be necessary). If everything went as planned, the circuit should be complete! <br/>

# Code

For understanding of the general flow of the code, please reference the following flowchart: <br/>

![image](https://github.com/user-attachments/assets/6a5933dc-bf28-4620-8548-4224c3139a19)

Copy `AirTagConfig.example.h` to the Git-ignored `AirTagConfig.h` and configure the SSID and password of your personal iPhone hotspot (I am hoping in the future to rid the project of this requirement entirely, but for now it is necessary). <br/>
Next, please enter into your iPhone's shortcuts app, it will be utilised to send the ESP32 the coordinates necessary to track your phone. Create a new shortcut and follow the steps outlined in the following photo. <br/>

<img src= "https://github.com/user-attachments/assets/64a06293-dd82-4510-9d7c-38e8c169f70e" width=25% height=25%>

**The old screenshot and bundled shortcut predate authentication and are obsolete. Use the authenticated POST setup below.** <br/>
Next, create a personal discord server. Nothing fancy needs to happen in it, the default server settings should do. Click on the settings of your "general" channel, go to the integrations tab, and enter the webhooks subtab. From here, create a new webhook, and copy the webhook url (you will need to paste it into the code starting from "/api..."). <br/>
You are now done with all of the prepwork. Copy all of the variables into the code in their place, upload the code onto your ESP32 (make sure that when you start up the AirTag, your hotspot is already open, also note that YOU MUST TURN ON THE MAXIMISE COMPATABILITY SETTING IN YOUR PHONE HOTSPOT SETTINGS OR THE AIRTAG WILL NOT WORK) and run the shortcut that you created. If everything worked correctly, when connected to a computer, on the serial monitor you should see the distance calculated between the phone and device; precise coordinates are no longer logged. <br/>
On first launch, the GPS module may take time to acquire sattelite information. This usually takes 5-10 minutes on first launch outside, so be patient. <br/>

# Endnotes

Thank you for viewing my project. As of now this is a work in progress, meaning while the project itself was submitted (I got a 99.67, no idea where the 0.33 points went) I will continue to work and improve upon this concept. Current priorities are to rid the project of the hotspot requirement and make it compatable with android. Updates will be provided when progress is made. 

## Security setup (required before flashing)

- Use a current supported ESP32 Arduino core, TinyGPSPlus and a private WPA2/WPA3
  hotspot. Do not forward port 80 or expose the device to the Internet/shared Wi-Fi.
- Set credentials only in ignored `AirTagConfig.h`. Generate a unique GPS key with
  `python -c "import secrets; print(secrets.token_hex(32))"`; never commit it or
  include it in a URL. The blank example intentionally leaves the device disabled.
- In iOS Shortcuts, get the current location and extract latitude/longitude as
  plain numbers (without list brackets). Use **Get Contents of URL** with URL
  `http://172.20.10.2/gps`, method **POST**, header **X-AirTag-Key** containing your
  key, and form fields **lat** and **lon**. Old unauthenticated GET shortcuts are
  rejected. Treat the shortcut as a secret and do not share/export a populated copy.
- Phone updates expire after two minutes. Malformed/nonfinite/out-of-range
  coordinates and browser-origin requests are rejected without modifying position.
- The phone endpoint uses HTTP on the private hotspot: the key is authentication,
  not transport encryption or replay protection. Only use a trusted private link;
  if you need an untrusted LAN, add authenticated HTTPS before enabling access.
- Discord uses TLS certificate and hostname verification for `discord.com`, with
  the public GTS Root R4 from [Google Trust Services](https://pki.goog/repository/).
  Root DER SHA-256: `349dfa4058c5e263123b398ae795573c4e1313c83fe68f93556cd5e8031b3c7d`.
  The current chain was verified against that root on 2026-10-24. If Discord changes
  certificate authorities, update the trust anchor from the CA's official source
  after independently verifying the chain. Never restore `setInsecure()` or use an
  unchecked hardcoded IP as a workaround. Failed TLS/time synchronization stops
  the outbound request. NTP needs hotspot Internet access.

Repository changes do not update a physical ESP32. Reflash the firmware and update
its shortcut together; no device/hardware behavior has been verified here.

### Compilation reference

The source is compiled with `esp32:esp32` core **3.3.12**, board
`esp32:esp32:esp32` and **TinyGPSPlus 1.0.3**. Open `AirTag_Code.ino` in
Arduino IDE and accept its same-named sketch-folder prompt, keeping both
configuration headers and `discord_root_ca.h` beside it. Select the exact board
matching your hardware before uploading. Compilation uses the blank configuration
example; no Wi-Fi/webhook secrets or physical device are required for that check.
