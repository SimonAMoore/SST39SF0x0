SST39SF0x0 Flash Eprom Programmer using an Arduino UNO R3

Address lines are generated by a PIC18F2550 microcontroller operating as a combined 18-bit shift register and counter.

Arduino controls the counter/shift register with three control lines:

  CLK_IN    Clock pulse, data and commands are latched on falling edge
  DATA_IN   Date to shift into register 
  MODE      Sets operating mode between shift or counter mode. Also resets register.

Arduino conects directly to EPROMs data lines DQ0-DQ7 and OE!, WE!, and CS!.