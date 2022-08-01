# Description #

The safety decoder performs two functions:
- Watchdog function: to check if the PC with train control software still sends accessory commands.
- Emergency stop: make sure all trains stop in case one of the emergency stop buttons is pushed.


### 1) Watchdog function ###
The goal of the watchdog function is to check the operation of the PC (program). For that purpose the PC program periodically sends a DCC Accessory message to the safety decoder.
The safety decoder has a relay that connects the (C) signal of the command station (such as a LENZ LZV100) to the (LV100) booster(s). If the safety decoder does not receive a new watchdog command every few seconds (default is 5 seconds), the relay will be released and the DCC signal to all boosters gets interrupted. As a consequence, all trains will be stopped.
This functions is comparable to the WD-DEC decoder from LDT.

At start-up the watchdog function will be inactive and the yellow LED is ON, to indicate that the state is **LOCAL**. The relay is switched on ON, and connect the (C) signal from the DCC Command Station to the booster(s) to allow operation without PC.
If the safety decoder receives a watchdog command from the PC, the watchdog function will be activated and the green LED will light to signal that the state has changed to **REMOTE**. Watchdog commands are accessory commands addressed to the first device of this decoder, and alternate between **+** and **-**

In the **REMOTE** state the safety decoder should receive new watchdog messages every few seconds. If a new watchdog command is not received before the watchdog time-out period is over, the decoder switches to the **W_STOP** state. There are multiple causes to explain the absence of new watchdog messages:
1. The computer (programma) crashed, and trains are still moving without computer control.
2. The user pushed the HALT button of the handheld, or the "Einfrieren" button of the TrainController program.
3. The user brought the TrainController program to a controlled stop. In that case all trains should have been gracefully stopped by the computer program.

To determine what caused the absence of watchdog messages, the watchdog function monitors if there a still trains moving (speed > 0).

1. If trains are still moving, the computer (program) must have been crashed and the safety decoder needs to act. The decoder will release the relay to interrupt the DCC signal that flows from master station to booster(s). This removes the DCC signal from the tracks, and stops all trains. The decoder enters the **W_RELAY_OFF** state, and turns the red LED ON and makes the button LEDs flash fast. The user must acknowledge that he  takes over control by pushing one of the emergency buttons. After such button is pushed, the  decoder will return to the **START-UP** state.

2. In case the user pushed the HALT button of the handheld, or the "Einfrieren" button of the TrainController program, the DCC master station will start transmitting DCC RESET packets. Such packets tell all trains that they must stop moving. The safety decoder detects such RESET packets, and knows that the user has taken over control and the decoder should switch back to the **STARTUP** / **LOCAL** state.

3. In case the user stopped the TrainController program, all trains should have been stopped. To detect if this is indeed the case, the decoder continuously checks if trains are still moving. If no moving trains are seen during a *checkMove* interval, the decoder knows that the user has brought all trains to a controlled stop. The decoder will now return to the **START_UP** state.


### 2) Emergency stop function ###
The safety decoder also monitors if one of the emergency stop buttons is pushed. Depending on the state the decoder is in, the decoder reacts as follows:

1. If the state is **LOCAL** (= watchdog function is inactive), the decoder releases the watchdog relay. The DCC signal that flows from master station to booster(s) gets interrupted, and the tracks no longer send a DCC signal. All trains are stopped, and the decoder enters the **L_PUSHED** state. After the user pushes the emergency button a second time, the  decoder returns to the **START-UP** state. The relay gets activated and the tracks will receive again the DCC signal

2. If the state is **REMOTE** (= watchdog function is active), the decoder sends an RS-Bus feedback message (Emergency bit is on) to the (TrainController) computer program. The computer program should stop all trains as soon as possible, although this may take a short time to accomplish. The decoder  therefore starts the *emergencyExpired* timer (default 2 seconds) and enters the **PC_WAIT** state. After the timer expires, the decoder moves to the **R_STOP** state and checks if the computer has indeed managed to stop all trains.  

   If trains have not been stopped by the computer, the decoder releases the relay to ensure the DCC signal gets removed from the tracks and all trains will be stopped. The decoder enters the **R_RELAY_OFF** state and waits till the user acknowledges by pushing the emergency button again that he/she has taken over control. The decoder now moves back to the **START_UP** / **LOCAL** state.

   If trains have been stopped by the computer, the decoder waits a while to allow human intervention. If, after this period is over, trains start moving again, the decoder moves back to the **START_UP** / **LOCAL** state.

### DCC and RS-Bus addresses ###
The DCC address can be set using the onboard button, like all other decoder boards. Once an Accessory address is entered, it will be stored in the respective CVs. However, via the onboard button we can not set the RS-bus address. By default that address is 127. In case a change is needed, POM messages should be used to change the CV for the RS-Bus address (CV10)

### LEDs ###

Meaning of LEDs connected to the X10 connector of the safety decoder
- Yellow LED: Local control: watchdog function is not active
- Green  LED: Remote control by the computer: watchdog function is active
- Red    LED: watchdog relay is released
- Extra  LED: no special meaning

### Hardware ###
The safety decoder hardware can be found on https://oshwlab.com/aikopras/watchdog-decoder
