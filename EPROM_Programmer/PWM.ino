// Enable T2B FastPWM 50% at 20KHz
void PWM_init() {
  // Enable Timer-2 Output-B, Set FastPWM Mode,
  // and set clock divider to 8.
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS21);

  // Freq = 16MHz / 8 / (99 + 1) = 20KHz
  //    % = (49 + 1) / (99 + 1)  = 50%

  // Set OCR2A for TOP
  OCR2A = 99;
  // Set OCR2B for duty cycle
  OCR2B = 49;

  // Enable output on pin 3
  pinMode(3, OUTPUT);
}