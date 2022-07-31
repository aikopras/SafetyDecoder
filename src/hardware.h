// *******************************************************************************************************
// File:      hardware.h
// Author:    Aiko Pras
// History:   2022/06/09 AP Version 1.0
// 
// Purpose:   Safety decoder - Board details
//  
// The safety decoder software has been developed for the ATMega 16A processor, 
// but for testing purpose it will also run on the Arduino 2560 lift decoder board. 
//
// Note that all DCC and RS-Bus pins are defined and initialised in the AP_DCC_Decoder_Basic library,
// which is included by dcc_rs.h
// 
//
// Compile settings: ATMega16A / Safety decoder board
// ==================================================
// Use the Mightycore board
// The following settings are used (under the Tools section of the Arduino IDE):
// - Board: ATmega16               - Important to set this right
// - Clock: 11.0592Mhz             - Important to set this right
// - BOD: 2,7V                     - Seems a save value
// - EEPROM: ***                   - Seems that retained doesn't work
// - Compiler: LTO enabled         - More efficient code
// - Pinout: Standard Pinout       - Important to set this right
// - Bootloader: No                - Important - See below  
// - Port: usbserial....           - Depends on the specifics of your development system
// - Programmer: USBASP (Megacore) - Depends on the specifics of your development system
//
// 
// Compile settings: ATMega2560 / Lift decoder board
// =================================================
// The lift decoder board uses some AVR pins that are not available on standard Arduino MEGA boards: 
// - Blue LED: PD4
// - Green LED: PD5
// - Yellow LEDs: PD6
// 
// To easily access these pins, for compiling via the Arduino IDE the MEGACORE - ATmega2560 board has been used
// See: https://github.com/MCUdude/MegaCore for installation and usage. 
//
// The following settings are used (under the Tools section of the Arduino IDE):
// - Board: ATmega2560             - Important to set this right
// - Clock: External 16Mhz         - Important to set this right
// - BOD: 2,7V                     - Seems a save value
// - EEPROM: ***                   - Seems that retained doesn't work
// - Compiler: LTO enabled         - More efficient code
// - Pinout: Arduino MEGA Pinout   - Important to set this right
// - Bootloader: No                - Important - See below  
// - Port: usbserial....           - Depends on the specifics of your development system
// - Programmer: USBASP (Megacore) - Depends on the specifics of your development system
// 
// Bootloader: although the board makes UART0 available as monitoring port, which can be connected
// to the development system via a serial to usb adapter, in general we won't be able to use such adapter 
// to upload new code. The main reason is that a bootloader requires a RESET before it will start 
// uploading new code. For this to happen the DTR signal of a serial adapter should be connected 
// to the boards reset pin. Most adapters don't export the DTR signal. 
// 
// Setting the Fuses
// Before the board can be used first, the fuses need to be set. There are two options for that:
// 1) Make sure that "Bootloader: No" is selected. After that select Tools => Burn Bootloader 
//    This will set the fuse bits, but will not upload a bootloader
// 2) Use the development system's command line to issue the following command (assuming usbasp):
// avrdude -c usbasp -p m2560 -U lfuse:w:0xEE:m -U hfuse:w:0xD9:m -U efuse:w:0xFE:m 
//
//
// ******************************************************************************************************
#pragma once

// LCD Output is relatively expensive regarding SRAM and CPU time. Since LCD Output is primarily
// useful for debugging, it may be disabled once the program has been tested.
// #define LCD_OUTPUT 


#if defined(MIGHTYCORE) && defined(__AVR_ATmega16__)
// This is the ATMega 16 Mightycore board for the watchdog / safety decoder
//
// In addition to the other basic DCC and RS-bus boards, the safety decoder board has the follwing
// specific hardware connections:
// - An OPTO-IN (opto-isolated) connector (X8), to connect the emergency stop button (and other buttons)
// - A LED-BLINK connector (X11), for the red LEDS of the push-buttons 
// - A LED-OUT connector (X10), for 4 additional signalling LEDs (only three are used)
// - A Watchdog connector (X9), to control a relay connected to the LENZ LZV100 
// 
// OPTO-IN:
// The safety decoder board has four opto-isolated inputs for buttons. If a button is pushed, the  
// associated AVR input port becomes high. All AVR input ports are connected with 6.8K to ground, 
// thus if a button is not pushed, the assiciated AVR input port remains low.
// The emergency stop button should be connected to the inner pin of the X8 connector.
// This is pin X8.5, which triggers PC7.
// 
// LED-BLINK:
// The LED-BLINK connector (X11) is controlled by PA5.
//
// LED-OUT:
// Three additional LEDs are connected to the X10 connector
//  - Yellow LED: X10 - pin 5, controlled by PA1
//  - Green LED: X10- pin 4, controlled by PA2 
//  - Red LED: X10- pin 2, controlled by PA3
//  - Optional LED: X10 - pin 1, controlled by PA4
//
// WATCHDOG RELAY
// Controlled by PA0
//
// The hardware supports a maximum of 4 buttons, connected to connector 8 pins 1, 2, 4 and 5.
// Each button requires 39 Bytes of SRAM; we use only a single button.
// The first button  connects to PIN_PC4: Arduino pin 20, Connector 8.1 and RS-bus Bit 5
// The fourth button connects to PIN_PC7: Arduino pin 23, Connector 8.5 and RS-bus Bit 8
#define BUTTONS_USED      4
#define FIRST_BUTTON      20             // PIN_PC4 / Connector 8.1 / RS-bus Bit 5
#define DEBOUNCETIME      80             // In ms. Default is 25ms
#define PULLUP_ENABLE     false          // Don't use the internal pull-up resistors
#define INVERT            false          // Use high logic level when a button is pressed

