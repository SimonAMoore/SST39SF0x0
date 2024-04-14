const uint32_t CRC32_POLY = 0xedb88320; // Polynomial for calculating CRC-32 checksum
      uint32_t crc32;

void CRC32_init() {
  crc32 = 0xffffffff;
}

void CRC32_update(uint32_t byte) {
  crc32 = crc32 ^ byte;

  for (uint8_t i = 0; i < 8; i++) {
    crc32 = (crc32 >> 1) ^ (CRC32_POLY & -(crc32 & 1));
  }
}

uint32_t CRC32_finalise() {
  return crc32 ^ 0xffffffff;
}