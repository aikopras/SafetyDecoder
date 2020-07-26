//------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder2
//
// Copyright (c) 2006 Kufer
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
// 
//------------------------------------------------------------------------
//
// file:      hardware.h
// author:    Wolfgang Kufer
// contact:   kufer@gmx.de
// webpage:   http://www.opendcc.de
// history:   2006-02-14 V0.1 kw copied from opendecoder.c
//            2007-05-21 V0.2 kw added OpenDecoder3
//            2008-09-28 V0.3 kw added OpenDecoder25
//            2010-10-31 V0.4 added OpenDecoder22 (RS-bus feedback)
//            2011-12-31 V0.3 Removed previous changes
//			      Added OpenDecoder22GBM
//
//------------------------------------------------------------------------
//
// purpose:   flexible general purpose decoder for dcc
//            here: hardware definitions
//
//
//========================================================================

// CPU Definitions:
//

#if (__AVR_ATmega8535__)
  // atmega8535:   512 Byte SRAM, 512 Byte EEPROM
  #define SRAM_SIZE		512
  #define EEPROM_SIZE	512
  #define EEPROM_BASE	0x810000L
#elif (__AVR_ATmega16__)
  // atmega16A:   1024 Byte SRAM, 512 Byte EEPROM
  #define SRAM_SIZE		1024
  #define EEPROM_SIZE	512
  #define EEPROM_BASE	0x810000L
#elif (__AVR_ATmega32__)
  // atmega32A:   2048 Byte SRAM, 1024 Byte EEPROM
  #define SRAM_SIZE		2048
  #define EEPROM_SIZE	1024
  #define EEPROM_BASE	0x810000L
#elif (__AVR_ATmega164A__)
  // atmega164A:   1024 Byte SRAM, 512 Byte EEPROM, 2 UARTS
  #define SRAM_SIZE		1024
  #define EEPROM_SIZE	512
  #define EEPROM_BASE	0x810000L
  #define ENHANCED_PROCESSOR	// other timer, interrupt and UART structure
#elif (__AVR_ATmega324A__)
  // atmega324A:   2048 Byte SRAM, 1024 Byte EEPROM, 2 UARTS
  #define SRAM_SIZE		2048
  #define EEPROM_SIZE	1024
  #define EEPROM_BASE	0x810000L
  #define ENHANCED_PROCESSOR	// other timer, interrupt and UART structure
#elif (__AVR_ATmega644P__)
  // atmega644:   1024 Byte SRAM, 512 Byte EEPROM, 2 UARTS
  #define SRAM_SIZE		4096
  #define EEPROM_SIZE	2048
  #define EEPROM_BASE	0x810000L
  #define ENHANCED_PROCESSOR	// other timer, interrupt and UART structure
#else
  #warning Processor
#endif


#ifndef F_CPU
   // prevent compiler error by supplying a default 
   # warning "F_CPU not defined, set default to 11059200 Hz"
   # define F_CPU 11059200UL
   // if changed: check every place where it is used
   // (possible range underflow/overflow in preprocessor!)
#endif


//-------------------------------------------------------------------------------
// PORT Definitions:
//
// PORTA:
// Port for setting the relay (PA0), four LEDs (PA1..PA4) and "button" LEDs (PA5)
// PA6 and PA7 are Not Connected
#define RELAY_PORT      PORTA   // this is the main output port
#define LED_PORT        PORTA   // this is the main output port
#define BUTTON_LED_PORT PORTA   // this is the main output port

#define RELAY_PIN       0       // Relay is connected to PA0
#define RELAY_OFF       	RELAY_PORT &= ~(1<<RELAY_PIN)
#define RELAY_ON        	RELAY_PORT |= (1<<RELAY_PIN)

#define LED_YELLOW_PIN  1       // Yellow LED is connected to PA1 / Connector X10
#define LED_YELLOW_OFF  	LED_PORT &= ~(1<<LED_YELLOW_PIN)
#define LED_YELLOW_ON   	LED_PORT |= (1<<LED_YELLOW_PIN)

#define LED_GREEN_PIN   2       // Green LED is connected to PA2 / Connector X10
#define LED_GREEN_OFF   	LED_PORT &= ~(1<<LED_GREEN_PIN)
#define LED_GREEN_ON    	LED_PORT |= (1<<LED_GREEN_PIN)

#define LED_RED_PIN     3      	// RED LED is connected to PA3 / Connector X10
#define LED_RED_OFF     	LED_PORT &= ~(1<<LED_RED_PIN)
#define LED_RED_ON      	LED_PORT |= (1<<LED_RED_PIN)
#define LED_RED_STATE  		(LED_PORT & (1<<LED_RED_PIN))

#define LED_EXTRA_PIN   4      	// EXTRA LED is connected to PA4 / Connector X10
#define LED_EXTRA_OFF   	LED_PORT &= ~(1<<LED_EXTRA_PIN)
#define LED_EXTRA_ON    	LED_PORT |= (1<<LED_EXTRA_PIN)

#define LED_BUTTONS_PIN 5      	// BUTTONS LED is connected to PA5 / Connector X11
#define LED_BUTTONS_OFF 	LED_PORT &= ~(1<<LED_BUTTONS_PIN)
#define LED_BUTTONS_ON  	LED_PORT |= (1<<LED_BUTTONS_PIN)
#define LED_BUTTONS_TOGLE  	LED_PORT ^= (1<<LED_BUTTONS_PIN)
#define LED_BUTTONS_STATE  	(LED_PORT & (1<<LED_BUTTONS_PIN))


// PORTB:
// Goes to flat cable connector
// Can be used for additional output, for example LCD display, LEDs or relais

// PORTC:
// Port for reading 4 inputs from 4 opto couplers (PC4..PC7)
#define INPUTPORT      	PINC   	// for the opto couplers

// PORTD:
#define LED             0       // output, 1 turns on LED
#define RSBUS_TX        1       // UART for sending feedback via RS-bus
#define RSBUS_RX        2       // must be located on INT0
#define DCCIN           3       // must be located on INT1
#define NC1             4       // output (OC1B)
#define NC2             5       // output (OC1A)
#define PROGTASTER      6       //
#define DCC_ACK         7       // output, sending 1 makes an ACK

#define DCC_PORT        PORTD   // must be defined to have portable code
#define DCC_PORT_IN     PIND    // must be defined to have portable code

#define DCCIN_STATE     (PIND & (1<<DCCIN))

#define PROG_PRESSED    (!(PIND & (1<<PROGTASTER)))
#define LED_OFF         PORTD &= ~(1<<LED)
#define LED_ON          PORTD |= (1<<LED)
#define DCC_ACK_OFF     PORTD &= ~(1<<DCC_ACK)
#define DCC_ACK_ON      PORTD |= (1<<DCC_ACK)

// Note LEDs is active high -> state on == pin high!
#define LED_STATE       ((PIND & (1<<LED)))



