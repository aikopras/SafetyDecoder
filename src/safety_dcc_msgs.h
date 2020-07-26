//-------------------------------------------------------------------------------
//
// OpenDCC - OpenDecoder
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at http://www.gnu.org/licenses/gpl.txt
// 
//-------------------------------------------------------------------------------

void init_safety_dcc_msgs(void);					// Called from init_safety

// Called from main
void analyse_switch_message(void);			// To check if a watchdog message is received
void trains_moving_message(void);			// To signal a DCC loco message with speed > 0 is received

// Called from run_state_machine, thus (maximum) once every 20ms
unsigned char watchdog_msg_received(void);	// Did we receive a watchdog message? If yes, clear the flag
unsigned char watchdog_timeout(void); 		// No watchdog messages have been received in the last interval
void start_timer_pc_stop(void);				// Timer to allow the computer to stop all trains
void start_timer_stoptrains(void);			// Timer to check if the computer stopped all trains
unsigned char pc_stop_timeout(void); 		// Has the time the PC may use to stop all trains expired?
unsigned char stoptrains_timeout(void); 	// Has the time in which we check if the PC stopped all trains expired?
void clear_trains_moving_flag(void);		// Flag to check if we see trains_moving_messages 
unsigned char trains_moving(void); 			// Have we seen trains_moving_messages?

// Called from check_safety_functions, thus once every 20ms
void update_watchdog_timer(void);			// Countdown the watchdog timer
void update_pc_stop_timer(void);			// Countdown the timer that allows the PC to stop all trains
void update_stoptrains_timer(void);			// Countdown the timer to check if the PC stopped all trains



