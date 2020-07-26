//*****************************************************************************************************
//
// OpenDCC - OpenDecoder2.2
//
// Copyright (c) 2011,2013 Pras
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at http://www.gnu.org/licenses/gpl.txt
// 
//*****************************************************************************************************
//
// file:      safety_dcc_msgs.c
// author:    Aiko Pras
// history:   2012-01-08 V0.1 ap based upon port_engine.c from the OpenDecoder2 project => relays.c
//            2013-12-25 V0.2 ap based upon relays.c => switch.c
//				 changed all relay specific code in switch specific code
//            2014-03-01 V0.3 ap tailered code to watchdog specific code
//            2016-01-12 V0.4 ap improved watchdog specific code
//
//
// Watchdog specific function for the safety decoder.
// Five kind of functions are defined:
// - an initialisation function
// - Functions to handle specific DCC (watchdog) messages
// - Functions to handle the watchdog timer
// - Functions to check if trains are still moving
// - Functions to manage the stoptrains timer
//
// 1) init_safety_dcc_msgs: called from init_safety at startup
// 
// 2) Functions related to the reception of watchdog DCC messages:
// - analyse_switch_message: called from main after reception of an ACCESSORY command
// - watchdog_msg_received?: called from the state machine to see the result of the above function
// To synchronize both functions, the watchdog_msg_received_flag is used
//
// 3) Functions to manage the watchdog timer
// - update_watchdog_timer: called from check_safety_functions every 20ms to decrease the timer
// - watchdog_timeout?: called from the state machine to see if the watchdog timer has expired
//
// 4) Functions to check if trains are still moving
// After the emergency button is pushed, the PC has a small time window (typically a few seconds)
// to stop all trains in a graceful manor. If the PC does not manage (to check this, the safety decoder
// listens to DCC loco messages with a speed > 0), the safety decoder will perform a "hard" stop, 
// by switching the relay off
// - trains_moving_message: called from main after a DCC train command with a speed > 0 is received
// - clear_trains_moving_flag: called from check_safety_functions to clear the flag every new 20ms period
// - trains_moving?: called from the state machine, as condition for IF statements
//
// 5) Functions to manage the stoptrains timer
// - start_timer_stoptrains: called from the state machine to give the computer time to stop all trains
// - update_stoptrains_timer: called from check_safety_functions, every time tick (20 ms)  
// - stoptrains_timeout: called from the state machine to check if the time is over for the computer 
//
//*****************************************************************************************************

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include "global.h"
#include "config.h"
#include "myeeprom.h"
#include "safety_dcc_msgs.h"

//*****************************************************************************************************
//************************************ Definitions and declarations ***********************************
//*****************************************************************************************************
// Set by analyse_switch_message / cleared by watchdog_msg_received()
unsigned char watchdog_msg_received_flag;  
// Set by trains_moving_message / cleared by trains_moving()
unsigned char trains_moving_flag;  

// Set by analyse_switch_message / decreased by update_watchdog_timer / read by watchdog_timeout
struct {
  unsigned int hold_time;	      // maximum acceptable time between watchdog messages (in 20 ms ticks)
  unsigned int rest_time;	      // remaining time before the next watchdog message must be received
} watchdog;


// Set by start_timer_pc_stop()
struct {
  unsigned int max_time;	      // maximum time the PC has to stop all trains (in 20 ms ticks)
  unsigned int rest_time;	      // remaining time the PC has to stop all trains
} pc_stop_timer;


// Set by start_stoptrains_timer()
struct {
  unsigned int max_time;	      // maximum time to check if the PC has stopped all trains (20 ms)
  unsigned int rest_time;	      // remaining time we check if the PC has stopped all trains
} stoptrains_timer;


//*****************************************************************************************************
//*************************************** Initialisation function *************************************
//*****************************************************************************************************
void init_safety_dcc_msgs(void) {
  // Called from safety.c
  // initialise the "administration" for the watchdog
  // store the hold time of the watchdog
  // T_Watchdog CV is in seconds, but time ticks are 20 msec 
  // Note: read the 8 bit value into the 16 bit integer before multiplication.
  watchdog.hold_time = my_eeprom_read_byte(&CV.T_Watchdog);
  watchdog.hold_time = watchdog.hold_time * 50;
  watchdog_msg_received_flag = 0;
  // Initialise the time (in 100 msec steps) we give the PC to stop all trains
  pc_stop_timer.max_time = my_eeprom_read_byte(&CV.T_TrainMove);
  pc_stop_timer.max_time = pc_stop_timer.max_time * 5;
  // Initialise the time (in 100 msec steps) we check if the PC stopped all trains
  stoptrains_timer.max_time = my_eeprom_read_byte(&CV.T_CheckMove);
  stoptrains_timer.max_time = stoptrains_timer.max_time * 5;
  // Initialise the flag if we have a DCC LOCO command with speed > 0
  trains_moving_flag = 0;
}


