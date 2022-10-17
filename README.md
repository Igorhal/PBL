# PBL
Files:
<br />
  a) pico (Raspberry Pi Pico):
    To program this device in Arduino IDE, you need to add dependency URL to your program (Preferences -> Additional Board Manager URL): 'https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json', and then you need to add Raspberry Pi Pico board in Board Menager and select it. Then you need to press 'BUTSEL' button on the device, connect USB cable and upload program to the device (without COM port), while pressing the button. After successful upload you can reconnect the cable, the device should be automaticly detected by your PC.
 <br />
 b) mega (Arduino Mega 2560):
    If you PC doesn't detect this device, you need to install CH340 driver. ArdunioJSON library is required, you can install it via Liblary Manager (Tools -> Library Manager -> search for 'ArdunioJSON').  
 <br />
  c) panda (Lattepanda)
    Admin password: admin22.
