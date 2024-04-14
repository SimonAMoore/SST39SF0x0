; EPROM Programmer, shift register and counter for address lines on 2764 EPROM
;
; (C) Simon Moore 2024. All Rights Reserved.
;
; Date: 24th February 2024.
;
;
;		    +--------------------------+
;	 MODE_IN -> :  1 RE3		RB7 28 : -> A12
;	     A13 <- :  2 RA0		RB6 27 : -> A11
;	     A14 <- :  3 RA1		RB5 26 : -> A10
;	     A15 <- :  4 RA2		RB4 25 : -> A9
;	     A16 <- :  5 RA3		RB3 24 : -> A8
;	     A17 <- :  6 RA4		RB2 23 : -> A7
;	     A18 <- :  7 RA5		RB1 22 : -> A6
;	VSS (0V) -> :  8		RB0 21 : -> A5
;		    :  9		    20 : <- VDD (5V)
;      Clock Out <- : 10 RA6		    19 : <- VSS (0V)
;	      A0 <- : 11 RC0		RC7 18 : -> A4
;	      A1 <- : 12 RC1		RC6 17 : -> A3
;	      A2 <- : 13 RC2		RC5 16 : <- CLK_IN!
;		    : 14		RC4 15 : <- DATA_IN!
;		    +--------------------------+
;
;   Rom Bank	    Rom Address High			Rom Address Low
;   +-----------+   +-------------------------------+	+-------------------------------+
;   |RA5|RA4|RA3|   |RA2|RA1|RA0|RB7|RB6|RB5|RB4|RB3|	|RB2|RB1|RB0|RC7|RC6|RC2|RC1|RC0|
;   +-----------+   +-------------------------------+	+-------------------------------+

PROCESSOR 18F2550 

#include "config.inc"
#include <xc.inc>

; Data section for program variables
PSECT udata_acs

; Counter for delay sub-routine
delayH:
    DS  1
delayL:
    DS  1

; Address output variable (Maps 19-bit address to [LSB]RC0:2,6:7 RB0:7 RA0:5[MSB])
addrOut_Bank:
    DS	1   // Rom Bank 0 to 7
addrOut_H:
    DS	1   // MSB
addrOut_L:
    DS	1   // LSB

; Address input variable (19 Bits, 0x00000-0x7ffff)
addrIn_Bank:
    DS	1   // Rom Bank 0 to 7
addrIn_H:
    DS	1   // MSB
addrIn_L:
    DS	1   // LSB

; Shift/Counter mode selection variables
;
; mode = 0, data = x : Default shift register (Shifts data 'x' into register every clock pulse)
; mode = 1, data = 1 : Counter Mode Increment (Increments register every clock pulse)
; mode = 1, data = 0 : Counter Mode Decrement (Decrements register every clock pulse)

mode:
    DS	1

; Define Pin names
#define CLOCK_IN    RC5
#define DATA_IN	    RC4
#define MODE_IN	    RE3

; Program section for reset vector at address 000000h
PSECT resetVec, reloc=2
resetVec:
    PAGESEL main
    goto    main

; Program section for high priority interrut vector at address 000008h
PSECT hiIntVec, reloc=2
hiIntVec:
    retfie

; Program section for low priority interrut vector at address 000018h
PSECT loIntVec, reloc=2
loIntVec:
    retfie

; Program section for main code block
PSECT code

main:
    ; Initialise internal oscillator settings (16 MHz)
    movlw   01110010B
    movwf   OSCCON, c
    movlw   10000000B
    movwf   OSCTUNE, c
 
    ; Disable USB module
    clrf    UCON, c
    movlw   00001000B
    movwf   UCFG, c

    ; Initialise PORTA for digital IO, output on RA0:5
    clrf    PORTA, c
    clrf    LATA, c
    ; Set all PORTA pins to digital IO
    movlw   0x0f
    movwf   ADCON1, c
    ; Switch comparators off
    movlw   0x07	
    movwf   CMCON, c
    ; Enable PORTA pins as outputs
    movlw   00000000B
    movwf   TRISA, c

    ; Initialise PORTB and PORTC as all outputs (Input ONLY on RC4, RC5)
    clrf    PORTB, c
    clrf    LATB, c
    clrf    TRISB, c
    clrf    PORTC, c
    clrf    LATC, c
    clrf    TRISC, c

    ; Clear address input variable
    clrf    addrIn_Bank, b
    clrf    addrIn_H, b
    clrf    addrIn_L, b

    ; Set default shift/counter mode (mode 0 = shift mode)
    clrf    mode, b

