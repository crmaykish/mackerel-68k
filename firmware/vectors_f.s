    .align 2

    .section .vectors, "a"

    .long 0x2000               | Initial stack pointer
    .long _start               | Program counter value
