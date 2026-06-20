MEMORY
{
    /* SDRAM below the ROMfs region at 0x7A0000 (the bootloader's own RAM lives in BSRAM)
       Bare metal programs (and Linux kernel) load at PROGRAM_START (0x400) */
    RAM(RWX) : ORIGIN = 0x400,  LENGTH = 0x7A0000 - 0x400
}

SECTIONS
{
    .text   : { *(.text*) } > RAM
    .rodata : { *(.rodata*) } > RAM
    /* Flat binary loaded at 0x400, so .data sits at its VMA already (no copy);
       newlib_init() zeroes .bss. */
    .data : {
        _sdata = .;
        *(.data*)
        *(.sdata*)
        _edata = .;
    } > RAM
    .bss (NOLOAD) : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM
    _end = .;
    _heap_end = ORIGIN(RAM) + LENGTH(RAM);
}
