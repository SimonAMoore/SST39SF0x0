#define PROGRAM_TITLE     "SST39SF0x0 Flash EPROM Programmer"
#define PROGRAM_COPYRIGHT "(C) Simon Moore. All Rights Reserved"
#define PROGRAM_VERSION   "Version 1.0.3"
#define PROGRAM_DATE      "14th April 2024."

#define STR_BUFFER_SIZE   80         // Size of temporary buffer for sprintf()
#define BAUD_RATE         57600      // Serial port BAUD rate
#define SERIAL_TIMEOUT    10000      // Command timeout length in milliseconds
#define CMND_NUM          18         // Number of commands in command list array
#define CMD_BUFFER_SIZE   8          // Size of command data buffer
#define EEBANK_CHECK      0xaa55aa55 // DWORD confirmation for erase bank command
#define EEPROM_CHECK      0xa5a5a5a5 // DWORD confirmation for erase chip command
#define ACK               '@'        // Function success acknowledgment
#define N_ACK             '!'        // Function failure acknowledgment

// List of serial protocol commands
// ================================
//
// Send handshake character 'z'
// Receive handshake acknowledge character 'Z'
//
// CMD_ID  char   Function             Bytes  Input/Output             Description
// =====================================================================================================================================
// SBINRY  [b|B]  Set binary mode       (0)                            Set data transfer to binary mode
// SASCII  [a|A]  Set ASCII mode        (0)                            Set data transfer to ASCII mode
// SDBLSZ  [d|D]  Set data block size   (2)   -> (WORD <= 0x0400)      Set size of data block for binary data transfers (Up to 1KBytes)
// GDEVID  [i|I]  Get device ID         (0)   <- (BYTE:BYTE)           Get the manufacturer and device ID for EPROM
// SSADDR  [s]    Set start address     (4)   -> (DWORD < 0x6ffff)     Set the start address for future commands (Resets hash funcions)
// SEADDR  [S]    Set end address       (1)   -> (BYTE)                Set size of EPROM in 4K banks
// RDBYTE  [r]    Read data byte        (0)   <- (BYTE)                Read EPROM byte from current address
// RDBLCK  [R]    Read data block       (0)   <- (Data Block)          Read EPROM block from current bank-masked-address
// WDBLCK  [w|W]  Write data block      (D)   -> (Data Block)          Write block of data starting at current address
// EEBANK  [e]    Erase EPROM bank      (4)   -> (DWORD)               Erase single 4KB bank of EPROM at address if DWORD = 0xaa55aa55
// EEPROM  [E]    Erase EPROM           (4)   -> (DWORD)               Erase entire EPROM if DWORD = 0xa5a5a5a5
// GCRC32  [g]    Get EPROM CRC         (0)   <- (char[16])            Calculate CRC32 hash value of BYTE no. of banks
// GSHA_1  [G]    Get EPROM SHA-1       (0)   <- (char[40])            Calculate SHA-1 hash value of BYTE no. of banks
// =====================================================================================================================================
//
// Acknowledge SUCCESS = '@', FAILURE = '!'

// Command_ID identifiers
enum CmdID { SBINRY, SASCII, SDBLSZ, GDEVID, SSADDR, SEADDR, RDBYTE, RDBLCK, WDBLCK, EEBANK, EEPROM, GCRC32, GSHA_1 };

// Command structure
struct Command {
  char    c;    // command character code
  int8_t  d;    // Size of data in bytes (-1 for binary data block)
  CmdID   id;   // Command ID
};

// List of serial protocol commands
Command cmdList[CMND_NUM] = { { 'b', 0, SBINRY}, { 'B', 0, SBINRY},
                              { 'a', 0, SASCII}, { 'A', 0, SASCII},
                              { 'd', 2, SDBLSZ}, { 'D', 2, SDBLSZ},
                              { 'i', 0, GDEVID}, { 'I', 0, GDEVID},
                              { 's', 4, SSADDR}, { 'S', 1, SEADDR},
                              { 'r', 0, RDBYTE}, { 'R', 0, RDBLCK},
                              { 'w',-1, WDBLCK}, { 'W',-1, WDBLCK},
                              { 'e', 4, EEBANK}, { 'E', 4, EEPROM},
                              { 'g', 0, GCRC32}, { 'G', 0, GSHA_1} };

Command cmd = {};   // Command currently being processed

uint8_t cmdBuffer[CMD_BUFFER_SIZE];   // Buffer for storing received data bytes

enum TransferMode { BINARY, ASCII };  // Data transfer mode types
TransferMode transferMode = ASCII;    // Current transfer mode

