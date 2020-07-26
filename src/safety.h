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
// file:      safety.h
// author:    Aiko Pras
// history:   2014-01-12 V0.1 ap initial version
//            2016-01-11 V0.2 ap Changed to reflect changes in states
//
//*****************************************************************************************************
// Global DEFINES and variables for all safety related functions:
//*****************************************************************************************************
// The "state" of the safety decoder can be read by RS_state_feedback()
extern unsigned char state;

// The safety decoder can be in any of the following states
#define STARTUP			1   // Initialising
#define LOCAL			2   // No active computer program like TC or Railware to control trains
#define L_PUSHED		3   // Button pushed in LOCAL  state. Relay has been switched off
#define REMOTE			4   // Under watchdog control. Computer sends Watchdog messages
#define W_STOP   		5   // Watchdog timer expired. Check if trains are still running
#define W_RELAY_OFF		6   // Watchdog timer expired and trains were running. Relay is OFF
#define PC_WAIT			7   // Button pushed in REMOTE state. Wait if computer stops all trains
#define R_STOP		    8   // Check if computer has gracefully stopped all trains
#define R_RELAY_OFF		9   // Computer did not gracefully stop all trains. Relay switched off
#define R_STOPPED	    10  // Computer gracefully stopped all trains.

// The LEDS within the emergency buttons can have the following values
#define OFF				0
#define ON				1
#define FLASH			2
#define FLASH_FAST		3


// Hardware details: first pin (0..7) and number of pins (buttons) that will be analysed
// Note that FIRST_INPUT_PIN + MAX_INPUT_PINS must be <= 8
#define FIRST_INPUT_PIN     4           // PC4 is the first input pin   
#define MAX_INPUT_PINS      4           // We have 4 input pins (PC4, PC5, PC6 and PC7)  

//*****************************************************************************************************
// Safety functions called from main
//*****************************************************************************************************
// Main will not call any other safety related function
void init_safety(void);             // called once at initialisation
void check_safety_functions(void);  // called every 20 mseconds


