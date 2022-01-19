    .align 2

    | ACIA memory-mapped registers
    .equ ACIA_DATA, 0xC0000
    .equ ACIA_STATUS, 0xC0001
    .equ ACIA_COMMAND, 0xC0002
    .equ ACIA_CONTROL, 0xC0003

    | ACIA status flags
    .equ ACIA_TX_READY, 0x10
    .equ ACIA_RX_READY, 0x08

    .section .vectors, #alloc

resetsp:    .long 0xBF000
resetpc:    .long _start

    .section .text

_start:
    bsr.s _acia_init
    
_loop:
    bsr.s _getc
    bsr.s _putc
    bra.s _loop

_acia_init:
    | Reset the ACIA
    move.b #0x00, ACIA_STATUS

    | Send the ACIA init command
    move.b #0x0B, ACIA_COMMAND

    | Set baudrate to 1/16 oscillator, no parity, 1 stop bit
    move.b #0b00010000, ACIA_CONTROL
    rts

_getc:
    | Wait for serial input data
    move.b ACIA_STATUS, %d1
    andi.b #ACIA_RX_READY, %d1
    cmpi.b #0x00, %d1
    beq.s _getc

    | Read byte from serial input
    move.b ACIA_DATA, %d0

    rts

_putc:
    | Wait for TX ready status
    move.b ACIA_STATUS, %d1
    andi.b #ACIA_TX_READY, %d1 
    cmpi.b #0x00, %d1
    beq.s _putc

    | Echo input byte to serial output
    move.b %d0, ACIA_DATA

    rts
