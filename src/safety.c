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
// file:      safety.c
// author:    Aiko Pras
// history:   2013-01-12 V0.1 ap initial version
//            2016-01-11 v0.2 ap major rewrite, to ensure compatibility with TrainController
//
//
// purpose:   Routines for the safety decoder
//            For a complete flow-diagram, see the files "safety-Local.pdf" and "safety-Watchdog.pdf"
//            in the documentation folder
//
// Summary:
// The safety decoder performs two functions: 
// 1) Watchdog function: to check if the PC with train control software still sends commands
// 2) Emergency stop: make sure all trains stop in case one of the emergency stop buttons is pushed
//
// Watchdog function:
// ------------------
// The watchdog function controls a relay connected to the DCC control system (such as LENZ LZV100).
// If the safety decoder does not receive a "switch command" within a few seconds (default is 5 seconds),
// the relay will be released and the DCC control system immediately stops all trains. 
// This functions is comparable to the WD-DEC decoder from LDT.
// 
// At start-up the watchdog function will be inactive and the yellow LED is on to indicate that the 
// state is LOCAL. The relay will be ON, however, to allow operation without PC.
// If the safety decoder receives a watchdog related accessory command from the PC, the watchdog  
// function will be activated and the green LED will light to signal that the state has changed to REMOTE.
// If a new watchdog related accessory command is not received before the watchdog period is over and  
// trains are still running, the relay will be released and the DCC control system will stop all trains.
// The state will become W_RELAY_OFF, the red LED turns on and the button light starts flashing fast.
//
// Note that trains can also be stopped if the user pushes the HALT button of the handheld or the
// "Einfrieren" button within the TrainController program. In that case DCC RESET packets will be send 
// to the DCC control system and trains will be automatically stopped. In such case this program goes
// back to the STARTUP phase.
//
// Emergency stop function:
// ------------------------
// The safety decoder will also monitor if one of the emergency stop buttons is pushed. If such button 
// is pushed, the decoder will generate an RS-bus feedback message and will react in one of the 
// following ways:
// 1) If the state is LOCAL (= watchdog function is inactive), it will deactivate the watchdog relay
// 2) If the state is REMOTE (= watchdog function is active), it will start a short timer 
//    (default 2 seconds) after which it checks if trains have been stopped by the computer or not.
//    2A: If trains have not been stopped by the computer, the relay will be released to ensure  
//        the DCC control system will stop all trains
//    2B: If trains have been stopped by the computer, we wait a while to allow human intervention.
//        If, after this period is over, trains start moving again, we move back to the LOCAL state.
//
// 
// Meaning of LEDs cpnnected to the X10 connector of the safety decoder
// - Yellow LED: Local control: watchdog function is not active
// - Green  LED: Remote control by the computer: watchdog function is active
// - Red    LED: watchdog relay is released
// - Extra  LED: no special meaning
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


#include "global.h"                // global variables
#include "config.h"                // general definitions the decoder, cv's
#include "hardware.h"              // port definitions for target
#include "led.h"                   // LED specific functions (for onboard LED)
#include "cv_pom.h"                // Programming on the Main
#include "rs_bus_hardware.h"	   // Hardware for the RS-bus feedback (UART, timer and interrupt)
#include "rs_bus_messages.h"	   // Analyse and collect feedback information 
#include "safety.h"          	   // Watchdog and Emergency stop functions
#include "safety_led.h"            // LED specific functions for external LEDs (on connector X10)
#include "safety_dcc_msgs.h"	   // To control the emergency stop relay
#include "safety_button.h"         // Functions related to the emergency stop button
#include "safety_feedback.h"       // Functions for sending safety messages via the RS-bus

#include "lcd.h"		           // Peter Fleury's LCD routines
#include "lcd_ap.h"		           // LCD messages to display speed or debugging messages



// Note that the follwing aliases have already been defined in hardware.h:
// LED_RED_ON
// LED_RED_OFF
// LED_YELLOW_ON
// LED_YELLOW_OFF
// LED_GREEN_ON
// LED_GREEN_OFF
// LED_EXTRA_ON
// LED_EXTRA_OFF
// LED_BUTTONS_OFF
// LED_BUTTONS_ON

//*****************************************************************************************************
//********************************************* DECLARATIONS ******************************************
//*****************************************************************************************************
// The following states are defined:
// STARTUP     - Initialising
// LOCAL       - No active computer program like TC or Railware to control trains
// REMOTE      - Under watchdog control. Computer sends Watchdog messages
// W_RELAY_OFF - Watchdog timer expired and trains still running
// L_PUSHED    - Button pushed in LOCAL  state. Relay has been switched off
// R_PUSHED    - Button pushed in REMOTE state. Wait if computer stops all trains
// R_WAIT      - Time we give the computer to stop all trains
// R_STOP      - The computer has gracefully stopped all trains
// R_RELAY_OFF - Button pushed in REMOTE state. Relay has been switched off
unsigned char state;	// The state in which the safety decoder currently is




//*****************************************************************************************************
// Support routine that concentrate common lines of code in one place 
//*****************************************************************************************************
unsigned char state_counter = 0;	// Temporary test variable

