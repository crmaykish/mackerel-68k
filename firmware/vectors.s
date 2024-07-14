    .align 2

    .section .vectors, #alloc

    .long 0x300000              | Initial stack pointer
    .long _start                | Program counter value
