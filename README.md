SST39SF0x0 Flash Eprom Programmer using an Arduino UNO R3

Address lines are generated by a PIC18F2550 microcontroller operating as a combined 19-bit shift register and counter.

The Arduino controls the counter/shift register with three control lines:

CLK_IN - Clock pulse, data and commands are latched on falling edge.

DATA_IN - Data to shift into register or control counter direction.

MODE - Sets operating mode between shift or counter mode. Also resets register.

Arduino is connected directly to SST39SF0x0 EPROM's data lines DQ0-DQ7; and control lines OE! - Output Enable, WE! - Write Enable, and CE! - Chip Enable (All active low).