#define LEDS_BLINKING     PIN_PA5        // Arduino pin 29 (A5)
#define LED_YELLOW        PIN_PA1        // Arduino pin 25 (A1)
#define LED_GREEN         PIN_PA2        // Arduino pin 26 (A2)
#define LED_RED           PIN_PA3        // Arduino pin 27 (A3)

#define WATCHDOG_RELAY    PIN_PA0        // Arduino pin 24 (A0) 


#define RS        4   // PB4 - Connector: Pin 6
#define RW        5   // PB5 - Connector: Pin 7
#define ENABLE    6   // PB6 - Connector: Pin 15
#define D4        0   // PB0 - Connector: Pin 2
#define D5        1   // PB1 - Connector: Pin 3
#define D6        2   // PB2 - Connector: Pin 4
#define D7        3   // PB3 - Connector: Pin 5



// ******************************************************************************************************
#elif defined(MEGACORE)
// This is the Megacore Lift decoder board
// https://easyeda.com/aikopras/support-lift-controller 
//
// The lift decoder board has 14 input and 14 outputs:
//
// Input number:      1   2   3   4   5   6   7   8   9  10  11  12  13  14
// Arduino Pin:      49  48  47  46  45  44  43  42  37  36  35  34  33  32
// PORT/PIN         PL0 PL1 PL2 PL3 PL4 PL5 PL6 PL7 PC0 PC1 PC2 PC3 PC4 PC5
// Buttons must be connected to ground, so if a button is pushed, the associated AVR input port
// will become low. If the button is not pushed, the input port should be high, this the internal
// pull-up resistors should be activated.
//
// Output number:     1   2   3   4   5   6   7   8   9  10  11  12  13  14
// Arduino Pin:      54  55  56  57  58  59  60  61  62  53  64  65  66  67
// PORT/PIN         PF0 PF1 PF2 PF3 PF4 PF5 PF6 PF7 PK0 PK1 PK2 PK3 PK4 PK5
//

// The hardware supports a maximum of 14 buttons, but only 4 of them may be used.
// The first  button connects to PIN_PC5: Arduino pin 32, Input number 14 and RS-bus Bit 5
// The fourth button connects to PIN_PC2: Arduino pin 35, Input number 11 and RS-bus Bit 8
#define BUTTONS_USED      1
#define FIRST_BUTTON      32             // PIN_PC5 / Input number 14 / RS-bus Bit 5
#define DEBOUNCETIME      80             // In ms. Default is 25ms
#define PULLUP_ENABLE     true           // Do use the internal pull-up resistors
#define INVERT            true           // Use high logic level when a button is pressed

#define LEDS_BLINKING     PIN_PF1        // Arduino pin 55 / Output 2
#define LED_YELLOW        PIN_PD6        // Arduino pin 79
#define LED_GREEN         PIN_PD5        // Arduino pin 78
#define LED_RED           PIN_PD4        // Arduino pin 77 / The color is in fact blue ...

#define WATCHDOG_RELAY    PIN_PF0        // Arduino pin 54 / Output 1


// LCD Pins (on the IDC 16 pin connector)
#define RS       53   // PB0 - Connector: Pin 6
#define RW       51   // PB2 - Connector: Pin 5
#define ENABLE   50   // PB3 - Connector: Pin 15
#define D4       12   // PB6 - Connector: Pin 2
#define D5       11   // PB5 - Connector: Pin 3
#define D6        9   // PH6 - Connector: Pin 4
#define D7       10   // PB4 - Connector: Pin 5


// ******************************************************************************************************
#else
  #error Code will not run on this board
#endif
