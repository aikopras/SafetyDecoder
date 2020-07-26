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
// file:		safety_feedback.h
// author:		Aiko Pras
// history:		2014-03-02 V0.1 ap Initial version
//				2016-01-11 V0.2 ap State and emergency feedback merged in a single file
//
//
//*****************************************************************************************************
void init_safety_feedback(void);					// Initialisation

// Called every 20 ms from check_safety_functions() in safety.c
void RS_connect(void);								// Register feedback module
void RS_button_feedback(void);						// Send the button values in the second RS-bus nibble

// Called from the state machine in safety.c
void RS_state_feedback(unsigned char state);		// current state in the first RS-bus nibble
void set_RS_emergency_flag(void);					// emergency flag in the second RS-bus nibble
void clear_RS_emergency_flag(void);					// emergency flag in the second RS-bus nibble

