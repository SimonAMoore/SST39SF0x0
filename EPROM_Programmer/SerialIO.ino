static const String   PROGRAM_TITLE     = "SST39SF0x0 Flash EPROM Programmer";
static const String   PROGRAM_COPYRIGHT = "(C) Simon Moore. All Rights Reserved.";
static const String   PROGRAM_VERSION   = "Version 1.0.2";
static const String   PROGRAM_DATE      = "12th April 2024.";

static const uint16_t STR_BUFFER_SIZE   = 80;       // Size of temporary buffer for sprintf()
static const uint32_t BAUD_RATE         = 921600;   // Serial port BAUD rate
static const uint32_t SERIAL_TIMEOUT    = 10000;    // Command timeout length in milliseconds
static const uint8_t  COMMAND_NUM       = 16;       // Number of commands in command list array

// List of serial protocol commands
// ================================
//
// Send handshake character 'z'
// Receive acknowledge character 'Z'
//
// CMD_ID  char   Function            Bytes   Input/Output             Description
// =====================================================================================================================================
// SBINRY  [B]    Set binary mode       (0)                            Set data transfer to binary mode
// SASCII  [A]    Set ASCII mode        (0)                            Set data transfer to ASCII mode
// SDBLSZ  [d|D]  Set data block size   (2)   -> (WORD <= 0x1000)      Set size of data block for binary data transfers (Up to 4KBytes)
// GDEVID  [i|I]  Get device ID         (0)   <- (BYTE:BYTE)           Get the manufacturer and device ID for EPROM
// SSADDR  [s]    Set start address     (5)   -> (DWORD < 0x6ffff)     Set the start address for future commands
// SEADDR  [S]    Set end address       (1)   -> (BYTE)                Set the end address of EPROM in 4K banks
// RDBYTE  [r]    Read data byte        (0)   <- (BYTE)                Read EPROM byte from current address
// RDBLCK  [R]    Read data block       (0)   <- (Data Block)          Read EPROM block from current bank-masked-address
// WDBLCK  [w|W]  Write data block      (D)   -> (Data Block)          Write block of data starting at current address
// EEBANK  [e]    Erase EPROM bank      (4)   -> (DWORD)               Erase single 4KB bank of EPROM at address if DWORD = 0xaa55aa55
// EEPROM  [E]    Erase EPROM          (16)   -> (char[16])            Erase entire EPROM if data matches CRC32 hash value
// GCRC32  [g]    Get EPROM CRC         (1)   -> (BYTE) <- (char[16])  Calculate CRC32 hash value of BYTE num. of blocks
// GSHA_1  [G]    Get EPROM SHA-1       (1)   -> (BYTE) <- (char[40])  Calculate SHA-1 hash value of BYTE num. of blocks
// =====================================================================================================================================

// Command_ID identifiers
enum CmdID { SBINRY, SASCII, SDBLSZ, GDEVID, SSADDR, SEADDR, RDBYTE, RDBLCK, WDBLCK, EEBANK, EEPROM, GCRC32, GSHA_1 };

String cmdDesc[COMMAND_NUM] = { "SBINRY", "SASCII", "SDBLSZ", "GDEVID", "SSADDR", "SEADDR", "RDBYTE", "RDBLCK", "WDBLCK", "EEBANK", "EEPROM", "GCRC32", "GSHA-1" };

// Command structure
struct Command {
  char    c;    // command character code
  int8_t  d;    // Size of data in bytes (-1 for binary data block)
  CmdID   id;   // Command ID
};

// List of serial protocol commands
Command cmdList[COMMAND_NUM] = { { 'B',  0, SBINRY}, { 'A',  0, SASCII},
                                 { 'd',  4, SDBLSZ}, { 'D',  4, SDBLSZ},
                                 { 'i',  0, GDEVID}, { 'I',  0, GDEVID},
                                 { 's',  5, SSADDR}, { 'S',  1, SEADDR},
                                 { 'r',  0, RDBYTE}, { 'R',  0, RDBLCK},
                                 { 'w', -1, WDBLCK}, { 'W', -1, WDBLCK},
                                 { 'e',  4, EEBANK}, { 'E', 16, EEPROM},
                                 { 'g',  1, GCRC32}, { 'G',  1, GSHA_1} };

int8_t cmdB = 0;    // Index of the current command being executed in command list

// Data transfer mode types
enum TransferMode { BINARY, ASCII };

TransferMode transferMode = ASCII;    // Current transfer mode

// State machine program states
enum PState { ERROR,
              IDLE,
              WAIT_CMD_B,
              WAIT_CMD_D,
              RECV_DBLOCK,
              CMD_RUN };

PState pState = IDLE;   // Current program state

uint32_t lastMillis = millis();   // 32 bit counter for timeout

void SerialIO_Begin() {
  Serial.begin(BAUD_RATE, SERIAL_8N1);
  Serial.println();
  Serial.println(PROGRAM_TITLE);
  Serial.println(PROGRAM_COPYRIGHT);
  Serial.println(PROGRAM_VERSION);
  Serial.println(PROGRAM_DATE);
  Serial.println();
}

void SerialIO_Loop() {
  switch(pState) {
    // Device error state.
    default:
    case ERROR:
      HALT("Device Error State");
      break;

    // Waiting for handshake character 'z'
    case IDLE:
      if (Serial.available() > 0) {
        if (Serial.read() == 'z') {
          Serial.write('Z\n');        // Confirm handshake with 'Z'
          pState = WAIT_CMD_B;        // Set program state to WAIT_CMD_B (wait for command byte)
          cmdB = 0;                   // Reset current command
        }
      }
      lastMillis = millis();          // Reset timeout counter
      break;

    // Waiting for command byte code
    case WAIT_CMD_B:
      if (Serial.available() > 0) {
        // Get command character from serial data
        char c = Serial.read();

        // Set program state to idle in case no valid command parsed
        pState = IDLE;

        // Parse command character
        for (uint8_t i = 0; i < COMMAND_NUM; i++) {
          if (c == cmdList[i].c) {
            cmdB = i;
            // Set next program state based on command's data byte length (d)
            switch (cmdList[i].d) {
              // Default is to expect a series of data bytes of length d
              default:
                pState = WAIT_CMD_D;
                break;

              // If (d = -1) then command is receiving a binary data block
              case -1:
                pState = RECV_DBLOCK;
                break;

              // If (d = 0) then command does not have any data bytes
              case 0:
                pState = CMD_RUN;
                break;
            }
            break;
          }
        }
      }
      break;

    // Waiting for command data
    case WAIT_CMD_D:
      // For DEBUG
      Serial.print("WAIT_CMD_D : ");
      Serial.println(cmdDesc[cmdList[cmdB].id]);

      // Process command data bytes from serial input

      break;

    // Receive binary data block
    case RECV_DBLOCK:
      // For DEBUG
      Serial.print("RECV_DBLOCK : ");
      Serial.println(cmdDesc[cmdList[cmdB].id]);

      // Proces binary data block from serial input

      break;

    // Execute command function
    case CMD_RUN:
      // For DEBUG
      Serial.print("CMD_RUN : ");
      Serial.println(cmdDesc[cmdList[cmdB].id]);

      // Execute current command
      
      break;
  }

  // If timer exceeds timeout value, reset program state to idle
  if (millis() - lastMillis > SERIAL_TIMEOUT) {
    Serial.println("IDLE");    // For DEBUG
    pState = IDLE;
  }
}
