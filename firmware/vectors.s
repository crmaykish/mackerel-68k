    .align 2

    .section .vectors, #alloc

resetsp:    .long 0xC0000
resetpc:    .long _start
