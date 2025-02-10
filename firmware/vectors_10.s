    .align 2

    .section .vectors, "a"

    .long 0xF00000              | Initial stack pointer
    .long _start                | Program counter value
