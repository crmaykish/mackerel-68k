    .section .vectors, #alloc

resetsp:    .long 0x8000
resetpc:    .long _start

    .section .text

_start:
    nop
    nop
    nop
    jmp _start
