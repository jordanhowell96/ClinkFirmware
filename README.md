# ClinkFirmware

Microcontroller Firmware which is part of an ongoing side project to automate PC wake-up and game launching from third-party controllers through Bluetooth.

This is to make a PC behave similar to how a game console behaves where a user can turn on the device and start a game all through the controller. 

## How it works

Windows Bluetooth does not work when the computer is asleep, so an ESP32 is used to recieve Bluetooth signals and the ProMicro is used to wake the computer by emulating a keyboard.

Base functionality:

* The desktop application (in a different repo) will send a the MAC address of the controller over Serial to the USB connected ProMicro
* The ProMicro will pass the MAC address to the ESP32
* The current state of the PC (awake, asleep, or unlocked) is communicated from the desktop app to the ProMicro via Serial
* The ESP32 will scan local Bluetooth signals, searching for the MAC address of the controller
* Once the controller's MAC address is detected, the ESP32 will send a Serial signal to the ProMicro 
* The ProMicro then emulates a keyboard to wake and unlock the PC and finally send a Serial signal to the PC to indicate that the controller was detected
  * Steps above may be skipped depending on the state of the PC. For example, if the PC is already awake then only the signal that the controller was detected will be sent
* The desktop application will then launch Steam in big picture mode (in a different repo)
* The user can now use the controller to operate the PC like a console

All communication features ACK and retry logic.

## Where I'm at

The system currently has most of the desired base functionality, but right now I am working on organizing and refactoring the code as well and improving reliability of the communication between the devices.
