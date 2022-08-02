/*******************************************************************************************************
File:      dcc_rs.cpp
Author:    Aiko Pras
History:   2022/06/08 AP Version 1.0


Purpose:   Implements the safety decoder specific DCC message reception


******************************************************************************************************/
#include <Arduino.h>
#include <AP_DCC_Decoder_Core.h>      // To include all objects, such as dcc, accCmd, etc.
#include <AP_DCC_Timer.h>             // The DccTimer class is defined there
#include "dcc_rs.h"


// Object instantiation
DccSystem dccSystem;
RsBus rsBus;
extern DccTimer watchdogTimer;       // Is instantiated in safety.cpp


//******************************************************************************************************
bool DccSystem::watchdogMsgReceived() {
  if (!watchdogReceived) return false;
  watchdogReceived = false;
  return true;
}


bool DccSystem::resetMsgReceived() {
   if (!resetReceived) return false;
  resetReceived = false;
  return true; 
}


//******************************************************************************************************
void DccSystem::update() {
  if (dcc.input()) {
    switch (dcc.cmdType) {

      case Dcc::MyAccessoryCmd:
        // A watchdog command has been received.
        // Watchdog Messages should address the first device of the decoder and may be a + or a - command
        // Note that the AP_DCC_Library filters duplicates, thus we need to send alternating + and - commands
        if (accCmd.turnout == 1) {
          watchdogReceived = true;
          watchdogTimer.restart();
        }
      break;
      
      case Dcc::MyPomCmd:
        // A Programming on Main (PoM) message is received      
        cvProgramming.processMessage(Dcc::MyPomCmd);
      break;
      
      case Dcc::SmCmd:
         // A Service Mode (SM - Programming track) message is received      
        cvProgramming.processMessage(Dcc::SmCmd);
      break;
      
      case Dcc::ResetCmd:
      case Dcc::MyEmergencyStopCmd:
        // The ResetCmd is send after the STOP button on the LH100 is pushed, or after TC Einfrieren
        // MyEmergencyStopCmd is never received from a LZV100, but included for possible future versions  
        resetReceived = true;
      break;
      
      case Dcc::SomeLocoMovesFlag:
        // The SomeLocoMovesFlag indicates that a loco speed command with a speed > 0 has been received. 
        // Such flag indicates that at least one train is (still) moving
        // The test if trains are still moving must be initiated (set to false) by the state machine
        trainsMoveFlag = true;
      break;
      
      default:
        // Nothing
      break;
     }
   }
 }
