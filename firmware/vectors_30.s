    .align 2

    .section .vectors, "a"

    .long 0x08000000 - 0x1000   | Initial stack pointer
    .long _start                | Program counter value
