/*******************************************************************************************************
File:      led.h
Author:    Aiko Pras
History:   2022/06/14 AP Version 1.0


Purpose:   Implements the safety decoder specific LEDs
           The LED objects defined in AP_DccLED are relatively expensive regarding SRAM (43 bytes)
           Therefore we create here our own simple On-Off LED class

******************************************************************************************************/
#pragma once
#include <Arduino.h>
#include <AP_DCC_Decoder_Core.h>


class Leds {
  public:
    void init();           // Attach the LEDs to the various pins
    void update();         // Called at the end of the Main loop as frequent as possible

    DccLed safety;
    BasicLed red;
    BasicLed yellow;
    BasicLed green;
};


// The following relay object is defined in led.cpp, and may be used elsewhere
extern Leds leds;
