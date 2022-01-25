    .align 2

    .section .vectors, #alloc

resetsp:    .long 0x100000
resetpc:    .long _start
