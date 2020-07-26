//------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder2.2
//
// Copyright (c) 2007 Kufer
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
// 
//------------------------------------------------------------------------
//
// file:      cv_data_relays4.h
// author:    Wolfgang Kufer / Aiko Pras
// history:   2007-08-03 V0.1 kw start
//            2007-08-18 V0.2 kw default for myADDR high changed to 0x80
//                               -> this means: unprogrammed    
//            2011-01-15 V0.3 ap initial values have been included for the new CVs: System and
//                               SkipEven.
//            2012-01-03 v0.4 ap New Cvs for GBM added.
//            2013-03-12 v0.5 ap The ability is added to program the CVs on the main (PoM).
//            2014-01-06 v0.6 ap File customized for relais.
//            2014-01-11 v0.7 ap File customized for the safety (watchdog) decoder.
//
// CVs specific for the safety decoder:
// - T_Watchdog   Number of seconds watchdog relay will remain active
//                After reception of a watchdog message form the PC, the safety decoder will keep the
//                watchdog relay on for a period of T_Watchdog seconds. Within this period a new watchdog
//                message should be received from the PC; if not, the relay will be switched off
// - T_TrainMove  Time after an RS-emergency button push for PC to stop all trains (in 100 msec steps)
//                After sending an RS-bus emergency stop message, the safety decoder allows the PC a few
//                seconds time to stop all trains. If trains are still running after this time, the relay
//                will be switched off
// - T_RS_PushX   Time RS-bus stays ON after a PUSH button is pushed (in 20 ms steps)
//                The safety decoder has four input ports for buttons:
//                - PC7 (RS-bit 8)
//                - PC6 (RS-bit 7)
//                - PC5 (RS-bit 6)
//                - PC4 (RS-bit 5)
//                If the value is 0, the button will act as a toggle button. In such case the first push
//                will activate the button and RS-bus feedback and the second will release it.
//
//-----------------------------------------------------------------------------
// data in EEPROM:
// Note: the order of these data corresponds to physical CV-Address
//       CV1 is coded at #00
//       see RP 9.2.2 for more information

// Content         Name         CV  Access Comment
   0x01,        // myAddrL       1  R/W    Accessory Address low (6 bits).
   0,           // cv2           2  R      not used
   5,           // T_on_F1       3  R      Hold time for relays 1 (in 20ms steps)
   5,           // T_on_F2       4  R      Same dor relays 2
   5,           // T_on_F3       5  R      Same dor relays 3
   5,           // T_on_F4       6  R      Same dor relays 4
   9,           // version       7  R      Software version. Should be > 7
   0x0D,        // VID           8  R/W    Vendor ID (0x0D = DIY Decoder
                                           // write value 0x0D = 13 to reset CVs to default values
   0x80,        // myAddrH       9  R/W    Accessory Address high (3 bits)
   127,         // MyRsAddress  10  R/W    RS-bus address for the safety decoder (1..128 / not set: 0)
   0,           // cv11         11  R      not used
   0,           // cv12         12  R      not used
   0,           // cv13         13  R      not used
   0,           // cv14         14  R      not used
   0,           // cv15         15  R      not used
   0,           // cv16         16  R      not used
   0,           // cv17         17  R      not used
   0,           // cv18         18  R      not used
   1,           // CmdStation   19  R/W    To handle manufacturer specific address coding
						// 0 - Standard (such as Roco 10764)
						// 1 - Lenz
   0,           // RSRetry      20  R/W    Number of RS-Bus retransmissions
   0,           // SkipEven     21  R/W    Only Decoder Addresses 1, 3, 5 .... 1023 will be used
   0,           // cv534        22  R      not used
   0,           // Search       23  R/W    If 1: decoder LED blinks
   0,           // cv536        24  R      not used
   0,           // Restart      25  R/W    To restart (as opposed to reset) the decoder: use after PoM write
   0,           // DccQuality   26  R/W    DCC Signal Quality
   0b10000000,  // DecType      27  R/W    Decoder Type
						// bx00010000 - Switch decoder
						// bx00010001 - Switch decoder with Emergency board
						// bx00010100 - Servo decoder
						// bx00100000 - Relais decoder for 4 relais
						// bx00100001 - Relais decoder for 16 relais
						// bx10000000 - Watchdog and safety decoder
   0,           // BiDi         28  R      Bi-Directional Communication Config. Keep at 0.
                // Config       29  R      similar to CV#29; for acc. decoders
      (1<<7)                                    // 1 = we are accessory
    | (0<<6)                                    // 0 = we do 9 bit decoder adressing
    | (0<<5)                                    // 0 = we are basic accessory decoder
    | 0,                                        // 4..0: all reserved
   0x0D,        // VID_2        30  R      Second Vendor ID, to detect AP decoders
   0,           // cv31         31  R      not used
   0,           // cv32         32  R      not used

// CV for SWITCH, RELAYS4, SAFETY etc Decoder
   1,           // SendFB       33  R/W    Decoder will send switch feedback messages via RS-Bus 
                                           // Should be 0 if decoder sends ONLY PoM feedback (Add. 128)
                                           // Usually 1, since most decoders, including the safety decoder, do send FB messages
// CVs specific for the SAFETY Decoder
   4,			// P_Emergency  34  R/W    Which Pin on the X8 connector is for emergency stop. Possible values: 1 .. 4
                                           // 1 = Port PC4 of the ATMEL processor
                                           // 4 = Port PC7 of the ATMEL processor
   5,           // T_Watchdog	35  R/W    Number of seconds watchdog relay will remain active
   20,          // T_TrainMove	36  R/W    Time after an RS-emergency button push for PC to stop all trains (in 100 msec steps)
   50,          // T_CheckMove	37  R/W    Interval in which we check if PC stopped all trains (in 100 msec steps)
   0,           // T_RS_Push1	38  R/W    Time RS-bus stays ON after a PUSH button is pushed (in 20 ms steps)
   0,           // T_RS_Push2	39  R/W    Same here. A value of 0 means a toggle button
   150,         // T_RS_Push3	40  R/W    Same here
   150,         // T_RS_Push4	41  R/W    Same here


