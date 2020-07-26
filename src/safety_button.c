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
// file:      safety_button.c
// author:    Aiko Pras
// history:   2014-02-05 V0.1 ap initial version
//            2016-01-12 V0.2 ap use CVs to set the PUSH, TOGGLE and Emergency stop buttons
// Called:    from safety.c
//            - init_safety_buttons(): during power-on or reset
//            - handdle_buttons(): every 20ms by check_safety_functions()
//            - emergency_button_pushed(): by the state machine
// Calling:   None
// Sharing:   The button input[] array is shared with safety_feedback.c
//
//
// purpose:   Routines for the OPTO-IN connector (X8) on the safety decoder
//            Configuration Variables (CVs) determine if a button is a PUSH or a TOGGLE button
//            A CV is also used to determine which pin is used for the energency stop button
//            The following INPUT ports exist:
//            - PC7 => RS-bit 8
//            - PC6 => RS-bit 7
//            - PC5 => RS-bit 6
//            - PC4 => RS-bit 5
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
#include "myeeprom.h"

#include "safety.h"            // For MAX_INPUT_PINS
#include "safety_button.h"     // For the input[] array


//*****************************************************************************************************
// DEFINES, STRUCTS and Variable declarations
//*****************************************************************************************************
// Constants to manage the general inputs. Note that we assume a 20ms sample time
// Possible values "pushed" can take
#define OFF                 0           // The last stable button value was "released"
#define ON                  1           // The last stable button value was "pushed"  

// LOW and HIGH values for the integrator. Note that the integrator is updated every 20ms
// If the integrator reaches one of these values, the result is considered to be stable
#define LOW_TRESHOLD        0           // Result becomes LOW if integrator reaches this value (default: 0)
#define HIGH_TRESHOLD       4           // Number of successive "one" ticks before integrator is HIGH

// Debounce time, in 20 ms steps
#define DEBOUNCE_MAX	   20       	// Debouncing time in 20 ms steps


//*****************************************************************************************************
// Initialisation routine. Called at STARTUP and RESET by check_safety_functions()
//*****************************************************************************************************
// Called at STARTUP and RESET by check_safety_functions() in "safety.c"
void init_safety_buttons(void)
{ unsigned char i;
  for (i = 0; i < MAX_INPUT_PINS; i++) 
  { // initialise all values of the input[] array (=clear all buttons)
    input[i].integrator = LOW_TRESHOLD;
    input[i].debounce_time = 0;
    input[i].pushed = OFF;			// button not yet pushed
    input[i].toggle = OFF;			// previous toggle position was off
    // Determine the type of button. 
    if (my_eeprom_read_byte(&CV.T_RS_Push1 + i) == 0) {input[i].type = TOGGLE;}
    else {input[i].type = PUSH;}
  }
  // Determine which pin is used for the emergency stop function
  // Since the CV value is between 1..MAX_INPUT_PINS, we have to substract 1
  emergency_pin = my_eeprom_read_byte(&CV.P_Emergency) - 1;
}



unsigned int test2 = 100;
void testing2(void) {
  test2++; 
  write_lcd_int(test2); 
}


//*****************************************************************************************************
// The next routine reads a value from one input pin (=button), and maintains "pushed" and "toggle"
//*****************************************************************************************************
// Called by handdle_buttons()
// Input: the variable "pin", which has a value between {0 ... (MAX_INPUT_PINS - 1)}
// Output: "pushed" and "toggle" values within the input[] array
// Description: After calling this routine, the "pushed" value within the input[] array represents the 
// current / most recent stable position of an input button. The "toggle" value becomes "ON" when the
// button is pushed for the first time, and becomes "OFF" when the button is called again.
// To determine if an input value is stable, the input[] array maintains
// two additional variables: an integrator and a debouncing time.
// The integrator is used to debounce input, see: http://www.kennethkuhn.com/electronics/debounce.c
// A figure explaining the operation of the integrator can also be found in the documentation folder
// (see "Debounce.png"), as well as below
//                
// Button       ++++             ++++++++++++     ++++
//
// Integrator    MAX min          MAX         min  MAX min   (MAX = HIGH_TRESHOLD / min = LOW_TRESHOLD)
//
//                +---+            +-----------+    +---+
// pushed         |   |            |           |    |   |
//          ------+   +------------+           +----+   +---
//
//
//                +----------------+                +-------
// toggle         |                |                |
//          ------+                +----------------+ 
//
void read_input(unsigned char pin) 
{ unsigned char input_value;	// Level on the requested input pin
  unsigned char input_mask;		// Needed to select requested input pin
  // Step 1: determine the microcontroller input port we will read now
  // Compensation is needed since the FIRST_INPUT_PIN need not be 0
  // Create an input_mask, with possible values {1, 2, 4, 8, 16, 32, etc}
  input_mask = (1<< (pin + FIRST_INPUT_PIN));     // left shift
  // Step 2: determine the value of that port.
  input_value = (INPUTPORT & input_mask);
  // Step 3: Update the "integrator" and "pushed" values of the input[] array, unless debouncing is going on
  if (input[pin].debounce_time > 0) {input[pin].debounce_time--;}
  else { 
    // Step 3A: update the integrator value of the input[] array
    // The integrator follows the input, decreasing or increasing, towards the tresholds
    // If button is not pushed, the value of the input is 0
    // If button is pushed, the value of the input is 1, 2, 4, 8, ... (depending on which pin is being read)
    if ((input_value == 0) && (input[pin].integrator > LOW_TRESHOLD))  {input[pin].integrator--;}
    if ((input_value != 0) && (input[pin].integrator < HIGH_TRESHOLD)) {input[pin].integrator++;}
    // Step 3B: update "pushed" of the input[] array. Set "debounce_time" if needed
    // Integrator has become HIGH, and last button value was "not pushed"
    if ((input[pin].integrator >= HIGH_TRESHOLD) && (input[pin].pushed == OFF)) { 
      input[pin].pushed = ON;						// change "pushed"
      input[pin].debounce_time = DEBOUNCE_MAX;		// and start debouncing
      input[pin].toggle = !input[pin].toggle;		// negate toggle
    }
    // Integrator becomes LOW, and last button value was "pushed"
    if ((input[pin].integrator <= LOW_TRESHOLD) && (input[pin].pushed == ON)) {
      input[pin].pushed = OFF;						// "change pushed"
      // Note that we might start debouncing here as well
    }
  }
}


//*****************************************************************************************************
// The next routine reads all button values
//*****************************************************************************************************
// Called by check_safety_functions() in "safety.c"
void handdle_buttons(void) {
  unsigned char i;
  // Step 1: determine for each input pin the last stable button position
  for (i = 0; i < MAX_INPUT_PINS; i++) read_input(i);
}


//*****************************************************************************************************
// The next function tells if the emergency button has just before being pushed 
//*****************************************************************************************************
// Called by run_state_machine() in "safety.c"
// The input[] array is updated by handdle_buttons() every 20 ms
// Uses a flag which remembers the previous value 20ms earlier, and allows detection of changes 
unsigned char previous_emergency_pushed_value = 0;

unsigned char emergency_button_pushed(void) {
  unsigned char result = 0;
  // Has the button just before been pushed or released?
  if (input[emergency_pin].pushed != previous_emergency_pushed_value) {
    // Reset the flag 
    previous_emergency_pushed_value = input[emergency_pin].pushed;
    // If it has just been pushed, the function should return "true"
    if (input[emergency_pin].pushed) {result = 1;}
  }
  return result;
}


