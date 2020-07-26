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
// file:		safety_feedback.c
// author:		Aiko Pras
// history:		2014-03-02 V0.1 ap Initial version
//				2016-01-11 V0.2 ap State and emergency feedback
// Called:		from safety.c
//
//
// purpose: RS-bus feedback routines to signal:
//          1) Which (main) STATE the safety decoder is in
//          2) What button values have changed
//          2) If the emergency button has been pushed
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


#include "global.h"              	// global variables
#include "config.h"             	// general definitions the decoder, cv's
#include "hardware.h"            	// port definitions for target
#include "myeeprom.h"               // To read CV values
#include "rs_bus_hardware.h"	 	// Hardware for the RS-bus feedback (UART, timer and interrupt)
#include "rs_bus_messages.h"	 	// Analyse and send feedback information 
#include "safety.h"          	 	// For using "state" and DEFINES
#include "safety_button.h"          // For using the input[] array that holds button values
#include "safety_feedback.h"		// Header file for this .C file


//*****************************************************************************************************
//******************************************** DECLARATIONS *******************************************
//*****************************************************************************************************
unsigned char emergency_flag;		// set/cleared by the state machine and used by RS_button_feedback
unsigned char send_rs_button_flag;	// set, in case a button change asks for a RS-bus message


struct {							// "global" array to keep track of RS-bus outputs
  unsigned int start_waitingtime;	// For PUSH buttons: minimum time after a HIGH output until a LOW 
  unsigned int current_waitingtime;	// For PUSH buttons: Current waiting time until a low
  unsigned char last_message;		// This value will / has been send to the master
} RS_buttons[MAX_INPUT_PINS];		// Array, one element per output signal


//*****************************************************************************************************
//*************************************** Initialisation routine **************************************
//*****************************************************************************************************
// Called at STARTUP and RESET by check_safety_functions() in "safety.c"
void init_safety_feedback(void)
{ unsigned char i;
  for (i = 0; i < MAX_INPUT_PINS; i++) 
  { // initialise the RS_buttons[] array
    RS_buttons[i].start_waitingtime = my_eeprom_read_byte(&CV.T_RS_Push1 + i); // in 20 ms steps
    RS_buttons[i].current_waitingtime = 0;
    RS_buttons[i].last_message = OFF;
  }
  emergency_flag = OFF;
  send_rs_button_flag = OFF;
}


//*****************************************************************************************************
//******************************************* RS-bus Connect ******************************************
//*****************************************************************************************************
// Called by check_safety_functions() (which checks every 20ms) if the connection is up
void RS_connect(void) {
  // Register feedback module: send low and high order nibble in 2 consequtive cycles
  unsigned char nibble;
  if (RS_Layer_1_active) { 			// wait till RS-bus is active 
    // send first nibble
    while (RS_data2send_flag) {};	// busy wait, till the USART ISR has send previous data
    nibble = (0<<NIBBLE);
    format_and_send_RS_data_nibble(nibble);      
    // send second nibble
    while (RS_data2send_flag) {};	// busy wait, till the USART ISR has send previous data
    nibble = (1<<NIBBLE);             
    format_and_send_RS_data_nibble(nibble);
    RS_Addr2Use = My_RS_Addr;
    RS_Layer_2_connected = 1;		// This module should now be connected to the master station
  }
}


//*****************************************************************************************************
// Send first nibble (for state feedback) or second nibble (for button feedback
//*****************************************************************************************************
// RS_state_feedback() is called from next_state(), which is part of the state machine in safety.c
// State information is send in the first RS-bus nibble 
void RS_state_feedback(unsigned char state) {
  unsigned char nibble;
  while (RS_data2send_flag) {};	// busy wait, till the USART ISR has send previous data
  if      (state == LOCAL   )    {nibble = (1 <<DATA_0) | (0<<NIBBLE);}
  else if (state == REMOTE  )    {nibble = (1 <<DATA_1) | (0<<NIBBLE);}
  else if (state == L_PUSHED)    {nibble = (1 <<DATA_2) | (0<<NIBBLE);}
  else if (state == R_RELAY_OFF) {nibble = (1 <<DATA_3) | (0<<NIBBLE);}
  else return; // for other states no RS-bus messages are send
  RS_Addr2Use = My_RS_Addr;
  format_and_send_RS_data_nibble(nibble);  
}


// RS_nibble2_feedback() is an internal subroutine, called by RS_button_feedback(), 
// which in turn is called every 20ms by check_safety_functions() in safety.c 
void RS_nibble2_feedback(void) {
  unsigned char nibble;
  while (RS_data2send_flag) {};	// busy wait, till the USART ISR has send previous data
  RS_Addr2Use = My_RS_Addr;
  nibble = (RS_buttons[0].last_message << DATA_0 )
         | (RS_buttons[1].last_message << DATA_1)
         | (RS_buttons[2].last_message << DATA_2)
         | (RS_buttons[3].last_message << DATA_3)
         | (1<<NIBBLE);
  format_and_send_RS_data_nibble(nibble);  

}


