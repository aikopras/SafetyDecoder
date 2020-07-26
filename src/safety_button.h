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
// file:      safety_button.h
// author:    Aiko Pras
// history:   2014-02-05 V0.1 ap initial version
//
//
// purpose:   Routines for the OPTO-IN connector (X8) on the safety decoder
//            Configuration Variables (CVs) determine if a button is a PUSG or a TOGGLE button
//            A CV is also used to determine which pin will be used for the energency stop button
//            The following INPUT ports exist:
//            - PC7 => RS-bit 8
//            - PC6 => RS-bit 7
//            - PC5 => RS-bit 6
//            - PC4 => RS-bit 5
//
//*****************************************************************************************************
// Constants to distinguish between PUSH and TOGGLE buttons.
// Note that the emergency button can be configured as PUSH or TOGGLE button
#define PUSH        0
#define TOGGLE      1  

// The input[] array is shared between the internal read_input and external routines in safety_feedback  
// The "type" variable tells if this input is configured as push or as toggle button.
// The "integrator" and "debounce_time" variables are used internally within the read_input() routine. 
struct {                                // Keep track of successive values of each input input
  unsigned int integrator;              // Integrator values range from LOW_TRESHOLD to HIGH_TRESHOLD
  unsigned int debounce_time;			// Time (in 20 ms) all further input should be ignored
  unsigned char type;					// Boolean to say if this is a push or toggle button
  unsigned char pushed;					// Boolean representing the (most recent) stable button position
  unsigned char toggle;					// Boolean to emulate toggle behavior for push buttons
} input[MAX_INPUT_PINS];                // Array, one element per button (input pin)


// The emergency pin has a value between 0 and (MAX_INPUT_PINS - 1)
// It is derived from P_Emergency CV (by subtracting 1 from the CV value)
// It is used by emergency_button_pushed() in safety_button.c and by safet_feeback.c
unsigned char emergency_pin;         	// Pin on the X8 connector for the emergency stop.    


// Called from safety.c
void init_safety_buttons(void);					// Called during initialisation
void handdle_buttons(void);						// Called every 20ms by check_safety_functions
unsigned char emergency_button_pushed(void);	// Called by the state machine
