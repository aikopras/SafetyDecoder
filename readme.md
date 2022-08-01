# Safety Decoder #

### Basic operation ###
The safety decoder includes a watchdog function and the ability to inform a DCC train controller program that an emergency stop button is pushed.

The goal of the watchdog function is to check the operation of the PC (program). For that purpose the PC program should periodically (like every 3 seconds) send a DCC Accessory message to the safety decoder.
If such accessory message has not been received, the safety decoder assumes the computer (program) has crashed and a relay is activated that interrupts the signal between command station and booster(s). As a consequence, track power will be removed and all trains are stopped. This functions is comparable to the WD-DEC decoder from LDT.

Whereas the watchdog function uses a rather *brute-force* method to stop trains (by removing power from the tracks), the safety decoder has also the ability to stop trains in a more *graceful* way by informing the train controller program via RS-Bus messages that the user pushed an emergency stop button. The train controller program has now the opportunity to bring all trains to a controlled stop, for example by sending an XpressNet stop all locomotives request / emergency stop command. The safety decoder will now monitor if the computer program has indeed ordered all trains to stop; if trains are still moving the safety decoder will activate the relay to force a *brute-force* stop.

### Compatibel DCC systems ###
The safety decoder was specifically designed for a LENZ (LZV100 / LV102). The decoder is able to send RS-Bus messages, and the relay can  connect the LZV100 master station to the LV102 boosters.

The watchdog function of the safety decoder can be used with all DCC systems that allow interruption of the signal between command station and booster. It *might* also work with integrated systems, provided such a system has an external emergency stop connector, to which the relay can be connected.

### Hardware ###
The software runs on the safety decoder board, as can be found on: https://oshwlab.com/aikopras/watchdog-decoder. The board design is open source, and can be copied using the (free / web-based) EasyEda PCB design program. After loading it into EasyEda, it can be ordered at JLCPCB for a few Euro's.

### Software ###
The safety decoder software is written for the Arduino IDE. The software requires the use of a number of libraries:
- RSBus: https://github.com/aikopras/RSbus
- AP_DCC_library: https://github.com/aikopras/AP_DCC_library
- TO BE COMPLETED
