    .align 2

    .section .vectors, "a"

    .long 0xC0080000 - 0x1000   | Initial stack pointer (top of 512KB SRAM, minus 4KB guard)
    .long _start                | Program counter value
