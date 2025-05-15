# Homemade-AirTag
This was my final project for my intro to engineering course at UTD. This project utilised an ESP32 and a NEO-6M GPS module to communicate to an iPhone and compare the locations of the created device and the phone. Once the distance was great enough, a discord message would be sent via webhooks into a personal server. At the moment this is a prototype which I am hoping to improve upon.

# Setup

The parts necessary for this build will be very simple: an ESP32, a NEO-6M GPS module, an active buzzer, and a 9V battery.
On the NEO-6M, there should be 5 pins. We will be utilising 3 of them, although connecting a 4th is optional. The following connections between the ESP32 and NEO-6M should be made:
Vin <--> Vcc
GND <--> GND
IO2 <--> RXD
IO4 <--> TXD
These connections will be utilised as UART to allow the NEO-6M to send information to the ESP32; while the ESP itself does not require sending information to the NEO-6M at the moment, it does not hurt to connect the pins in case a future version will require somthing of the sort.
Next, connect the buzzer anode to IO27 on the ESP32, connect the cathode to another GND pin. The final circuit should look like this schematic, reference it if you get confused at any point.

![image](https://github.com/user-attachments/assets/6724c615-d056-4b5d-93d0-86e8fef3b8a5)

To power the project once it is disconnected from a computer, you should connect a 9V battery (please note that most 9V batteries do not come with an adapter to USB-C, which is the connection type of the ESP32, so buying one may be necessary). If everything went as planned, the circuit should be complete!

# Code

For understanding of the general flow of the code, please reference the following flowchart:

![image](https://github.com/user-attachments/assets/6a5933dc-bf28-4620-8548-4224c3139a19)

To modify the code for your own, you must first input the SSID and password of your personal iPhone hotspot (I am hoping in the future to rid the project of this requirement entirely, but for now it is necessary).
* Please note that currently this project only works for iPhone, and I will be working on an android version as soon as I get a friend with an android willing to help.
Next, please enter into your iPhone's shortcuts app, it will be utilised to send the ESP32 the coordinates necessary to track your phone. Create a new shortcut and follow the steps outlined in the following photo.

![image](https://github.com/user-attachments/assets/64a06293-dd82-4510-9d7c-38e8c169f70e)

If you are not able to find the actions necessary to make the shortcut, I will also attach a link to copy the shortcut entirely.
Next, create a personal discord server. Nothing fancy needs to happen in it, the default server settings should do. Click on the settings of your "general" channel, go to the integrations tab, and enter the webhooks subtab. From here, create a new webhook, and copy the webhook url (you will need to paste it into the code starting from "/api...")
You are now done with all of the prepwork. Copy all of the variables into the code in their place, upload the code onto your ESP32 (make sure that when you start up the AirTag, your hotspot is already open, also note that YOU MUST TURN ON THE MAXIMISE COMPATABILITY SETTING IN YOUR PHONE HOTSPOT SETTINGS OR THE AIRTAG WILL NOT WORK) and run the shortcut that you created. If everything worked correctly, when connected to a computer, on the serial monitor you should see the GPS coordinates of your device and of your phone, and the distance calculated between them.
On first launch, the GPS module may take time to acquire sattelite information. This usually takes 5-10 minutes on first launch outside, so be patient.

# Endnotes

Thank you for viewing my project. As of now this is a work in progress, meaning while the project itself was submitted (I got a 99.67, no idea where the 0.33 points went) I will continue to work and improve upon this concept. Current priorities are to rid the project of the hotspot requirement and make it compatable with android. Updates will be provided when progress is made. 