enum PState { ERROR, IDLE, WAIT_CMD_B, WAIT_CMD_D, RECV_DBLOCK, CMD_RUN, TIMEOUT };  // State machine possible states
PState pState = IDLE;   // Current program state

void SerialIO_Begin() {
  Serial.begin(BAUD_RATE, SERIAL_8N1);
  Serial.println();
  Serial.println(PROGRAM_TITLE);
  Serial.println(PROGRAM_COPYRIGHT);
  Serial.println(PROGRAM_VERSION);
  Serial.println(PROGRAM_DATE);
  Serial.println();
}

uint32_t lastMillis = millis();   // 32 bit counter for timeout

void SerialIO_Loop() {

  switch(pState) {
    // Device error state.
    default:
    case ERROR: {
      HALT("Device Error State");
      break;
    }

    // Waiting for handshake character 'z'
    case IDLE: {
      if (Serial.available() > 0) {
        if (Serial.read() == 'z') {
          Serial.write('Z');          // Confirm handshake with 'Z'
          pState = WAIT_CMD_B;        // Set program state to WAIT_CMD_B (wait for command byte)
          cmd = {};                   // Reset current command
        }
      }
      lastMillis = millis();          // Reset timeout counter
      break;
    }

    // Waiting for command byte code
    case WAIT_CMD_B: {
      if (Serial.available() > 0) {
        // Get command character from serial data
        char c = Serial.read();

        // Set program state to idle in case no valid command parsed
        pState = IDLE;

        // Parse command character
        for (uint8_t i = 0; i < CMND_NUM; i++) {
          if (c == cmdList[i].c) {
            cmd = cmdList[i];
            // Set next program state based on command's data byte length (d)
            switch (cmd.d) {
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
            break;  // Break for loop
          }
        }
      }
      break;
    }

    // Waiting for command data
    case WAIT_CMD_D: {
      uint8_t i = 0;
      uint8_t j = (transferMode == ASCII) ? cmd.d * 2 : cmd.d;

      // Process command data bytes from serial input
      do {
        if (Serial.available() > 0) {
          cmdBuffer[i++] = Serial.read();
        }
      } while (i < j && millis() - lastMillis < SERIAL_TIMEOUT);

      // If ASCII mode enabled convert data string to binary format
      if (transferMode == ASCII && i == j) {
        for (i = 0; i < j; i+=2) {
          char byteStr[2] = { cmdBuffer[i], cmdBuffer[i + 1] };
          cmdBuffer[i >> 1] = strtol(byteStr, NULL, 16);
        }
      }

      if (i == j) pState = CMD_RUN; else pState = TIMEOUT;

      break;
    }

    // Receive binary data block
    case RECV_DBLOCK: {
      // Process data block from serial input
      uint16_t i = 0;
      uint16_t j = 0;
      char byteStr[2] = "";

      do {
        if (Serial.available() > 0) {
          if (transferMode == ASCII) {
            byteStr[i & 1] = Serial.read();
            if (i & 1 == 1) {
              dataBuffer[j++] = strtol(byteStr, NULL, 16);
            }
            i++;
          }
          else {
            // In binary mode read data directly from serial port
            dataBuffer[j++] = Serial.read();
          }
        }
      } while (j < blockSize && millis() - lastMillis < SERIAL_TIMEOUT);

      if (j == blockSize) pState = CMD_RUN; else pState = TIMEOUT;

      break;
    }

    // Execute command function
    case CMD_RUN: {
      switch(cmd.id) {
        // Command not recognized
        default:
          HALT("Illegal Command");

        // Set BINARY transfer mode
        case SBINRY: {
          transferMode = BINARY;
          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Set ASCII transfer mode
        case SASCII: {
          transferMode = ASCII;
          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Set data block size
        case SDBLSZ: {
          uint16_t word = SerialIO_wordFromCmdBuffer();
          blockSize = (word < DATA_BUFFER_LEN) ? word : DATA_BUFFER_LEN;
          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Get manufacturer and device ID
        case GDEVID: {
          if (transferMode == ASCII) {
            sprintf(strBuffer,"%02x%02x", EPROM_SST_manufacturerID(), EPROM_SST_deviceID());
            Serial.print(strBuffer);
            //Serial.print(EPROM_SST_manufacturerID(), HEX);
            //Serial.print(EPROM_SST_deviceID(), HEX);
          }
          else {
            Serial.write(EPROM_SST_manufacturerID());
            Serial.write(EPROM_SST_deviceID());
          }
          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Set start address
        case SSADDR: {
          uint32_t dword = SerialIO_dwordFromCmdBuffer();
          startAddr = (dword < endAddr) ? dword : endAddr - 1;

          // Initialise CRC-32 and SHA-1 Calculations
          CRC32_init();
          SHA1_init();

          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Set end address of EPROM
        case SEADDR: {
          uint32_t dword = SerialIO_dwordFromCmdBuffer();
          startAddr = (dword < endAddr) ? dword : endAddr;

          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Read single byte from current address
        case RDBYTE: {
          SCR_setAddress(startAddr);
          if (transferMode == ASCII) {
            sprintf(strBuffer, "%02x", EPROM_readByte());
            Serial.print(strBuffer);
          }
          else {
            Serial.write(EPROM_readByte());
          }
          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Read data block from current address
        case RDBLCK: {
          SerialIO_readBlock(false);
          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Write data block to EPROM
        case WDBLCK: {
          bool result = SerialIO_writeBlock(false);
          Serial.write(result ? ACK : N_ACK);
          pState = IDLE;
          break;
        }

        // Erase EPROM bank containing startAddr
        case EEBANK: {
          // Extract dword from command buffer
          uint32_t dword = SerialIO_dwordFromCmdBuffer();
          bool result = false;

          // Check for correct data to validate erase command
          if (dword == EEBANK_CHECK) {
            result = EPROM_SST_sectorErase(startAddr & 0xfffff000, true);
          }

          // Acknowledge success or failure
          Serial.write(result ? ACK : N_ACK);

          pState = IDLE;
          break;
        }

        // Erase entire EPROM
        case EEPROM: {
          // Extract dword from command buffer
          uint32_t dword = SerialIO_dwordFromCmdBuffer();
          bool result = false;

          // Check for correct data to validate erase command
          if (dword == EEPROM_CHECK) {
            result = EPROM_SST_chipErase(true);
          }

          // Acknowledge success or failure
          Serial.write(result ? ACK : N_ACK);

          pState = IDLE;
          break;
        }

        // Return CRC32 hash value
        case GCRC32: {
          Serial.print(CRC32_finalise());
          Serial.write(ACK);
          pState = IDLE;
          break;
        }

        // Return SHA-1 hash value
        case GSHA_1: {
          // Finalise SHA-1 calculation
          SHA1_finalise();
  
          // SHA-1 160-bit final state variables
          uint32_t a,b,c,d,e;

          // Get the SHA-1 final state
          SHA1_getState(&a, &b, &c, &d, &e);
          sprintf(strBuffer, "%08lx%08lx%08lx%08lx%08lx", a, b, c, d, e);
          Serial.print(strBuffer);
          Serial.write(ACK);
          pState = IDLE;
          break;
        }
      }
      break;
    }

    // If timer exceeds timeout value, reset program state to idle
    case TIMEOUT: {
      Serial.write('!');
      pState = IDLE;
    }
  }
}

bool SerialIO_writeBlock(bool suppressOutput) {
  bool result = false;
  uint32_t i = 0;

  for (uint32_t address = startAddr; address < startAddr + blockSize; address++) {
    // Write a byte of data to EPROM
    result = EPROM_SST_byteProgram(address, dataBuffer[i++]);
    
    if (!result) break;

    if (!suppressOutput) {
      uint8_t data = EPROM_readByte();

      if (transferMode == ASCII) {
        sprintf(strBuffer, "%02x", data);
        Serial.print(strBuffer);
      }
      else {
        Serial.write(data);
      }
    }
  }

  return result;
}

void SerialIO_readBlock(bool suppressOutput) {
  // Set start address of block and set register to counter mode
  SCR_setAddress(startAddr);
  SCR_setCounterMode();

  for (uint32_t address = startAddr; address < startAddr + blockSize; address++) {
    // Read a byte of data from EPROM
    uint8_t data = EPROM_readByte();

    // Update the CRC32 and SHA-1 calculation with current data byte
    CRC32_update(data);
    SHA1_update(data);

    // Output the current data byte if needed
    if (!suppressOutput) {
      if (transferMode == ASCII) {
        sprintf(strBuffer, "%02x", data);
        Serial.print(strBuffer);
      }
      else {
        Serial.write(data);
      }
    }

    // Pulse clock of shift/counter register to increment counter
    SCR_clockPulse();
  }

  // Set start address to begining of next block
  startAddr += blockSize & (endAddr - 1);
}

uint32_t SerialIO_dwordFromCmdBuffer() {
  return (uint32_t)cmdBuffer[0] << 24 | (uint32_t)cmdBuffer[1] << 16 | (uint32_t)cmdBuffer[2] << 8 | (uint32_t)cmdBuffer[3];
}

uint16_t SerialIO_wordFromCmdBuffer() {
  return (uint16_t)cmdBuffer[0] << 8 | (uint16_t)cmdBuffer[1];
}