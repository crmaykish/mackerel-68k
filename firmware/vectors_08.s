    .align 2

    .section .vectors, "a"

    .long 0x380000              | Initial stack pointer
    .long _start                | Program counter value
