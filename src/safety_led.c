//*****************************************************************************************************
//
// OpenDCC - OpenDecoder2.2
//
// Copyright (c) 2014 Aiko Pras
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at http://www.gnu.org/licenses/gpl.txt
// 
//
// file:      safety_led.c
// author:    Aiko Pras
// history:   2014-01-12 V0.1 ap initial version
//            2016-01-11 v0.2 ap updated, to limit to the LEDs within the emergency stop buttons
//
//
// purpose:   Routines for the LEDS associated with the "emergency stop" buttons (connector X11)
//
//
//*****************************************************************************************************
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <string.h>

#include "global.h"
#include "config.h"
#include "hardware.h"
#include "led.h"
#include "safety.h"


//*****************************************************************************************************
//***************************************** LED specific code *****************************************
//*****************************************************************************************************
// The Green, Yellow, Red and Extra LEDs have just two modes: ON or OFF
// The Emergency Buttons LEDs can flash (fast), and require therefore the following type definition
struct {
  unsigned char flash_time;   		// LED on/off time for normal flash (in 20 msec ticks)
  unsigned char flash_fast_time;  	// LED on/off time for fast flash (in 20 msec ticks)
  unsigned char rest;     			// Remaining time before LED status changes (in 20 msec ticks)
  unsigned char status;     		// OFF, ON, FLASH or FLASH_FAST
} buttons_led;


//*****************************************************************************************************
// init_safety_leds() is called from init_safety() at startup
//*****************************************************************************************************
void init_safety_leds(void) {
  buttons_led.flash_time = 25;
  buttons_led.flash_fast_time = 7;
  buttons_led.rest = 0;
  buttons_led.status = OFF;
}


//*****************************************************************************************************
// led_buttons() is called from the state machine in safety.c
//*****************************************************************************************************
// the next function will be called from run_state_machine() if needed
void led_buttons(unsigned char value){
  if (value == OFF) {
	LED_BUTTONS_OFF;
	buttons_led.status = OFF;
  }
  if (value == ON) {
	LED_BUTTONS_ON;
	buttons_led.status = ON;
  }
  if (value == FLASH) {
	LED_BUTTONS_ON;
	buttons_led.status = FLASH;
	buttons_led.rest = buttons_led.flash_time;
  }
  if (value == FLASH_FAST) {
	LED_BUTTONS_ON;
	buttons_led.status = FLASH_FAST;
	buttons_led.rest = buttons_led.flash_fast_time;
  }
}


// the next function will be called from check_safety_functions() every 20 seconds
void check_safety_leds_time_out(void) {
  // LEDs within the emergency push buttons
  if (buttons_led.status == FLASH) {
	if (buttons_led.rest > 0) {buttons_led.rest -- ;}	// decrease the timer
	else {												// timer has expired
      buttons_led.rest = buttons_led.flash_time;  		// Re-initialise the timer
      LED_BUTTONS_TOGLE;
      // if (LED_BUTTONS_STATE) {LED_BUTTONS_OFF;}		// LEDs are ON (AVR Pin for LED is high) -> switch off
      // else {LED_BUTTONS_ON;}		   					// LEDs are OFF (AVR Pin for LED is low) -> switch on
	}
  }
  if (buttons_led.status == FLASH_FAST) {
	if (buttons_led.rest > 0) {buttons_led.rest -- ;}	// decrease the timer
	else {						// timer has expired
      buttons_led.rest = buttons_led.flash_fast_time;  	// Re-initialise the timer
      LED_BUTTONS_TOGLE;
      //if (LED_BUTTONS_STATE) {LED_BUTTONS_OFF;}		// LEDs are ON (AVR Pin for LED is high) -> switch off
      //else {LED_BUTTONS_ON;}		   					// LEDs are OFF (AVR Pin for LED is low) -> switch on
	}
  }
}

