    .equ PORTB, 0x80000
    .equ PORTA, 0x80001
    .equ DDRB, 0x80002
    .equ DDRA, 0x80003

    .section .vectors, #alloc

resetsp:    .long 0xF0000
resetpc:    .long _start

    .section .text

_start:
    move.b #0xFF, DDRB
    move.b #0xFF, DDRA
    move.b #0xAA, PORTB

    move.w #0, %d0
_loop:
    move.b %d0, PORTA
    add #1, %d0

    move.w #0x0000, %d1

_delay:
    addi.w #1, %d1
    cmpi.w #0x1000, %d1
    bne.s _delay

    jmp _loop
