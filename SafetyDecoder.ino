// *****************************************************************************************************
// File:      safetyDecoder.ino
// Author:    Aiko Pras
// History:   2013-01-12 V0.1 AP: initial version, based on OpenDCC
//            2016-01-11 V0.2 AP: major rewrite, to ensure compatibility with TrainController
//            2022/08/01 V1.0 AP: First implementation of the safety decoder for Arduino 
//
// Summary:
// The safety decoder performs two functions: 
// 1) Watchdog function: to check if the PC with train control software still sends commands
// 2) Emergency stop: make sure all trains stop in case one of the emergency stop buttons is pushed
// See Description.md for details
//
// Compiling:
// ----------
// See "hardware.h" for details.
// Since Timer 1 is available, it is best to compile the safety decoder using the RSBUS_USES_SW_T1 variant.
// This should be configured in the RSBus library, in the file "RSbusVariants.h"
//
// Hardware
// ========
// The safety decoder hardware can be found on: https://oshwlab.com/aikopras/watchdog-decoder
// For testing purposes it also runs on: https://oshwlab.com/aikopras/support-lift-controller
//
// *****************************************************************************************************

#include <Arduino.h>
#include <LiquidCrystal.h>           // Allow LCD output of the current state and position
#include <AP_DCC_Decoder_Core.h>     // all generic objects are instantiated in here
#include "hardware.h"                // Pins for LEDs, DCC input, RS-Bus output, buttons, relays ...
#include "safety.h"                  // Pins for LEDs, DCC input, RS-Bus output, buttons, relays ...
#include "button.h"                  // The buttons object is defined here
#include "dcc_rs.h"                  // DCC commands - PoM/SM actions (dccSystem) - RS-Bus
#include "led.h"                     // The led object is defined here
#include "relay.h"                   // The relay object is defined here


void setup() {
  // Step 1: Write a welcome text to the lcd_display, which can be connected for debugging purposes
  // Note: LCD output may be enbles / disabled in hardware.h
  #ifdef LCD_OUTPUT
  lcd.begin(20,4);
  lcd.setCursor(0, 0);
  lcd.print("Safety decoder");
  #endif

  // Step 2: Set the CV default values (see AP_CV_values.h. for details) 
  // The safety decoder is different from normal (switch, servo and GBM) decoders, in the
  // sense that the default addresses for the decoder and RS-Bus is not "undefined",  
  // but 1005 for the accesory address and 127 for the RS-Bus address.
  // Both addresses may be modified however using PoM or SM messages.
  // In addition, the accesory address may be modified by pushing the onboard programming button 
  // Such push also resets the RS-Bus address: in case the new accessory address > 512,
  // the new RS-Bus address will default to 0. In that case we need to rewrite it into 127.
  cvValues.init(SafetyDecoder, 10);
  if (cvValues.read(myRSAddr) == 0) cvValues.write(myRSAddr, 127);

  // Step 3: initialise the dcc and rsBus objects with addresses and other settings.
  // This includes attaching the pins of the DCC input, the RS-Bus in- and output, the onboard LED
  // and button, and the Accessory Address as stored in the CV.
  decoderHardware.init();

  // Step 4: Initialise the object for sending button and state changes via the RS-Bus
  rsBus.address = cvValues.read(myRSAddr);    // 1..127 - Default is 127

  // Step 5: Initialise the LEDs
  leds.init();
  leds.green.turn_off();
  leds.red.turn_off();
  leds.yellow.turn_off();
  leds.safety.turn_on();

  // Step 6: Initialise the buttons and relay
  buttons.init();
  relay.init();
  relay.turn_on();                    // Connection between LZV-100 and boosters will be established

  // Step 7: Initialise the state machine
  stateMachine.init();
}


// ******************************************************************************************************
void loop() {
  // Step 1: Check if a safety button was pushed
  buttons.update();
  
  // Step 2: Run the safety decoder's state machine
  stateMachine.run();

  // Step 3: RS-Bus updates
  if (cvValues.defaults[SendFB]) {
    // We have to check if the buffer contains feedback data, and the ISR is ready to send that data via the UART.
    rsBus.checkConnection();
    // In addition we should check if the RS-Bus wants us to resend the latest feedback data.
    // This is is wanted after a decoder restart or after a RS-Bus error  
    if (rsBus.feedbackRequested) rsBus.send8bits(rsBus.feedbackData);
  };

  // Step 4: as frequent as possible we should inspect the RS-Bus polling process, check if  
  // the programming button is pushed, and if the status of the onboard LED should be changed.
  decoderHardware.update();

  // Step 5: as frequent as possible we should check receiption of a new DCC message.
  // Reception of Watchdag accessory messages and checking if trains are moving is implemented here.
  // In addition, it reacts on PoM and SM messages. 
  dccSystem.update();

  // Step 6: update the safety specific LEDs
  leds.update();
};
