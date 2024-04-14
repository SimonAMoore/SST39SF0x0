const uint8_t CLK   = A0; // Shift register clock output pin on Arduino UNO
const uint8_t DATA  = A1; // Shift register data output pin on Arduino UNO
const uint8_t MODE  = A2; // Shift register command output pin on Arduino Uno

const uint32_t CLKT = 50; // Clock pulse width in microseconds

void SCR_setAddress(uint32_t address) {
  uint8_t bank = address >> 16;
  uint16_t addr = address & 0xffff;
  SCR_setAddress(bank, address);
}

// Shift 19-bit bank:address into shift-register
void SCR_setAddress(uint8_t bank, uint16_t address) {
 // Set address bank bits
  for (int position = 2; position >= 0; position--) {
    // Get individual bits from address
    uint8_t bit = (bank & bit(position)) != 0;

    // Set data out based on address bit
    digitalWrite(DATA, !bit);

    // Pulse clock to signal shift register to read in data
    SCR_clockPulse();
  }

  // Set address bits 0-15
  for (int position = 15; position >= 0; position--) {
    // Get individual bits from address
    uint8_t bit = (address & bit(position)) != 0;

    // Set data out based on address bit
    digitalWrite(DATA, !bit);

    // Pulse clock to signal shift register to read in data
    SCR_clockPulse();
  }

  /*
  Serial.print("Setting address bus: ");
  Serial.print(bank, HEX);
  Serial.print(":");
  Serial.println(address, HEX);
  delay(1000);
  */
 }

void SCR_init() {
  // Set up shift/counter register control pins
  pinMode(CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(MODE, OUTPUT);

  // Shift/counter register clock and data are active low, mode is active high
  digitalWrite(DATA, HIGH);
  digitalWrite(MODE, LOW);
  digitalWrite(CLK, HIGH);

  SCR_reset();
}

void SCR_reset() {
  // Set shift/counter into reset mode
  digitalWrite(DATA, LOW);
  digitalWrite(MODE, HIGH);

  // Pulse clock to execute reset command
  SCR_clockPulse();

  // Set shift/counter into run mode
  digitalWrite(MODE, LOW);
}

void SCR_setCounterMode() {
  // Set shift/counter into command mode
  digitalWrite(DATA, HIGH);
  digitalWrite(MODE, HIGH);

  // Pulse clock to execute set counter mode
  SCR_clockPulse();

  // Set shift/counter into run mode
  digitalWrite(MODE, LOW);
}

void SCR_clockPulse() {
  digitalWrite(CLK, LOW);
  delayMicroseconds(CLKT);
  digitalWrite(CLK,HIGH);
}
