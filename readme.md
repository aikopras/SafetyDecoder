# Settings #

### Watchdog commands: ###
- Accessory Address: 1005
- DCC connected to LZV "RIJDEN"
- Switch commands should alternate between + and -
- Time-out is 5 seconds

### RS-Bus: ###
- Address: 127
- Connected to LZV "RIJDEN"
- Emergency bit: 8
- Configured as Push button

### Connection ###
The Watchdog relay connects / disconnects the C Signal between LZV 100 and the boosters. Note that the E signal is NOT used to signal an overload or short-circuit to the LZV 100, since the LZV reacts upon such situation by schwitching off all connected devices via Xpressnet. As a consequence, all feedback decoders will loose power.
