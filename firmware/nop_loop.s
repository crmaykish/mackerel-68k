    .equ ACIA_DATA, 0x80000
    .equ ACIA_STATUS, 0x80001
    .equ ACIA_COMMAND, 0x80002
    .equ ACIA_CONTROL, 0x80003

    .section .vectors, #alloc

resetsp:    .long 0xF0000
resetpc:    .long _start

    .section .text

_start:
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
    move.b #0b00010000, ACIA_CONTROL
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

_restart:
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
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
_loop:
    move.b %d0, ACIA_DATA
    add #1, %d0

    move.w #0x0000, %d1

_delay:
    addi.w #1, %d1
    cmpi.w #0x1000, %d1
    bne.s _delay

    cmpi.b #'Z'+1, %d0
    beq.s _restart

    jmp _loop
