// SST39SF0x0A EPROM Programmer Functions
//
// (c) Simon Moore 2024. All Rights Reserved.
//
// version 1.00
//
// 8th April 2024.
//
//        F010  F020  F040                            F040  F020  F010
//                            +--------\/--------+            
//         NC    NC   *A18 -> | 1             32 | <- VDD(5V) 
//                     A16 -> | 2             31 | <- WE#.    
//                     A15 -> | 3             30 | <- A17*   A17   NC
//                     A12 -> | 4             29 | <- A14
//                      A7 -> | 5             28 | <- A13
//                      A6 -> | 6             27 | <- A8
//                      A5 -> | 7             26 | <- A9
//                      A4 -> | 8             25 | <- A11
//                      A3 -> | 9             24 | <- OE#
//                      A2 -> | 10            23 | <- A10
//                      A1 -> | 11            22 | <- CE#
//                      A0 -> | 12            21 | -> DQ7
//                     DQ0 <- | 13            20 | -> DQ6
//                     DQ1 <- | 14            19 | -> DQ5
//                     DQ2 <- | 15            18 | -> DQ4
//                 VSS(0V) -> | 16            17 | -> DQ3
//                            +------------------+

static const uint16_t POLL_COUNT = 0x7fff;

void EPROM_SST_IDEntry() {
  SCR_setAddress(0x05555); EPROM_writeByte(0xaa);
  SCR_setAddress(0x02aaa); EPROM_writeByte(0x55);
  SCR_setAddress(0x05555); EPROM_writeByte(0x90);
}

void EPROM_SST_IDExit() {
  SCR_setAddress(0x05555); EPROM_writeByte(0xaa);
  SCR_setAddress(0x02aaa); EPROM_writeByte(0x55);
  SCR_setAddress(0x05555); EPROM_writeByte(0xf0);
}

bool EPROM_SST_byteProgram(uint32_t address, uint8_t data) {
  SCR_setAddress(0x05555); EPROM_writeByte(0xaa);
  SCR_setAddress(0x02aaa); EPROM_writeByte(0x55);
  SCR_setAddress(0x05555); EPROM_writeByte(0xa0);
  SCR_setAddress(address);

  return EPROM_writeByte(data);
}

bool EPROM_SST_sectorErase(uint32_t address, bool suppressOutput) {
  if (!suppressOutput) {
    Serial.print("Erasing sector at address: 0x");
    Serial.print(address, HEX);
  }

  SCR_setAddress(0x05555); EPROM_writeByte(0xaa);
  SCR_setAddress(0x02aaa); EPROM_writeByte(0x55);
  SCR_setAddress(0x05555); EPROM_writeByte(0x80);
  SCR_setAddress(0x05555); EPROM_writeByte(0xaa);
  SCR_setAddress(0x02aaa); EPROM_writeByte(0x55);
  SCR_setAddress(address); EPROM_writeByte(0x30);

  bool result = EPROM_SST_dataPolling();

  if (!suppressOutput) Serial.println(result ? " [SUCCESS]\n" : " [FAILED]\n");

  return result;
}

bool EPROM_SST_chipErase(bool suppressOutput) {
  if (!suppressOutput) Serial.print("Erasing EPROM...");

  SCR_setAddress(0x05555); EPROM_writeByte(0xaa);
  SCR_setAddress(0x02aaa); EPROM_writeByte(0x55);
  SCR_setAddress(0x05555); EPROM_writeByte(0x80);
  SCR_setAddress(0x05555); EPROM_writeByte(0xaa);
  SCR_setAddress(0x02aaa); EPROM_writeByte(0x55);
  SCR_setAddress(0x05555); EPROM_writeByte(0x10);

  bool result = EPROM_SST_dataPolling();

  if (!suppressOutput) Serial.println(result ? " [SUCCESS]\n" : " [FAILED]\n");

  return result;
}

bool EPROM_SST_dataPolling() {
  bool result = false;

  EPROM_readMode();
  for (uint16_t i = 0; i < POLL_COUNT; i++) {
    if (digitalRead(DQ7)) {
      result = true;
      break;
    }
  }
  EPROM_idleMode();

  return result;
}
uint8_t EPROM_SST_manufacturerID() {
  uint8_t ID = 0;

  EPROM_SST_IDEntry();
  SCR_setAddress(0x00000);
  EPROM_readMode();
  ID = EPROM_readByte();
  EPROM_idleMode();
  EPROM_SST_IDExit();

  return ID;
}

uint8_t EPROM_SST_deviceID() {
  uint8_t ID = 0;

  EPROM_SST_IDEntry();
  SCR_setAddress(0x00001);
  EPROM_readMode();
  ID = EPROM_readByte();
  EPROM_idleMode();
  EPROM_SST_IDExit();

  return ID;
}