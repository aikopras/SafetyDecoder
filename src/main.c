//*****************************************************************************************************
//
// OpenDCC - OpenDecoder2.2
//
// Copyright (c) 2006, 2007 Kufer
// Copyright (c) 2011, 2012, 2013 AP
//
// This source file is subject of the GNU general public license 2,
// that is available at http://www.gnu.org/licenses/gpl.txt
// 
//*****************************************************************************************************
//
// file:      main.c
// author:    Wolfgang Kufer / Aiko Pras
// history:   2011-12-31 V0.01 ap first version, supporting RS-Bus feedback of switch values
//            2014-01-06 V0.02 ap second version, supporting PoM of CV values
//				  Second version uses identical software for switch and relays decoders
//
// Bugs:
// - Input port P4 is not working (note that we don't need that port for the safety decoder)
//
//*****************************************************************************************************
//
// purpose:   Watchdog / Safety decoder with RS-Bus feedback for ATmega16A PU and other AVR 
//
// Note: "global.h" defines which DECODER_TYPE we have, thus how the software should behave.
// Possible behaviors are: 
// - WATCHDOG / SAFETY decoder
//
//
// Here:
// 1. Includes 
// 2. Initialize the hardware
// 3. Program the initial decoder addresses
// 4. Initialize the global variables
// 5. MAIN: analyze command, call the action
//
//*****************************************************************************************************
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <avr/pgmspace.h>        // put var to program memory
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <string.h>

#include "global.h"              // global variables
#include "config.h"              // general definitions the decoder, cv's
#include "myeeprom.h"            // wrapper for eeprom
#include "hardware.h"            // port definitions for target
#include "rs_bus_hardware.h"	 // Hardware for the RS-bus feedback (UART, timer and interrupt)
#include "dcc_receiver.h"        // receiver for dcc
#include "dcc_decode.h"          // decode dcc and handle CV access
#include "timer1.h"         	 // handling of LED control
#include "led.h"                 // LED specific functions (for onboard LED)
#include "safety.h"          	 // Watchdog and Emergency stop functions
#include "safety_dcc_msgs.h"     // analyse_switch_message() / trains_moving_message()  

#include "cv_pom.h"              // Programming on the Main

#include "lcd.h"                 // Peter Fleury's LCD routines
#include "lcd_ap.h"              // LCD messages to display speed or debugging messages

#include "main.h"


//*****************************************************************************************************
//***************************************** AVR hardware ports ****************************************
//*****************************************************************************************************
// Port B is connected to the extension connector
// Depending on the type of decoder, initialisation settings can be overwritten in later functions
void init_hardware(void) {
    
    PORTD = (0<<LED)            // LED off
          | (0<<RSBUS_TX)       // output default off (UART controlled)
          | (1<<RSBUS_RX)       // 1 = pullup
          | (1<<DCCIN)          // 1 = pullup
          | (1<<NC1)            // 1 = pullup (pin is Not Connected)
          | (1<<NC2)            // 1 = pullup (pin is Not Connected)
          | (1<<PROGTASTER)     // 1 = pullup
          | (0<<DCC_ACK);       // ACK off

    DDRD  = (1<<LED)            // output
          | (1<<RSBUS_TX)       // output
          | (0<<RSBUS_RX)       // input (INT0)
          | (0<<DCCIN)          // input (INT1)
          | (0<<NC1)            // input (OC1B)
          | (0<<NC2)            // input (OC1A)
          | (0<<PROGTASTER)     // input    
          | (1<<DCC_ACK);       // output, sending 1 makes an ACK

    DDRA  = 0xFF;               // PORTA: All Bits as Output (Relay and LEDs)
    DDRB  = 0xFF;               // PORTB: All Bits as Output (Extension board)
    DDRC  = 0x00;               // PORTC: All Bits as input  (Opto coupler)
       
    PORTA = 0x00;               // Output: All bits off (relay off, LEDs off)
    PORTB = 0x00;               // output: All bits off (Extension board)     
    PORTC = 0xFF;               // Input opto coupler: pull down
  }


//*****************************************************************************************************
//******************************* Programming after the button is pushed ******************************
//*****************************************************************************************************
// DoProgramming() is called when PROG is pressed
// -- manual programming and accordingly setting of the DCC-bus adress CV
//