//*****************************************************************************************************
//********************* Functions related to the reception of watchdog DCC messages *******************
//*****************************************************************************************************
void analyse_switch_message(void) { 
  // Called from main, after a DCC accessory decoder command is received 
  // If it recognizes a watchdog alive message, it sets the watchdog_msg_received_flag
  // and initialises the watchdog timer
  // This flag will be released by the watchdog_msg_received function
  // This function uses the following global variables:
  // - TargetDevice: the switch / relays4 that is being addressed. Range: 0 .. (NUMBER_OF_DEVICES - 1)
  //   For normal switch / relays4 decoders, the range will be 0..3
  // - TargetGate: Targetted coil within that Port. Usually + or - / green or red
  // - TargetActivate: Coil activation (value = 1) or deactivation (value = 0) 
  // Step 1: The watchdog message should address the first device of the decoder
  if (TargetDevice == 0) {
    // Step 2: The watchdog message should be a + command
    if (TargetGate) {
      // Step 3: The watchdog message should be a coil activation command
      if (TargetActivate) {
        watchdog.rest_time = watchdog.hold_time;
        watchdog_msg_received_flag = 1;
      }
    }
  }
} 


unsigned char watchdog_msg_received(void) {
  // Called from safety.c as condition for IF statements
  // It returns the value of the watchdog_msg_received_flag
  // and resets that flag
  unsigned char result = 0;
  if (watchdog_msg_received_flag) {
    result = 1;			     		// Function returns TRUE
    watchdog_msg_received_flag = 0;	// Reset the flag
  }
  return result;
}

//*****************************************************************************************************
// Functions related to the watchdog timer
//*****************************************************************************************************
void update_watchdog_timer(void) { 
  // Called from check_safety_functions, every time tick (20 ms)  
  if (watchdog.rest_time > 0) {
    watchdog.rest_time = watchdog.rest_time - 1;
  }
}


unsigned char watchdog_timeout(void) {
  // Called from the state machine, as condition for IF statements
  // It checks if the value of watchdog.rest_time equals zero
  unsigned char result = 0;
  if (watchdog.rest_time == 0) {
    result = 1;
  }
  return result;
}


//*****************************************************************************************************
//******************************* Functions to check if trains are moving *****************************
//*****************************************************************************************************
// The DCC Command Station continously sends DCC packets. According to the OpenDCC website, the time
// it takes to send a single packet is between 8-12ms. Measeruments using DCCMon showed that DCC 
// packets are send roughly every 15ms. The Command Station cycles through the list of DCC adresses 
// stored in its buffer (cache), and sends (loops) for each of these addresses DCC packets that set  
// Loco speed and functions. Next to DCC Loco packets, the Command Station also sends DCC idle packets.
// Experiences showed that a Command Station that buffers 50 trains may need between 2 and 3 seconds
// before packets for all Loco's have been send and retransmission starts.


void trains_moving_message(void) { 
  // Called from main, after a DCC loco command with a speed > 0 is received.
  // Note that code in dcc_decode.c not only signalled that a speed command was received,
  // but already checked if the speed was indeed greater than zero (thus the train was moving).
  trains_moving_flag = 1;
} 


void clear_trains_moving_flag(void) {
  // Called from the state machine to clear the flag
  // The flag will be set again by main() every time it seens DCC loco messages with speed > 0 
  trains_moving_flag = 0;
} 


unsigned char trains_moving(void) {
  // Called from the state machine, as condition for IF statements 
  // After the call to clear_trains_moving_flag(), did we see DCC loco messages with speed > 0? 
  if (trains_moving_flag) return 1;
  else return 0;
}


//*****************************************************************************************************
// Timer functions needed to allow the PC to stop all trains and check if the PC managed to stop all
//*****************************************************************************************************
// If the computer is told to stop all trains, it sends the appropriate messages to the Command Station
// The Command Station sends to all trains DCC loco messages with speed = 0, or DCC idle packets 
// (depending on how the computer program is configured). If the Command Station has entries for many
// trains in its buffer, cycling through the entire buffer will take some time. Before taking any 
// further actions, we should wait such time (= pc_stop_timer)
void start_timer_pc_stop(void) {
  // Called from the state machine. 
  pc_stop_timer.rest_time = pc_stop_timer.max_time;
}


void update_pc_stop_timer(void) { 
  // Called from check_safety_functions, every time tick (20 ms)  
  if (pc_stop_timer.rest_time > 0) {
    pc_stop_timer.rest_time = pc_stop_timer.rest_time - 1;
  }
}


unsigned char pc_stop_timeout(void) {
  // Called from the state machine. 
  unsigned char result = 0;
  if (pc_stop_timer.rest_time == 0) {
    result = 1;
  }
  return result;
}


//*****************************************************************************************************
// After the computer is supposed to have stopped all trains gracefully, we need to monitor the DCC 
// commands for a certain period of time to check if we still see trains with a speed > 0.
// This time should at least be 2 to 3 seconds, but preferably a bit longer
void start_timer_stoptrains(void) {
  // Called from the state machine. 
  stoptrains_timer.rest_time = stoptrains_timer.max_time;
}


void update_stoptrains_timer(void) { 
  // Called from check_safety_functions, every time tick (20 ms)  
  if (stoptrains_timer.rest_time > 0) {
    stoptrains_timer.rest_time = stoptrains_timer.rest_time - 1;
  }
}


unsigned char stoptrains_timeout(void) {
  // Called from the state machine. 
  unsigned char result = 0;
  if (stoptrains_timer.rest_time == 0) {
    result = 1;
  }
  return result;
}

