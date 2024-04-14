const uint8_t ROM_OE = A3;  // EPROM output enable, active low. G=0 Read mode, G=1 Write mode
const uint8_t ROM_CE = A4;  // EPROM chip enable, active low. E=0, ROM enabled
const uint8_t ROM_WE = A5;  // EPROM write enable, active low. 

//const uint32_t EPROM_DELAY = 5; // Delay in microseconds between successive EPROM commands

const uint8_t DQ0 = 4;  // EPROM data bit 0, pin PD4 Arduino UNO
const uint8_t DQ1 = 5;  // EPROM data bit 1, pin PD5 Arduino UNO
const uint8_t DQ2 = 6;  // EPROM data bit 2, pin PD6 Arduino UNO
const uint8_t DQ3 = 7;  // EPROM data bit 3, pin PD7 Arduino UNO
const uint8_t DQ4 = 8;  // EPROM data bit 4, pin PB0 Arduino UNO
const uint8_t DQ5 = 9;  // EPROM data bit 5, pin PB1 Arduino UNO
const uint8_t DQ6 = 10; // EPROM data bit 6, pin PB2 Arduino UNO
const uint8_t DQ7 = 12; // EPROM data bit 7, pin PB4 Arduino UNO

// Array to map Arduino Pins to EPROM data lines
const uint8_t dataPins[8] = {DQ0, DQ1, DQ2, DQ3, DQ4, DQ5, DQ6, DQ7};

void EPROM_init() {
  // Initialise Arduino pins as outputs for EPROM control signals
  pinMode(ROM_OE, OUTPUT);
  pinMode(ROM_CE, OUTPUT);
  pinMode(ROM_WE, OUTPUT);

  // Set EPROM control signals to inactive mode.
  EPROM_idleMode();
}

void EPROM_readMode() {
  // Set up EPROM data lines for input
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(dataPins[i], INPUT);
  }

  // Enable read mode
  digitalWrite(ROM_WE, HIGH);
  digitalWrite(ROM_CE, LOW);
  digitalWrite(ROM_OE, LOW);
}

void EPROM_writeMode() {
  // Set up EPROM data lines for output
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(dataPins[i], OUTPUT);
  }

  // Enable EPROM write mode
  digitalWrite(ROM_WE, HIGH);
  digitalWrite(ROM_OE, HIGH);
  digitalWrite(ROM_CE, LOW);
}

void EPROM_idleMode() {
  // Enable idle mode
  digitalWrite(ROM_WE, HIGH);
  digitalWrite(ROM_CE, HIGH);
  digitalWrite(ROM_OE, HIGH);
}

uint8_t EPROM_readByte() {
  uint8_t data = 0;

  // Enable EPROM read control lines
  EPROM_readMode();

  // Get data byte from data pins
  for (uint8_t i = 0; i < 8; i++) {
    data += digitalRead(dataPins[i]) << i;
  }

  // Enable EPROM idle mode
  EPROM_idleMode();

  return data;
}

bool EPROM_writeByte(uint8_t data) {
  bool result = false;

  // Enable EPROM control lines for Write mode
  EPROM_writeMode();

  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(dataPins[i], (data & (1 << i)) >> i);
  }

  // Toggle EPROM WE pin to signal a data write
  digitalWrite(ROM_WE, HIGH);
  digitalWrite(ROM_WE, LOW);
  digitalWrite(ROM_WE, HIGH);

  // Enable EPROM read mode
  EPROM_readMode();

  // Data polling until data is written
  for (uint8_t i = 0; i < 0x80; i++) {
    if (digitalRead(DQ7) == ((data & 0x80) >> 7)) {
      result = true;
      break;
    }
  }

  // Enable EPROM idle mode
  EPROM_idleMode();

  return result;
}