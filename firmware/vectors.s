    .align 2

    .section .vectors, #alloc

resetsp:    .long 0x400000
resetpc:    .long _start
