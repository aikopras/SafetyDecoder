# Safety (watchdog) decoder

Safety (including watchdog) decoder for the DCC (Digital Command Control) system with feedback via the (Lenz) RS-Bus.
The  decoder performs two functions: 
1. Watchdog function: to check if the PC with train control software still sends commands
2. Emergency stop: make sure all trains stop in case one of the emergency stop buttons is pushed

## Watchdog function:
The watchdog function controls a relay connected to the DCC control system (such as LENZ LZV100).
If the safety decoder does not receive a "switch command" within a few seconds (default is 5 seconds),
the relay will be released and the DCC control system immediately stops all trains. 
This functions is comparable to the WD-DEC decoder from LDT.

At start-up the watchdog function will be inactive and the yellow LED is on to indicate that the state is LOCAL. 
The relay will be ON, however, to allow operation without PC.
If the safety decoder receives a watchdog related accessory command from the PC, the watchdog function will be activated and the green LED will light to signal that the state has changed to REMOTE.
If a new watchdog related accessory command is not received before the watchdog period is over and trains are still running, the relay will be released and the DCC control system will stop all trains.
The state will become W_RELAY_OFF, the red LED turns on and the button light starts flashing fast.

 Note that trains can also be stopped if the user pushes the HALT button of the handheld or the "Einfrieren" button within the TrainController program. In that case DCC RESET packets will be send to the DCC control system and trains will be automatically stopped. In such case this program goes back to the STARTUP phase.

## Emergency stop function:
The safety decoder will also monitor if one of the emergency stop buttons is pushed. If such button is pushed, the decoder will generate an RS-bus feedback message and will react in one of the following ways:
1. If the state is [LOCAL](documentation/safety-local.pdf) (= watchdog function is inactive), it will deactivate the watchdog relay
2. If the state is [REMOTE](documentation/safety-remote.pdf) (= watchdog function is active), it will start a short timer (default 2 seconds) after which it checks if trains have been stopped by the computer or not.
    1. If trains have not been stopped by the computer, the relay will be released to ensure the DCC control system will stop all trains
    2. If trains have been stopped by the computer, we wait a while to allow human intervention. If, after this period is over, trains start moving again, we move back to the LOCAL state.

Meaning of LEDs cpnnected to the X10 connector of the safety decoder:
* Yellow LED: Local control: watchdog function is not active,
* Green LED: Remote control by the computer: watchdog function is active,
* Red LED: watchdog relay is released.

## Hardware
A description of this decoder and related decoders can be found on [https://sites.google.com/site/dcctrains](https://sites.google.com/site/dcctrains).
The hardware and schematics can be downloaded from my [EasyEda page](https://easyeda.com/aikopras/watchdog-decoder),


## Compiling and Flashing
The software is written in C and runs on ATMEGA16A and similar processors (32A, 164A, 324A, 644P). 
It is an extension of the [Opendecoder](https://www.opendcc.de/index_e.html) project, and written in "pre-Arduino times". Therefore compilation is based on a traditional [makefile](src/Makefile), and assumes availability of gcc, [avrdude](https://www.nongnu.org/avrdude/) as well as a USBasp programmer.
Note however that the program can also be compiled and flashed via the Arduino IDE. Instructions for using the Arduino IDE can be found in the [<b>Arduino-SAFETY.ino</b> file](/src/Arduino-SAFETY.ino). Note that you have to rename the /src directory into "Arduino-SAFETY" before you open the .ino file. 
