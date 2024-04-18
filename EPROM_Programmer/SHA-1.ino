// SHA-1 hash variables
uint32_t h0;
uint32_t h1;
uint32_t h2;
uint32_t h3;
uint32_t h4;

// SHA-1 Constants, K0, K1, K2, and K3
#define k1 0x5a827999
#define k2 0x6ed9eba1
#define k3 0x8f1bbcdc
#define k4 0xca62c1d6

//static const uint32_t k[4] = {0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6};

uint32_t  w[16];            // SHA-1 message 32-bit words, w0, w1, ..., w79
uint8_t   sha1_buffer[64];  // SHA-1 512 bit message block
uint64_t  sha1_msgLen = 0;  // SHA-1 message length in bits

// SHA-1 Macros
#define ROL32(v, b) (((v) << (b)) | ((v) >> (32 - (b))))
#define W(n) w[(n) & 0x0f]
#define CH(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define PARITY(x, y, z) ((x) ^ (y) ^ (z))
#define MAJ(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))

void SHA1_init() {
  // Initialise starting hash values
  h0 = 0x67452301;
  h1 = 0xefcdab89;
  h2 = 0x98badcfe;
  h3 = 0x10325476;
  h4 = 0xc3d2e1f0;

  // Reset message length to zero
  sha1_msgLen = 0;

  // Set first bit of buffer, then fill rest with zeros
  sha1_buffer[0] = 0x80;
  for (uint8_t i = 1; i < 64; i++) {
    sha1_buffer[i] = 0;
  }
}

void SHA1_update(uint32_t byte) {
  // Calculate index into SHA-1 message buffer
  uint8_t i = (sha1_msgLen >> 3) & 0x3f;

  // Add byte to SHA-1 message buffer
  sha1_buffer[i] = byte;

  // Increment message length by 8 bits
  sha1_msgLen += 8;

  // If block is complete, process it
  if (i == 0x3f) SHA1_processBlock();
}

void SHA1_processBlock() {
  uint32_t temp;

  // Get current SHA-1 state
  uint32_t a = h0;
  uint32_t b = h1;
  uint32_t c = h2;
  uint32_t d = h3;
  uint32_t e = h4;

  // Convert SHA-1 512 bit block into 16 32-bit big-endian integers
  for (uint8_t i = 0; i < 16; i++) {
    w[i]  = ((uint32_t) sha1_buffer[i * 4 + 0]) << 24;
    w[i] += ((uint32_t) sha1_buffer[i * 4 + 1]) << 16;
    w[i] += ((uint32_t) sha1_buffer[i * 4 + 2]) << 8;
    w[i] += ((uint32_t) sha1_buffer[i * 4 + 3]) << 0;
  }

  // SHA-1 Hash computation
  for (uint8_t i = 0; i < 80; i++) {
    // Prepare the message schedule
    if (i >= 16) {
      W(i) = ROL32(W(i + 13) ^ W(i + 8) ^ W(i + 2) ^ W(i), 1);
    }

    // Calculate SHA-1 temp value
    if (i < 20) {
      temp = ROL32(a, 5) + CH(b, c, d)      + e + W(i) + k1;
    }
    else if (i < 40) {
      temp = ROL32(a, 5) + PARITY(b, c, d)  + e + W(i) + k2;
    }
    else if (i < 60) {
      temp = ROL32(a, 5) + MAJ(b, c, d)     + e + W(i) + k3;
    }
    else {
      temp = ROL32(a, 5) + PARITY(b, c, d)  + e + W(i) + k4;
    }

    // Update working registers
    e = d;
    d = c;
    c = ROL32(b, 30);
    b = a;
    a = temp;
  }

  // Update SHA-1 state
  h0 += a;
  h1 += b;
  h2 += c;
  h3 += d;
  h4 += e;
}

void SHA1_getState(char *str) {
  // Return state as ASCII character string
  sprintf(str, "%08lx%08lx%08lx%08lx%08lx", h0, h1, h2, h3, h4);
}

void SHA1_getState(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d, uint32_t *e) {
  // Return state as five 32 bit integers
  *a = h0;
  *b = h1;
  *c = h2;
  *d = h3;
  *e = h4;
}

uint32_t SHA1_finalise() {
  // TO DO
  // Need to account for a buffer that is not a full block of 512 bits

  // Set first bit of SHA-1 block, set remaining to zero
  sha1_buffer[0] = 0x80;
  for (uint8_t i = 1; i < 56; i++) {
    sha1_buffer[i] = 0;
  }

  // Add message length in bits to end of block as big-endian 32 bit integer
  sha1_buffer[56] = (sha1_msgLen & 0xff00000000000000LL) >> 56;
  sha1_buffer[57] = (sha1_msgLen & 0x00ff000000000000LL) >> 48;
  sha1_buffer[58] = (sha1_msgLen & 0x0000ff0000000000LL) >> 40;
  sha1_buffer[59] = (sha1_msgLen & 0x000000ff00000000LL) >> 32;
  sha1_buffer[60] = (sha1_msgLen & 0x00000000ff000000LL) >> 24;
  sha1_buffer[61] = (sha1_msgLen & 0x0000000000ff0000LL) >> 16;
  sha1_buffer[62] = (sha1_msgLen & 0x000000000000ff00LL) >> 8;
  sha1_buffer[63] = (sha1_msgLen & 0x00000000000000ffLL) >> 0;

  // Process final block
  SHA1_processBlock();
}