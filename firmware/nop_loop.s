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
    move.b #0xAA, PORTA
_loop:
    jmp _loop
    