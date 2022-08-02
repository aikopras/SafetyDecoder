/*******************************************************************************************************
File:      safety.cpp
Author:    Aiko Pras
History:   2014-01-12 V0.1 ap initial version
           2016-01-11 V0.2 ap Changed to reflect changes in states
           2022-06-13 v1.0 ap major rewrite, to make it Arduino compatible

Purpose:   Implements the statemachine that controls operation of the safety decoder 
           For a complete flow-diagram, see the files "safety-Local.pdf", "safety-remote.pdf"
           and "safety-remote-button.pdf" in the "extras" folder
           
******************************************************************************************************/
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <AP_DCC_Decoder_Core.h>     // all generic objects are instantiated in here
#include "safety.h"
#include "button.h"
#include "dcc_rs.h"
#include "hardware.h"
#include "led.h"
#include "relay.h"


// ******************************************************************************************************
// There are three timers:
// 1 - watchdogTimer
// Timer to check if DCC Watschdog messages, that are send from the PC to the safety decoder, arrive regularly.
// If the timer expires, two possible causes exist:
// a) the user has gracefully stopped the PC programme. 
// b) an error occured, and no guarantees exist that the PC programme still controls the system
//    In this latter case the safety decoder takes over, and forces a DCC halt (by releasing the relay).
// The default value (5) is retrieved during setup from the T_Watchdog CV (the value is in seconds)
//
// 2 - emergencyTimer
// Time that the PC is given to stop all trains, after the RS-emergency button is pushed and the decoder
// has send an (RS-Bus) emergency stop message to the PC.
// The default value (20) is retrieved during setup from the T_Emergency CV (the value is in 100ms steps)
 
// 3 - checkMoveTimer
// If the user has told the computer to stop all trains, or if the safety decoder has told the computer to
// stop all trains (since the emergency button was pushed), it may still take some time before the computer
// has managed to stop 
// ******************************************************************************************************
// ************************************************* Objects ********************************************
StateMachine stateMachine;
DccTimer watchdogTimer;
DccTimer emergencyTimer;
DccTimer checkMoveTimer;


// *****************************************************************************************************
#ifdef LCD_OUTPUT
LiquidCrystal lcd(RS, RW, ENABLE, D4, D5, D6, D7);
void StateMachine::showState() {
  lcd.clear();
  lcd.print("State: ");
  switch (state) {
    case LOCAL:       lcd.print("LOCAL");       break;
    case L_PUSHED:    lcd.print("L_PUSHED");    break;
    case REMOTE:      lcd.print("REMOTE");      break;
    case W_STOP:      lcd.print("W_STOP");      break;
    case W_RELAY_OFF: lcd.print("W_RELAY_OFF"); break;
    case PC_WAIT:     lcd.print("PC_WAIT");     break;
    case R_STOP:      lcd.print("R_STOP");      break;
    case R_RELAY_OFF: lcd.print("R_RELAY_OFF"); break;
    case R_STOPPED:   lcd.print("R_STOPPED");   break;
    default: break;
  }
  lcd.setCursor(0, 1);
  if (state != STARTUP) counter++;
  lcd.print(counter);
}
#endif


void StateMachine::sendState() {
  // State data is stored in RS-Bus bits 1..4 (nibble1)
  bool feedbackIsRequested = false;
  uint8_t nibble1 = 0;
  switch (state) {
    case LOCAL:
      nibble1 = 1;  // RS-Bus bit 1 
      feedbackIsRequested = true;         
    break;
    case REMOTE:
      nibble1 = 2;  // RS-Bus bit 2
      feedbackIsRequested = true;         
    break;
    case L_PUSHED:         
      nibble1 = 4;  // RS-Bus bit 3
      feedbackIsRequested = true;         
    break;
    case R_RELAY_OFF:         
      nibble1 = 8;  // RS-Bus bit 4
      feedbackIsRequested = true;         
    break;
    default:
    break;
  }
  if (feedbackIsRequested && cvValues.defaults[SendFB]) {
    // A next state is entered for which a RS-Bus message should be send
    // The SendFB CVs indicates that feedback messages are requested
    // Check if the new nibble is indeed different from the old one
     if (nibble1 != (rsBus.feedbackData & 0x0F)) {
       // Update the 4 lower bits of the feedback data 
       rsBus.feedbackData = (rsBus.feedbackData & 0xF0) + nibble1;
       rsBus.send4bits(LowBits, nibble1);
       feedbackIsRequested = false;
     }
  }
}


