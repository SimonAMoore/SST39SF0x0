const String romTypes[16] = { "6502 Basic",
                              "6502 Turbo", 
                              "6502 Code ", 
                              "6800 Code ", 
                              "----------", 
                              "----------", 
                              "----------", 
                              "----------", 
                              "Z80 Code  ", 
                              "32016 Code", 
                              "Reserved  ", 
                              "80186 Code", 
                              "80286 Code", 
                              "ARM Code  ", 
                              "----------", 
                              "----------"};

// Read in and display rom headers from SSTF010 1MBit 8*16KB Rom
void BBC_getRomHeaders(uint8_t romBank) {
  uint32_t  romAddress  = (uint32_t)romBank << 14;

  // Reset shift/counter register
  SCR_reset();

  // Set address to start of ROM Header
  SCR_setAddress(romAddress);

  // Set shift/counter register to count mode
  SCR_setCounterMode();

  // Declare 256 byte buffer for paged ROMs
  uint8_t romBuffer[256];

  // Copy first 256 bytes of ROM into buffer
  for (uint16_t i = 0; i < 256; i++) {
    romBuffer[i] = EPROM_readByte();
    SCR_clockPulse();
  }

  // Check for valid Language or Service ROMs
  if (BBC_isValidROM(romBuffer))
  {
    // BBC Paged Rom Header Variables
    const uint16_t  romLanguageEntry    = (uint16_t)romBuffer[2] << 8 | romBuffer[1];
    const uint16_t  romServiceEntry     = (uint16_t)romBuffer[5] << 8 | romBuffer[4];
    const uint8_t   romTypeByte         = romBuffer[6];
    const uint8_t   romCopyrightOffset  = romBuffer[7];

    sprintf(strBuffer, "  [%02x]:%05lx | &%04x  &%04x  ", romBank, romAddress, romLanguageEntry, romServiceEntry);
    Serial.print(strBuffer);
    Serial.print(romTypes[romTypeByte & 0x07] + "  ");
    Serial.print((romTypeByte & 0x80) ? "S" : "-");
    Serial.print((romTypeByte & 0x40) ? "L" : "-");
    Serial.print((romTypeByte & 0x20) ? "T" : "-");
    Serial.print("  ");
    
    uint8_t i = BBC_printHeaderString(0x09, romBuffer, "  ");
    BBC_printHeaderString(i + 1, romBuffer, " ");
    BBC_printHeaderString(romCopyrightOffset + 1, romBuffer, "\n");
  }
  else {
    sprintf(strBuffer, "  [%02x]:%05lx |", romBank, romAddress);
    Serial.println(strBuffer);
  }
}

uint8_t BBC_printHeaderString(uint8_t i, uint8_t *buffer, String str) {
  do {
    uint8_t b = buffer[i++];
    if (b >= 0x20 && b < 127) Serial.print((char)b);
  } while (buffer[i] != 0);
  Serial.print(str);
  return i;
}

bool BBC_isValidROM(char *buffer) {
  bool isValid = false;

  if ((buffer[0] == 0x4c) || (buffer[0] == 0x00) &&
      (buffer[3] == 0x4c) || (buffer[3] == 0x00)) {
      isValid = true;
  }

  return isValid;
}