void lcd_test(unsigned char state) {
  // write_lcd_int(state);
  if      (state == STARTUP) 		{write_lcd_string("STARTUP");}
  else if (state == LOCAL)			{write_lcd_string("LOCAL");}
  else if (state == L_PUSHED)		{write_lcd_string("L_PUSHED");}
  else if (state == REMOTE) 		{write_lcd_string("REMOTE");}
  else if (state == W_STOP) 		{write_lcd_string("W_STOP");}
  else if (state == W_RELAY_OFF) 	{write_lcd_string("W_RELAY_OFF");}
  else if (state == PC_WAIT) 		{write_lcd_string("PC_WAIT");}
  else if (state == R_STOP) 		{write_lcd_string("R_STOP");}
  else if (state == R_RELAY_OFF) 	{write_lcd_string("R_RELAY_OFF");}
  else if (state == R_STOPPED) 		{write_lcd_string("R_STOPPED");}
  else {write_lcd_string("UNKNOWN");};
  state_counter++;
  write_lcd_int2(state_counter);
}

void next_state(unsigned char next_state) {
  state = next_state;
  RS_state_feedback(state);
  lcd_test(state);

}


//*****************************************************************************************************
// init_safety() is called from main at startup
//*****************************************************************************************************
void init_safety(void) {
  init_safety_buttons();
  init_safety_dcc_msgs();
  init_safety_feedback();
  init_safety_leds();
  next_state(STARTUP);
}


//*****************************************************************************************************
// Run the state machine.
// For details: "Safety-Local", "Safety-Remote" and "Safety-Remote-button" in the documentation folder  
//*****************************************************************************************************
void run_state_machine(void) {
  // 1) See: Safety-Local
  // STARTUP
  if (state == STARTUP) {
    LED_YELLOW_ON;
    LED_GREEN_OFF;
    LED_RED_OFF;
    led_buttons(ON); 
    RELAY_ON;
    clear_RS_emergency_flag();
    next_state(LOCAL);
  }
  // LOCAL
  else if (state == LOCAL) {
    if (watchdog_msg_received()) {
      LED_YELLOW_OFF;
      LED_GREEN_ON;
      next_state(REMOTE);
    }
    else if (emergency_button_pushed()) {
      RELAY_OFF;
      led_buttons(FLASH); 
      set_RS_emergency_flag();
      next_state(L_PUSHED);
    }
  }
  // L_PUSHED
  else if (state == L_PUSHED) {
    if (emergency_button_pushed()) {
      next_state(STARTUP);
    }
  }
  // 2) See: Safety-Remote
  // REMOTE
  else if (state == REMOTE) {
    if (watchdog_msg_received()) {
      ; // No action needed. Everything OK
    }
    else if (watchdog_timeout()) {
      LED_GREEN_OFF;
      LED_RED_ON;
      start_timer_stoptrains();
      clear_trains_moving_flag();
      next_state(W_STOP);
    }
    else if (emergency_button_pushed()) {
      led_buttons(FLASH); 
      set_RS_emergency_flag();
      start_timer_pc_stop();
      next_state(PC_WAIT);
    }
  }
  //W_STOP
  else if (state == W_STOP) {
    if (trains_moving()) {
      RELAY_OFF;
      led_buttons(FLASH_FAST); 
      next_state(W_RELAY_OFF);
    }
    else if (stoptrains_timeout()) {
      next_state(STARTUP);
    }
  }
  //W_RELAY_OFF
  else if (state == W_RELAY_OFF) {
    if (emergency_button_pushed()) {
      next_state(STARTUP);
    }
  }  
  // PC_WAIT
  else if (state == PC_WAIT) {
    if (pc_stop_timeout()) {
      LED_GREEN_OFF;
      LED_RED_ON;
      start_timer_stoptrains();
      clear_trains_moving_flag();
      next_state(R_STOP);
    }
  }
  // 3) See: Safety-Remote_button
  // R_STOP
  else if (state == R_STOP) {
    if (trains_moving()) {
      RELAY_OFF;
      led_buttons(FLASH_FAST); 
      next_state(R_RELAY_OFF);
    }
    else if (stoptrains_timeout()) {
      next_state(R_STOPPED);
    }
  }
  // R_STOPPED
  else if (state == R_STOPPED) {
    if (emergency_button_pushed()) {
      next_state(STARTUP);
    }
    else if (trains_moving()) {
      next_state(STARTUP);
    }
  }
  // R_RELAY_OFF
  else if (state == R_RELAY_OFF) {
    if (emergency_button_pushed()) {
      next_state(STARTUP);
    }
  }
}


//*****************************************************************************************************
// check_safety_functions() is called from main every 20 ms
//*****************************************************************************************************
void check_safety_functions(void) {
  // Step 1: Update the various timers that require 20ms ticks
  check_safety_leds_time_out();
  update_watchdog_timer();
  update_pc_stop_timer();
  update_stoptrains_timer();
  // Step 2: Check if RS-Bus is connected 
  if ((RS_Addr2Use != INVALID_DEC_ADR) && (RS_Layer_2_connected == 0)) {RS_connect();}	// Try to connect
  // Step 3: Handle all (push and toggle) buttons, including the emergency stop button
  handdle_buttons();
  // Step 4: Run the state machine
  run_state_machine();
  // Step 5: Send the RS-bus feedback messages for the various buttons
  // Not everytime that the emergency button is pushed a feedback message should be send; sending of
  // emergency feedback messages is therefore explicitly controlled by the state machine.
  // Therefore the state machine must be called before RS_button_feedbacks is called.
  RS_button_feedback();
  // Step 6: Clear flag that signals if trains are still running during the next 20ms tick
}
