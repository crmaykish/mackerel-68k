    .align 2

    .section .vectors, #alloc

resetsp:    .long 0x80000
resetpc:    .long _start
