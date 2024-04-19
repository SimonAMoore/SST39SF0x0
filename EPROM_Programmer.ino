//
//                                           [ SCL ]
//                                           [ SDA ]
//                                           [ AREF]
//                                           [ GND ]
//                  [  NC ]                  [ D13 ]
//                  [IORF ]                  [ D12 ] <> DQ7
//                  [ RST ]                  [ D11 ]
//                  [ 3V3 ]                  [ D10 ] <> DQ6
//                  [  5V ]                  [ D9  ] <> DQ5
//                  [ GND ]                  [ D8  ] <> DQ4
//                  [ GND ]                  
//                  [ VIN ]                  [ D7  ] <> DQ3
//                                           [ D6  ] <> DQ2 
//           CLK <- [  A0 ]                  [ D5  ] <> DQ1
//          DATA <- [  A1 ]                  [ D4  ] <> DQ0
//          MODE <- [  A2 ]                  [ D3  ]
//        ROM_OE <- [  A3 ]                  [ D2  ]
//        ROM_CE <- [  A4 ]                  [ TX  ] -> USB Data
//        ROM_WE <- [  A5 ]                  [ RX  ] <- USB Data
//

#define STR_BUFFER_LEN   80
#define DATA_BUFFER_LEN  1024

uint32_t startAddr  = 0x00000;
uint32_t endAddr    = 0x20000;
uint16_t blockSize  = DATA_BUFFER_LEN;

char    strBuffer[STR_BUFFER_LEN];    // String buffer for sprintf() function
uint8_t dataBuffer[DATA_BUFFER_LEN];  // Data byte buffer for reading/writing EPROM

void setup() {
  // Start serial communication
  SerialIO_Begin();

  // Set up EPROM control pins
  EPROM_init();

  // Set up shift/counter register
  SCR_init();
}

void loop() {
  // Process serial IO
  SerialIO_Loop();
}

void HALT(String msg) {
  Serial.println("\n[HALTED]: " + msg);
  Serial.flush();
  Serial.end();
  while(1);
}
