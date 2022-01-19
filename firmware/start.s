    .align 2

    .section .vectors, #alloc

resetsp:    .long 0xBF000
resetpc:    .long _start

    .section .text

_start:

    | TODO: Clear .bss section
    | TODO: Copy .data from ROM to RAM

    jsr main
    bra _start
