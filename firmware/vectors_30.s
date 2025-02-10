    .align 2

    .section .vectors, "a"

    .long 0x80000               | Initial stack pointer
    .long _start                | Program counter value
