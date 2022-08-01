/*******************************************************************************************************
File:      led.cpp
Author:    Aiko Pras
History:   2022/06/14 AP Version 1.0


Purpose:   Implements the safety decoder specific LEDs

******************************************************************************************************/
#include <Arduino.h>
#include "led.h"
#include "hardware.h"


// A number of LEDs are connected to the safety decoder. Their objects are instantiated here.
Leds leds;


void Leds::init() {
  safety.attach(LEDS_BLINKING);
  red.attach(LED_RED);
  yellow.attach(LED_YELLOW);
  green.attach(LED_GREEN);
}

void Leds::update() {
  safety.update();
}