void StateMachine::nextState(states next) {
  if (next != state) {
    state = next;
    sendState();        // Send for selected states an RS-Bus message
    #ifdef LCD_OUTPUT
    showState();        // Show state information on the LCD screen
    #endif
  }
}


// *****************************************************************************************************
// init is called from main at startup
// *****************************************************************************************************
void StateMachine::init(void) {
  // Initialise the timeout values from the respective CVs
  // Note that T_Watchdog has a resolution of 1 second, whereas the others have 100ms steps
  watchdogTimer.runTime  = cvValues.read(T_Watchdog) * 1000;
  emergencyTimer.runTime = cvValues.read(T_Emergency) * 100;
  checkMoveTimer.runTime = cvValues.read(T_CheckMove) * 100;
  // 
  nextState(STARTUP);
}


// *****************************************************************************************************
// Run the state machine.
// For details, see: "Safety-Local", "Safety-Remote" and "Safety-Remote-button" in the extras folder  
// *****************************************************************************************************
void StateMachine::run(void) {
  // Step 1: Handle all (push and toggle) buttons, including the emergency stop button
  buttons.update();

  // Step 2: Did we receive a Reset (NotHalt) message?
  // In that case we immediately  move back to the (STARTUP and) LOCAL state 
  // No need to perform any action, since the computer / handheld seem to be in control
  if (dccSystem.resetMsgReceived()) {
      if ((state != STARTUP) && (state != LOCAL) && (state != W_RELAY_OFF) && (state != R_RELAY_OFF)) nextState(STARTUP);
  }

  // Step 3: Run the state machine
  switch (state) {
    // 1) see Safety-Local
    // ===================
    case STARTUP:         
      leds.yellow.turn_on();
      leds.green.turn_off();
      leds.red.turn_off();
      leds.safety.turn_on(); 
      relay.turn_on();
      buttons.clearRsEmergencyBit();
      nextState(LOCAL);
    break;
    case LOCAL:
      if (dccSystem.watchdogMsgReceived()) {
        leds.yellow.turn_off();
        leds.green.turn_on();
        nextState(REMOTE);
      }
      else if (buttons.emergencyPushed()) {
        relay.turn_off() ;
        leds.red.turn_on();
        leds.safety.flashSlow(); 
        buttons.sendRsEmergencyBit();
        nextState(L_PUSHED);
      }
    break;
    case L_PUSHED:
      if (buttons.emergencyPushed()) {
        nextState(STARTUP);
      }
    break;
    // 2) see Safety-Remote
    // ====================
    case REMOTE:          
      if (dccSystem.watchdogMsgReceived()) {
        // Everything OK, do nothing
      }
      else if (watchdogTimer.expired()) {
        leds.green.turn_off();
        leds.red.turn_on();
        checkMoveTimer.start();
        dccSystem.trainsMoveFlag = false;
        nextState(W_STOP);
      }
      else if (buttons.emergencyPushed()) {
        leds.safety.flashSlow(); 
        buttons.sendRsEmergencyBit();
        emergencyTimer.start();
        nextState(PC_WAIT);
      }
    break;
    case  W_STOP:
      if (dccSystem.trainsMoveFlag) {
        relay.turn_off();
        leds.safety.flashFast(); 
        nextState(W_RELAY_OFF);
      }
      else if (checkMoveTimer.expired()) {
        nextState(STARTUP);
      }
    break;
    case  W_RELAY_OFF:
      if (buttons.emergencyPushed()) {
        nextState(STARTUP);
      } 
    break;
    case  PC_WAIT:
      if (emergencyTimer.expired()) {
        leds.green.turn_off();
        leds.red.turn_on();
        checkMoveTimer.start();
        dccSystem.trainsMoveFlag = false;
        nextState(R_STOP);
      }
    break;
    // 3) see Safety-Remote_button
    // ===========================
    case  R_STOP:
      if (dccSystem.trainsMoveFlag) {
        relay.turn_off();
        leds.safety.flashFast(); 
        nextState(R_RELAY_OFF);
      }
      else if (checkMoveTimer.expired()) {
        nextState(R_STOPPED);
      }
    break;
    case  R_STOPPED:
      if (buttons.emergencyPushed()) {
        nextState(STARTUP);
      }
      else if (dccSystem.trainsMoveFlag) {
        nextState(STARTUP);
      }
    break;
    case  R_RELAY_OFF:
      if (buttons.emergencyPushed()) {
        nextState(STARTUP);
      }
    break;
  }
}
