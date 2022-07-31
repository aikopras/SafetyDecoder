/*******************************************************************************************************
File:      relay.h
Author:    Aiko Pras
History:   2022/06/14 AP Version 1.0


Purpose:   Implements the safety decoder specific relay

******************************************************************************************************/
#pragma once
#include <Arduino.h>


class Relay {
  public:
    void init();             // Attach the Relay to the correct pin
    void turn_on();
    void turn_off();
};


// The following relay object gets instantiated in relay.cpp, and may be used elsewhere
extern Relay relay;
