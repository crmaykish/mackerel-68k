    .align 2

    .section .vectors, #alloc

    .long 0x8000                | Initial stack pointer
    .long _start                | Program counter value
