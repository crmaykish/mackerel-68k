    .align 2

    .section .vectors, #alloc

    .long 0x100000              | Initial stack pointer
    .long _start                | Program counter value