//*****************************************************************************************************
// Emergency feedback  
//*****************************************************************************************************
// The emergency button has non-standard button behaviour. In some cases it should trigger a RS-bus
// message (and possibly release the relay), in other cases it should not trigger the sending of a
// RS-bus message but just allow us to move back to the STARTUP state (where the relay will be set).
// To distinguish between these cases, the state machine has included explicit set_RS_emergency_flag()
// calls; pushing the emergency button will not trigger actions by the RS_button_feedback() routine,
// unless a call to set_RS_emergency_flag() has been performed earlier.
// If set_RS_emergency_flag() is called, the emergency_flag will be set. This flag is checked every
// 20ms via RS_button_feedback(), and triggers sending of an RS-bus message. Clearing the flag can
// be done in two different ways: 
// - by clear_RS_emergency_flag, once the state machine enters the STARTUP state 
// - after a time-out, provided the emergency button is configured as push button
// Note that calls to set_RS_emergency_flag() and clear_RS_emergency_flag() are performed by the 
// state machine, just before RS_button_feedback() is called.


// Called by the state machine from safety.c
void set_RS_emergency_flag(void) {
  emergency_flag = ON;
}


// Called by the state machine from safety.c
void clear_RS_emergency_flag(void) {
  emergency_flag = OFF;
}


// For the emergency buttons we use as "soll-wert" the value of the emergency_flag.
// Therefore we ignore the values of input[].pushed and input[].toggle.
// We do need to check input[].type, however, to automatically clear the RS-bus message
// for push buttons after a time-out. Note that, after such time-out, the emergency_flag
// may still be set. In such case we should still send a RS-bus message with value 0;
// RS-bus messages with value 1 will only be send once the emergency_flag is set again
// For that purpose we maintain the boolean "allow_new_RS_ON"
//
// Emergency_flag           +++              ++++++++++++     +++++++
//
//                          +-+              +----------+     +------
// last_message - TOGGLE    | |              |          |     |
//                       ---+ +--------------+          +-----+
//
//                          +---+            +---+            +---+
// last_message - PUSHED    |   |            |   |            |   |
//                       ---+   +------------+   +------------+   +--
//
// allow_new_RS_ON       ++++---++++++++++++++----------+++++++------
//
unsigned char allow_new_RS_ON = ON;


void handle_emergency_button(unsigned char i) {
  if (input[i].type == TOGGLE) {
    // In case of TOGGLE buttons we have to follow the emergency_flag
    if (emergency_flag != RS_buttons[i].last_message) {
      send_rs_button_flag = ON;
      RS_buttons[i].last_message = 1;
    }    
  };
  if (input[i].type == PUSH) {
    // In case of PUSH buttons, we send an RS-bus SET message if the emergency_flag
    // is set as well as the boolean allow_new_RS_ON.
    if ((emergency_flag == ON) && (allow_new_RS_ON == ON)) {
      send_rs_button_flag = ON;
      RS_buttons[i].last_message = 1;
      RS_buttons[i].current_waitingtime = RS_buttons[i].start_waitingtime;
      allow_new_RS_ON = OFF;
    }
    // If the waitingtime is not over (thus the last RS-bus message was 1), decrease that time
    else if (RS_buttons[i].current_waitingtime > 0) {
      RS_buttons[i].current_waitingtime--;
    }
    // The waitingtime is over 
    else if (RS_buttons[i].current_waitingtime == 0) {
      // if the last RS-bus message was 1, we send an RS-bus message with value 0
      if (RS_buttons[i].last_message == 1) {
        send_rs_button_flag = ON;
        RS_buttons[i].last_message = 0;
      }
      if (emergency_flag == OFF) {
        allow_new_RS_ON = ON;
      }
    }
  }
}


//*****************************************************************************************************
// General button support functions 
//*****************************************************************************************************
void handle_push_button(unsigned char i) {
  // Step 1: check if we are in a waiting state, due to an earlier push
  if (RS_buttons[i].current_waitingtime > 0) {RS_buttons[i].current_waitingtime--;}
  // Step 2: If we are not in a waiting state due to an earlier button push, we may act
  if (RS_buttons[i].current_waitingtime == 0) {
    // If the input button is pushed, set the waitingtime
    if (input[i].pushed) {RS_buttons[i].current_waitingtime = RS_buttons[i].start_waitingtime;}
    // Check if the current value has already been send
    if (input[i].pushed != RS_buttons[i].last_message) {
      send_rs_button_flag = ON;
      RS_buttons[i].last_message = input[i].pushed;
    }
  }
}


void handle_toggle_button(unsigned char i) {
  // If needed, sets RS_buttons[i].last_message to hold the correct value for the next RS-bus (button) message 
  // The new / current toggle position does not reflect the value send in the last RS-bus message  
  if (input[i].toggle != RS_buttons[i].last_message) {
    send_rs_button_flag = ON;
    RS_buttons[i].last_message = input[i].toggle;
  }
}

//*****************************************************************************************************
//***************************************** RS_button_feedback ****************************************
//*****************************************************************************************************
// Called every 20ms by check_safety_functions() in "safety.c"
// Determines if sending of RS-bus feedback messages is needed.
// For each individual pin we check if the "soll-wert" matches the "last RS-bus messega". 
// Depending on the situation, the "soll-wert" should be derived from either the input[] array or 
// the emergency_flag  
void RS_button_feedback(void) {
  unsigned char i;
  // Step 1: reset the flag that asks for the transmission of RS-bus button related messages 
  send_rs_button_flag = OFF;
  // Step 1: Check for each button if button values have changed and a RS-bus message is needed
  for (i = 0; i < MAX_INPUT_PINS; i++) {
    if (i == emergency_pin) {handle_emergency_button(i);}
    else if (input[i].type == PUSH) {handle_push_button(i);}
    else if (input[i].type == TOGGLE) {handle_toggle_button(i);}
  }
  // Step 2: Send RS-bus messages to indicate changes
  if (send_rs_button_flag == ON) {
    RS_nibble2_feedback();
  }
}



