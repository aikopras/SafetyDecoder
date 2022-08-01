/*******************************************************************************************************
File:      safety.h
Author:    Aiko Pras
History:   2014-01-12 V0.1 ap initial version
           2016-01-11 V0.2 ap Changed to reflect changes in states
           2022-06-13 v1.0 ap major rewrite, to make it Arduino compatible


Purpose:   Implements the statemachine that controls operation of the safety decoder 
           For a complete flow-diagram, see the files "safety-Local.pdf", "safety-remote.pdf"
           and "safety-remote-button.pdf" in the "extras" folder
           
******************************************************************************************************/
#pragma once
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <AP_DCC_Timer.h>


class StateMachine {
  public:
    void init();     // called once at initialisation
    void run();      // called from main (every ??? mseconds)

  private:  
    enum states {
      STARTUP = 1,       // Initialising
      LOCAL = 2,         // No active computer program like TC or Railware to control trains
      L_PUSHED = 3,      // Button pushed in LOCAL  state. Relay has been switched off
      REMOTE = 4,        // Under watchdog control. Computer sends Watchdog messages
      W_STOP = 5,        // Watchdog timer expired. Check if trains are still running
      W_RELAY_OFF = 6,   // Watchdog timer expired and trains were running. Relay is OFF
      PC_WAIT = 7,       // Button pushed in REMOTE state. Wait if computer stops all trains
      R_STOP = 8,        // Check if computer has gracefully stopped all trains
      R_RELAY_OFF = 9,   // Computer did not gracefully stop all trains. Relay switched off
      R_STOPPED = 10     // Computer gracefully stopped all trains.
    } state;
    int counter = 0; // Temporary test variable to count the number of state changes
    void nextState(states state);
    void sendState();
    void showState();
};


// The following objects are instantiated in safety.cpp but must be accessable elsewhere
extern StateMachine stateMachine;              // used by the main loop()
extern DccTimer watchdogTimer;                 // set in dcc.cpp
extern LiquidCrystal lcd;                      // used by the main loop()
