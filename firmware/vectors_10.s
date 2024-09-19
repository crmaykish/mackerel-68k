    .align 2

    .section .vectors, #alloc

    .long 0x900000              | Initial stack pointer
    .long _start                | Program counter value