main_loop:
    call    pollButtons
    call    updateOutput
    goto    main_loop

pollButtons:
    ; Test for clock-pulse, loop if not
    btfsc   CLOCK_IN
    goto    pollButtons

    ; Check for set mode input, skip if set
    btfss   MODE_IN
    goto    operatingMode

setMode:
    ; Read DATA_IN input into mode register
    clrf    mode, b
    btfss   DATA_IN
    goto    setModeReset
    bsf	    mode, 0, b

    goto    pollButtonsSkip

setModeReset:
    ; Reset mode and registers to power on defaults
    ;clrf    mode, b
    clrf    addrIn_L, b
    clrf    addrIn_H, b
    clrf    addrIn_Bank, b
    ;call    updateOutput

    goto    pollButtonsSkip

operatingMode:
    ; Check which operating mode is active
    btfsc   mode, 0, b
    goto    operatingMode_1

operatingMode_0:
    ; Clear carry flag
    bcf	    CARRY
    ; Check state of data input
    btfss   DATA_IN
    ; If data-in is high, set carry flag
    bsf	    CARRY
    ; Shift address register left with carry flag
    rlcf    addrIn_L, b
    rlcf    addrIn_H, b
    rlcf    addrIn_Bank, b

    goto    pollButtonsSkip

operatingMode_1:
    ; Test for count direction from data input
    btfss   DATA_IN
    goto    operatingMode_1_DEC

operatingMode_1_INC:
    ; Increment address register by 1
    movlw   0x01
    addwf   addrIn_L, b
    movlw   0x00
    addwfc  addrIn_H, b
    addwfc  addrIn_Bank, b

    goto    pollButtonsSkip

operatingMode_1_DEC:
    ; Decrement address register by 1
    movlw   0x01
    subwf   addrIn_L, b
    movlw   0x00
    subfwb  addrIn_H, b
    subfwb  addrIn_Bank, b
    goto    pollButtonsSkip
    
pollButtonsSkip:
    ; Wait for clock signal to go low
    btfss   CLOCK_IN
    goto    pollButtonsSkip

    return

updateOutput:
    ; Transfer address input to output variable
    movff   addrIn_L, addrOut_L
    movff   addrIn_H, addrOut_H
    movff   addrIn_Bank, addrOut_Bank
    ; Clear output latches
    clrf    LATA, c
    clrf    LATB, c
    clrf    LATC, c
    ; Mask out lower 3 bits of addrOut_L for RC2:0
    movlw   0x07
    andwf   addrOut_L, b, w
    iorwf   LATC, c
    ; Clear carry flag
    bcf	    CARRY
    ; Shift address bits 3 left to skip over RC5:3
    rlcf    addrOut_L, b
    rlcf    addrOut_H, b
    rlcf    addrOut_Bank, b
    rlcf    addrOut_L, b
    rlcf    addrOut_H, b
    rlcf    addrOut_Bank, b
    rlcf    addrOut_L, b
    rlcf    addrOut_H, b
    rlcf    addrOut_Bank, b
    ; Mask out top 2 bits of addrOut_L for RC7:6
    movlw   0xc0
    andwf   addrOut_L, b, w
    iorwf   LATC, c
    ; Output addrOut_H to PORTB latch
    movff   addrOut_H, LATB
    ; Output addrOut_Bank to PORTA latch
    movff   addrOut_Bank, LATA

    return

    ; Delay Function
delay:
    decfsz  delayL, b
    goto    delay
    decfsz  delayH, b
    goto    delay
    return

    
; This fix is needed because the K128 programmer has a bug when loading hex files
; with data that is not word aligned. This PSECT fills the one byte gaps at
; CONFIG3L (0x300004) and CONFIG4H (0x300007) to ensure the hex file data is contiguous
PSECT fixConfig, reloc=2, abs
org 300004h
    DB	    000h
org 300007h
    DB	    000h

END resetVec