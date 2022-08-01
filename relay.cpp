/*******************************************************************************************************
File:      relay.cpp
Author:    Aiko Pras
History:   2022/06/14 AP Version 1.0


Purpose:   Implements the safety decoder specific relay

******************************************************************************************************/
// There is one relay that controls the signal between the LZV 100 and boosters. 
// The relay gets activated at startup. As a result, the signal between LZV 100 and boosters can flow
// as if there was no safety decoder. In case of a watchdog time-out or emergency button push
// (in the local state) the relay will be switched off and the signal between LZV 100 and Boosters
// will be interrupted. As a result, power on the tracks will be removed.
#include <Arduino.h>
#include "relay.h"
#include "hardware.h"

// The relay object is instantiated here
Relay relay;


void Relay::init() {
  pinMode(WATCHDOG_RELAY, OUTPUT);
  digitalWrite(WATCHDOG_RELAY, LOW);
}

void Relay::turn_on() {
  digitalWrite(WATCHDOG_RELAY, HIGH);
}

void Relay::turn_off() {
  digitalWrite(WATCHDOG_RELAY, LOW);
}
