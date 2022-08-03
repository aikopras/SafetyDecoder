/*******************************************************************************************************
File:      button.cpp
Author:    Aiko Pras
History:   2022/06/14 AP Version 1.0


Purpose:   Implements the safety decoder buttons

******************************************************************************************************/
//
//                         wasPressed()                         2 x wasPressed()
//                         v                                     v           v
//
//               +---------+                                 +---+    +------+
// Push button   |         |                                 |   |    |      |
//           ----+         +---------------------------------+   +----+      +-------------------------
//
//                         +------------------+                  +--------------------------------+
// RS-bit                  |                  |                  |                                |
//           --------------+                  +------------------+                                +----
//                         <----- onTime ----->                  <------ onTime ...
//                                                                           <------ onTime ------>

//
//
// Toggle button behaviour
// =======================
//
//               +---------+                                 +---+    +------+
// Toggle button |         |                                 |   |    |      |
//           ----+         +---------------------------------+   +----+      +-------------------------
//
//                         +-------------------------------------+                 
// RS-bit                  |                                     |                  
//           --------------+                                     +-------------------------------------
//                         <------ 1000ms ------>                <------ 1000ms ------>
//
// ****************************************************************************************************

#include <Arduino.h>
#include <AP_DCC_Decoder_Core.h>      // To read CV values
#include "button.h"
#include "hardware.h"
#include "dcc_rs.h"                   // The RS-Bus object is defined and instantiated here


// The `buttons' object supports a total number of `BUTTONS_USED' individual buttons
Buttons buttons;



// ******************************************************************************************************
// Public methods: Called by setup() and the main loop
// ******************************************************************************************************
void Buttons::init() {
  // Step 1: Determine which button is used for the emergency stop button. 
  // P_Emergency is a CV and has a value between 1 and 4
  emergencyPin = cvValues.defaults[P_Emergency] - 1;
  for (uint8_t i = 0; i < BUTTONS_USED; i++) {
    // Step 1: attach the buttons
    button[i].attach(FIRST_BUTTON + i, DEBOUNCETIME, PULLUP_ENABLE, INVERT);    
    // Step 2: read the T_RS_Push CVs
    // T_RS_Push is defined in 20ms steps. The value 0 represents toggle behaviour
    button[i].timer.runTime = cvValues.defaults[T_RS_Push1 + i] * 20;
    // Step 3: Determine if a button is a Push or Toggle button
    if (button[i].timer.runTime == 0) button[i].type = TOGGLE;
    else button[i].type = PUSH;
    // Step 4: Determine which input pin is for the emergency stop button. 
    if (i == emergencyPin) button[i].isEmergency = true;
    // Step 5: Set the RSbus bit that will be used for this pin.
    // Button[0] maps to RSbit 4, button[3] to RSbit 7
    button[i].rsBit = 4 + i; 
  }
  // The feedbackIsRequested boolean may be set by each individual button
  feedbackIsRequested = false;
}


bool Buttons::emergencyPushed() {
  if (!emergencyFlag) return false;
  emergencyFlag = false;
  return true;
}


void Buttons::update() {
  // Pushing the emergency button results in setting a flag, which is read by the statemachine via emergencyPushed()
  // Pushing any other button will request the sending of a RS-Bus feedback message
  // In case of a push button, a second RS-Bus feedback message (bit is cleared) will be send after a time-out
  // In step 1 we check for each button if it was pressed; in step 2 we send (if requested) the RS-Bus message 
  // Step 1: Check each individual button
  for (uint8_t i = 0; i < BUTTONS_USED; i++) {
    // Step 1A: has this button been pressed?
    button[i].read();
    if (button[i].wasPressed()) {
      if (button[i].isEmergency) {
        emergencyFlag = true;   // sending possible RS-Bus messages is handled by the state machine
      }
      else {
        // A normal button was pressed. Was it a push button or a toggle button?
        if (button[i].type == PUSH) pushEvent(i);
        else toggleEvent(i);
      }
    }
    // Step 1B: has this push button timer expired? Clear the corresponding bit and send a RS-Bus message
    // Note that we clear the bit also if this is the emergency button (configured as push button)
    if ((button[i].type == PUSH) && button[i].timer.expired()) {
      bitClear(rsBus.feedbackData, button[i].rsBit);
      feedbackIsRequested = true;
    }
  }
  // Step 2: Were one of more buttons pushed that require sending a RS-Bus message?
  // Only send RS-Bus messaages if also requested by the corresponding CV
  if (feedbackIsRequested && cvValues.defaults[SendFB]) {
    // Button data is stored in RS-Bus bits 5..8 
    uint8_t nibble2 = (rsBus.feedbackData & 0xF0) >> 4;
    rsBus.send4bits(HighBits, nibble2);
    feedbackIsRequested = false;
  }
}


// ******************************************************************************************************
// Public methods: the state mechine calls sendRsEmergencyBit() and clearRsEmergencyBit()
// ******************************************************************************************************
void Buttons::sendRsEmergencyBit() {
  // Send a RS-Bus message and set the timer for later release
  bitSet(rsBus.feedbackData, button[emergencyPin].rsBit);
  feedbackIsRequested = true;
  button[emergencyPin].timer.start();          // (re)start the timer   
}


void Buttons::clearRsEmergencyBit() {
  bitClear(rsBus.feedbackData, button[emergencyPin].rsBit);
  feedbackIsRequested = true;
}


// ******************************************************************************************************
// Private methods - See figures on top of this file
// ******************************************************************************************************
void Buttons::pushEvent(uint8_t i) {
  // Send a RS-Bus message and set the timer for later release
  // To avoid duplicating RS-Bus messages, check if the previous one is still active   
  if (button[i].timer.running() == false) {
    bitSet(rsBus.feedbackData, button[i].rsBit);      // Set the corrsponding feedback bit
    feedbackIsRequested = true;
  };
  button[i].timer.start();          // (re)start the timer, even if is was still running    
}


void Buttons::toggleEvent(uint8_t i) {
  // Only react if the toggle button was not pushed just before 
  if (button[i].timer.running() == false) {
    rsBus.feedbackData ^= 1 << button[i].rsBit;       // Flip the corrsponding feedback bit 
    button[i].timer.setTime(1000);                    // Start the timer, to ignore further input for 1000 ms    
    feedbackIsRequested = true;
  }
}


 