void WaitDebounceTime(void) {
  // Busy waits till debouncing time is over
  // We choose as debouncing time 100 x 1 = 100ms
  signed int my_ms = 0;
  while(my_ms < 100) {
    _mydelay_us(1000);
    my_ms++;
  }
}


void DoProgramming(void) {
  unsigned char my_cv1;				// Local copy of CV1
  unsigned char my_cv9;				// Local copy of CV9
  int Ticks_Waited = 0;				// With 100 msec resolution
  WaitDebounceTime();                           // Busy wait debouncing time, for stable button pushed
  if (PROG_PRESSED) {                           // only act if key is still pressed after 100 ms
    turn_led_on();				// indicator we are in programming mode
    while(PROG_PRESSED) {      			// wait for release, and ...
      WaitDebounceTime();			// wait (again) 100 msec
      Ticks_Waited ++;
    }
    if (Ticks_Waited <= 50) {                   // button is released within 5 sec => programme address
      WaitDebounceTime();                       // Busy wait debouncing time, for stable button release
      while(!PROG_PRESSED) {
        if (semaphor_get(C_Received)) {         // Message
          analyze_message(&incoming);
          // CmdType == ANY_ACCESSORY_CMD => Accessory command but not for my current address 
          // CmdType == ACCESSORY_CMD     => Accessory command for my current address 
          if ((CmdType == ACCESSORY_CMD) || (CmdType == ANY_ACCESSORY_CMD)){
            if (RecDecAddr <= 511) {
              // Step 1: Set the Decoder Address
              // The valid range for CV1 is 0..63
              // The valid range for CV9 is 0..3  (or 128, if the decoder has not been initialised)
              // The range of the received decoder address (RecDecAddr) is 0..255 (LENZ) / 511 (NMRA)
              my_cv1 = (RecDecAddr & 0b00111111);
              my_cv9 = ((RecDecAddr >> 6) & 0b00000111);
              my_eeprom_write_byte(&CV.myAddrL, my_cv1);     
              my_eeprom_write_byte(&CV.myAddrH, my_cv9);
              // Note that we do not set the RS-bus address of the safety decoder. By default that 
              // address is 127. In case a change is needed, POM messages should be used to change CV10
            }
            LED_OFF;
            // we got reprogrammed -> forget everthing running and restart decoder!
            _restart();
          }
        }
      }
    }
    else {                                        // button is pushed for more than 5 seconds => Reset
      ResetDecoder();                             // Is defined in cv_pom
      _restart();                                 // really hard exit
    }
  }
  return;  
}


//*****************************************************************************************************
//********************************* Initialisation of global variables ********************************
//*****************************************************************************************************
void init_global(void)
{ // Step 1: Determine the kind of accessory decoder addressing we react upon
  MyConfig = my_eeprom_read_byte(&CV.Config) & (1<<6);  // (0=basic, 1=extended)
  // Step 2: Determine the decoder type
  // Will be one of the following: TYPE_SWITCH, TYPE_SERVO, TYPE_RELAYS4, TYPE_RELAYS16 or WATCHDOG
  MyType = my_eeprom_read_byte(&CV.DecType);
  // Step 3: Determine the decoder address, based on CV1 and CV9.
  // On the web there is some confusion regarding the exact relationship between the
  // decoder address within the DCC decoder hardware and CV1 and CV9. The convention used by 
  // my decoders is: My_Dec_Addr = CV1 + (CV9*64). 
  // Note that this implies that the minimum value for CV1 should be 0 (and not 1).
  // Thus we have the following:
  // The valid range of CV1 is 0..63
  // The valid range of CV9 is 0..3  (or 128, if the decoder has not been initialised)
  unsigned char cv1 = my_eeprom_read_byte(&CV.myAddrL);
  unsigned char cv9 = my_eeprom_read_byte(&CV.myAddrH);
  cv1 = cv1;
  cv9 = cv9 & 0x07; 					// Select only the last three bits
  if (MyConfig != 1) My_Dec_Addr = (cv9 << 6) + cv1;	// Basic Acc. Addressing
  else My_Dec_Addr = (cv9 << 8) + cv1;			// Extended Acc. Addressing
  // The valid range of My_Dec_Addr is 0..511 (0..255 if Xpressnet is used).
  // Make sure that My_Dec_Addr will be INVALID_DEC_ADR if the decoder has not been initialised  
  if ((cv1 > 63)) My_Dec_Addr = INVALID_DEC_ADR;
  if (My_Dec_Addr > 511) My_Dec_Addr = INVALID_DEC_ADR;
  if (my_eeprom_read_byte(&CV.myAddrH) & 0x80) My_Dec_Addr = INVALID_DEC_ADR;
  // Step 4: Determine the RS-Bus address. The valid range is 1..128
  // It can be 0, however, if the myRSAddr CV has not been initialised yet
  // In that case it can later be initialised via a PoM message
  My_RS_Addr = my_eeprom_read_byte(&CV.MyRsAddr);
  if (My_RS_Addr > 128) {My_RS_Addr = 0;}
  // In several decoder software variants multiple RS-bus addresses are used
  // For the safety decoder we use one address for normal feedbacks, but another one for PoM
  RS_Addr2Use = My_RS_Addr;
  // Step 5: Determine the address for (Loco) PoM messages. Use My_Dec_Addr
  // Check for invalid addresses; if invalid then use LOCO_OFFSET - 1
  // This is the address we will listen to for initialising the decoder
  My_Loco_Addr = My_Dec_Addr + LOCO_OFFSET;
  if (My_Loco_Addr < LOCO_OFFSET) {My_Loco_Addr = LOCO_OFFSET - 1;}
  if (My_Loco_Addr > (255 + LOCO_OFFSET)) {My_Loco_Addr = LOCO_OFFSET - 1;}
  // Step 6: Initialise global variables
  Have_Feedback = my_eeprom_read_byte(&CV.SendFB);
  CmdType = IGNORE_CMD;
}


