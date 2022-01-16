    .equ ACIA_DATA, 0x80000
    .equ ACIA_STATUS, 0x80001
    .equ ACIA_COMMAND, 0x80002
    .equ ACIA_CONTROL, 0x80003

    .equ ACIA_TX_READY, 0x10
    .equ ACIA_RX_READY, 0x08

    .section .vectors, #alloc

resetsp:    .long 0xF0000
resetpc:    .long _start

    .section .text

_start:
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    | Reset the ACIA
    move.b #0x00, ACIA_STATUS
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    | Send the ACIA init command
    move.b #0x0B, ACIA_COMMAND
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    | Set baudrate to 1/16 oscillator, no parity, 1 stop bit
    move.b #0b00010000, ACIA_CONTROL

_restart:
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    move.b #0x0A, ACIA_DATA
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    move.b #0x0D, ACIA_DATA
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    move.b #'A', %d0

_loop:
    | Wait for TX ready status
    move.b ACIA_STATUS, %d2
    andi.b #ACIA_TX_READY, %d2 
    cmpi.b #0x00, %d2
    beq.s _loop

    | Write byte to ACIA
    move.b %d0, ACIA_DATA

    | Increment output byte
    add #1, %d0

    | If byte is out of range, start over
    cmpi.b #'Z'+1, %d0
    beq.s _restart

    | Send next byte
    jmp _loop
