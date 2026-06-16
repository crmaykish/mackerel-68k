    .align 2

    .section .vectors, "a"

    .long 0x800000             | Initial stack pointer (top of 8 MB SDRAM; grows down)
    .long _start               | Program counter value
