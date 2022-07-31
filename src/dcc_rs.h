/*******************************************************************************************************
File:      dcc_rs.h
Author:    Aiko Pras
History:   2022/06/13 AP Version 1.0


Purpose:   Implements the safety decoder specific DCC message reception and the RS-Bus interface
           Note that all DCC pins are defined and initialised in the AP_DCC_Decoder_Basic library

Each RS-Bus message contians four bits (a nibble).
The first nibble contains the current state, the second tells which button was pushed.
Note that bit numbering on the LH-100 handheld is 1 higher

      nibble 2          nibble 1
  +----------------+----------------+
  | 7   6   5   4  |  3   2   1   0 |
  +----------------+----------------+
       button            state

  Meaning of the various bits:
  0: state == LOCAL
  1: state == REMOTE
  2: state == L_PUSHED
  3: state == R_RELAY_OFF

  4: Button on PIN_PC4 / Connector 8.1
  5: Button on PIN_PC5 / Connector 8.2
  6: Button on PIN_PC6 / Connector 8.4
  7: Emergency button pushed (PIN_PC7 / Connector 8.4)

******************************************************************************************************/
#pragma once
#include <Arduino.h>                  // For general definitions
#include <AP_DCC_Decoder_Basic.h>     // To include all objects, such as dcc, accCmd, etc.


class DccSystem {
  public:
    void update();                    // Called at the end of the Main loop as frequent as possible
    bool watchdogMsgReceived();       // We use boolean functions for these two flags, to avoid the 
    bool resetMsgReceived();          // need to clear the flags from the state machine
    bool trainsMoveFlag;              // This must remain a flag
    
  private:
    bool watchdogReceived;
    bool resetReceived;
};


// The RsBus class is inherited from the RSbusConnection class
class RsBus: public RSbusConnection {
  public:
    uint8_t feedbackData = 0;
};


extern DccSystem dccSystem;
extern RsBus rsBus;                            // used by the main loop()