//*****************************************************************************************************
//********************************************* Main loop *********************************************
//*****************************************************************************************************
int main(void)
  {
    init_lcd();
    
    init_hardware();			// setup hardware ports
    init_global();              // initialise the global variables
 
    
    My_Dec_Addr = 251;			// Wissel 1005 +
    

    
    init_dcc_receiver();		// DCC physical layer
    init_dcc_decode();			// DCC datalink layer
    init_timer1();              // General 1 ms timer, also used for 20 / 40 msec ticks
    init_RS_hardware();			// RS-bus physical layer
    init_safety();              // Safety decoder specific
    
    // write_lcd_string("Safety");
    
    sei();				// Global enable interrupts
    
    // Check if the EEPROM has been initialised. In case the program is compiled 
    // and flashed using "make flash", the EEPROM should have been initialised during flash. 
    // However, the Arduino IDE does not flash the EPPROM during program upload. 
    // In that case we need to initialise from here. 
    if ((my_eeprom_read_byte(&CV.VID) != 0x0D) || (my_eeprom_read_byte(&CV.VID_2) != 0x0D)) {
      ResetDecoder();                             // Copy all default values to EEPROM
      _restart();                                 // really hard exit
    }

    // check if the decoder has a valid Decoder address
    if (My_Dec_Addr == INVALID_DEC_ADR) flash_led_fast(5);
    
    while(1) {
      if (PROG_PRESSED) DoProgramming();
      if (semaphor_query(C_Received)) {	// DCC message received
        analyze_message(&incoming);
        if (CmdType >= 1) {   
 //         write_lcd_int(CmdType + 10);
          
          if (CmdType == ANY_ACCESSORY_CMD) ;
          if (CmdType == LOCO_F0F4_CMD)	    ; // In future versions watchdog functions could listen to F0F4
          if (CmdType == RESET_CMD)	        ; // DCC Reset messages are send if the HALT on the handheld is pushed
          if (CmdType == ACCESSORY_CMD)	 analyse_switch_message();	// is this a watchdog message?
          if (CmdType == LOCO_SPEED_CMD) trains_moving_message();	// this is a train speed > 0 message
          if (CmdType == POM_CMD) 	     cv_operation(POM_CMD); 
          if (CmdType == SM_CMD) 	     cv_operation(SM_CMD); 
        }
        semaphor_get(C_Received);	// now take away the protection
      }
      if (timer1fired) {			// 1 time tick (20ms) has passed)
        check_led_time_out();
        check_PoM_time_out();
        check_safety_functions();
        timer1fired = 0;
      }
    } // End WHILE
    
  }

//*****************************************************************************************************
