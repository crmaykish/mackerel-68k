    | ACIA memory-mapped registers
    .equ ACIA_DATA, 0x80000
    .equ ACIA_STATUS, 0x80001
    .equ ACIA_COMMAND, 0x80002
    .equ ACIA_CONTROL, 0x80003

    | ACIA status flags
    .equ ACIA_TX_READY, 0x10
    .equ ACIA_RX_READY, 0x08

    .section .vectors, #alloc

resetsp:    .long 0x0000    | Unused, no RAM installed
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

_loop:
    | Wait for serial input data
    move.b ACIA_STATUS, %d0
    andi.b #ACIA_RX_READY, %d0
    cmpi.b #0x00, %d0
    beq.s _loop

    | Read byte from serial
    move.b ACIA_DATA, %d1

_wait_to_write:
    | Wait for TX ready status
    move.b ACIA_STATUS, %d0
    andi.b #ACIA_TX_READY, %d0 
    cmpi.b #0x00, %d0
    beq.s _wait_to_write

    | Echo input byte back to serial output
    move.b %d1, ACIA_DATA

    jmp _loop
