import serial

def readLine():
    response = arduino.read_until(b'\n').decode()
    print(response.strip())

def sendCommand(s: str) -> str:
    arduino.write(('z' + s).encode('utf-8'))
    response = arduino.read(1).decode('utf-8')
    if (response == 'Z'):
        message = arduino.read_until(b'@').decode().strip('@')
    else:
        message = b'!'
    return message

arduino = serial.Serial(port='/dev/tty.usbmodem14101', baudrate=57600, timeout=11)

# Read text lines to clear Arduino message text from serial input
readLine()
readLine()
readLine()
readLine()
readLine()
readLine()

deviceID = sendCommand('i')         # Get manufacturer and device id
print('Device ID:' + deviceID)      # Print device id

sendCommand('d0010')                # Set data block size to 16
sendCommand('s00000000')            # Set start address to 0x00000000

# Print first 256 bytes of EPROM
sendCommand('s00000000')            # Set start address to 0x00000000
for i in range(16):
    print(sendCommand("R"))         # Read and print 16-byte data block from EPROM

sendCommand('Ea5a5a5a5')            # Send erase EPROM command

sendCommand('s00000000')            # Set start address to 0x00000000
sendCommand('w'.ljust(33, 'a'))     # Write 16 bytes of 0xaa to EPROM
sendCommand('s00000010')            # Set start address to 0x00000010
sendCommand('w'.ljust(33, '5'))     # Write 16 bytes of 0x55 to EPROM

# Print first 256 bytes of EPROM
sendCommand('s00000000')            # Set start address to 0x00000000
for i in range(16):
    print(sendCommand("R"))         # Read and print 16-byte data block from EPROM
