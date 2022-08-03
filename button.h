/*******************************************************************************************************
File:      button.h
Author:    Aiko Pras
History:   2022/06/14 AP Version 1.0


Purpose:   Implements the safety decoder buttons

******************************************************************************************************/
// Four buttons may be connected to the safety board, one of these becomes the emergency stop button.
// Which of these will act as emergency stop button, is determined by the P-Emergency CV.
//
// Normal buttons
// ==============
// Once a normal button, thus a button that is NOT the emergency stop button, is pushed, the 
// associated RS-Bus message will be send from the code here.
// In case that button is configured as a PUSH button, the associated RS-bus bit will be cleared  
// after the time-out that is determined by the T_RS_Push CV.
// If the value of the T_RS_Push CV is 0, the button will act as a toggle button: the associated
// RS-bus bit will be cleared after the button is pushed again. After pushing a toggle button, all 
// further pushes within the next second will be ignored.
//
// Emergency stop button
// =====================
// The emergency button has non-standard button behaviour. In some cases it should trigger a RS-bus
// message (and possibly release the relay), in other cases it should not trigger the sending of a
// RS-bus message but just allow us to move back to the STARTUP state (where the relay will be set).
// Therefore this part of the code only sets emergencyPushed(), and all further actions related
// to the emergency button are handled from within the state machine.
//
// ******************************************************************************************************
#pragma once
#include <Arduino.h>
#include <AP_DCC_Decoder_Core.h>
#include <AP_DCC_Timer.h>
#include "hardware.h"


class Buttons {
  public:
    void init(void);                     // Called from init() in the main sketch
    void update(void);                   // Called from the main loop
    void sendRsEmergencyBit(void);       // Called from the state machine
    void clearRsEmergencyBit(void);      // Called from the state machine
    bool emergencyPushed(void);          // Checked by the state machine
    uint8_t emergencyPin;

  private:
    // The Button class is inherited from the DccButton class, but has some extra parameters
    const uint8_t TOGGLE = 0;            // for readability
    const uint8_t PUSH = 1;
    bool emergencyFlag;                  // Internal boolean, checked and cleared by emergencyPushed()
    class Button: public DccButton {
      public:
        bool isEmergency = false;
        bool type;
        uint8_t rsBit;
        DccTimer timer;
      };
    Button button[BUTTONS_USED];
    bool feedbackIsRequested;
    void pushEvent(uint8_t i);
    void toggleEvent(uint8_t i);
};


extern Buttons buttons;
