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
// file:      safety_led.h
// author:    Aiko Pras
// history:   2014-01-12 V0.1 ap initial version
//
//
// purpose:   Routines for the LED output connector (X10) on the safety decoder
//            The following LEDs should be connected
//            - Yellow LED: watchdog function is not activated
//            - Green  LED: watchdog function is activated (receives commands from PC)
//            - Red    LED: watchdog function is activated but PC commands are no longer received
//            - Extra  LED: no special meaning
//
//            And the LEDS in the "emergency stop" buttons (connector X11)
//
//
// Note that the following aliases have already been defined in hardware.h:
// LED_GREEN_ON
// LED_GREEN_OFF
// LED_YELLOW_ON
// LED_YELLOW_OFF
// LED_RED_ON
// LED_RED_OFF
// LED_EXTRA_ON
// LED_EXTRA_OFF
// LED_BUTTONS_OFF
// LED_BUTTONS_ON
//
//*****************************************************************************************************

void init_safety_leds(void);

void led_buttons(unsigned char state);

void check_safety_leds_time_out(void);


