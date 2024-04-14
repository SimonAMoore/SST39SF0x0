const static uint8_t  STR_BUFFER_LEN  = 80;
const static uint16_t DATA_BUFFER_LEN = 4096;

char strBuffer[STR_BUFFER_LEN];       // String buffer for sprintf() function
uint8_t dataBuffer[DATA_BUFFER_LEN];  // Data byte buffer for reading/writing EPROM

void setup() {
  // Start serial communication
  SerialIO_Begin();

  // Set up EPROM control pins
  EPROM_init();

  // Set up shift/counter register
  SCR_init();
  
  // Get manufacturer and device IDs and print to console
  sprintf(strBuffer, "Device ID: [%02X:%02X]\n", EPROM_SST_manufacturerID(), EPROM_SST_deviceID());
  Serial.println(strBuffer);

  // Test erasing sector 0x08 at address 0x08000
  EPROM_SST_sector_Erase(0x08000);

  // Test writing a byte to EPROM
  bool result = EPROM_SST_byte_Program(0x1ffff, 0x55);
  Serial.print("Writing byte: 0x55 to address: 0x1ffff ");
  Serial.print(result ? "[SUCCESS] " : "[FAILURE] ");
  Serial.print("0x1ffff = 0x");
  Serial.println(EPROM_readByte(), HEX);
  Serial.println();

  // Test erasing entire EPROM
  EPROM_SST_chip_Erase();

  // Get BBC Master Rom Header Information
  for (uint8_t i = 0; i < 8; i++) {
    BBC_getRomHeaders(i);
  }
  //HALT();
}

void loop() {
  // Process serial IO
  SerialIO_Loop();

  // Delay for debuging
  delay(1000);

  /*
  const uint32_t START  = 0x00000;
  const uint32_t END    = 0x1ffff;

  // Reset shift/counter register and set it to counter mode
  SCR_reset();
  SCR_setAddress(START);
  SCR_setCounterMode();

  // Initialise CRC-32 and SHA-1 Calculations
  CRC32_init();
  SHA1_init();

  for (uint32_t address = START; address <= END; address++) {
    // Read a byte of data from 2764 EPROM
    uint8_t data = EPROM_readByte();

    // Update the CRC32 and SHA-1 calculation with current data byte
    CRC32_update(data);
    SHA1_update(data);

    // Output current address if this is the start of a line
    if ((address % 16) == 0) {
      uint32_t temp = address;
      sprintf(strBuffer, "\r\n%04lx:%04lx | ", address >> 16, address & 0xffff);
      Serial.print(strBuffer);
    }

    // Output the current data byte
    sprintf(strBuffer, "%02x ", data);
    Serial.print(strBuffer);

    // Pulse clock of shift/counter register to increment counter
    SCR_clockPulse();
  }

  Serial.println("\r\n-----------------------------------------------------------");

  // Print final CRC32 value
  sprintf(strBuffer, "CRC32 : %08lx", CRC32_finalise());
  Serial.println(strBuffer);

  // Finalise SHA-1 calculation
  SHA1_finalise();
  
  // SHA-1 160-bit final state variables
  uint32_t a,b,c,d,e;

  // Get the SHA-1 final state
  SHA1_getState(&a, &b, &c, &d, &e);
  sprintf(strBuffer, "SHA-1 : %08lx%08lx%08lx%08lx%08lx", a, b, c, d, e);
  Serial.println(strBuffer);

  Serial.println("-----------------------------------------------------------");

  // HALT
  HALT("DEBUG");
  */
}

void HALT(String msg) {
  Serial.println("\n[HALTED]: " + msg);
  Serial.flush();
  Serial.end();
  while(1);
}
