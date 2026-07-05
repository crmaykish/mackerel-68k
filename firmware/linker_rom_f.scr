MEMORY
{
    ROM(RX) : ORIGIN = 0xFF0000,  LENGTH = 0xC000             /* 48 KB ROM region */
    RAM(RW) : ORIGIN = 0xFFC000,  LENGTH = 0x3800             /* 14 KB BSRAM for the bootloader (top 2 KB = peripherals) */
}

SECTIONS
{
    .vectors 0xFF0000 : { *(.vectors) } > ROM
    .text   : { *(.text*) } > ROM
    .rodata : { *(.rodata*) } > ROM
    .data : {
        _sdata = .;
        *(.data*)
        *(.sdata*)
        _edata = .;
    } > RAM AT > ROM
    _sidata = LOADADDR(.data);
    .bss (NOLOAD) : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM
    _end = .;
    _heap_end = ORIGIN(RAM) + LENGTH(RAM);
    stack_top = ORIGIN(RAM) + LENGTH(RAM);

    /* RAM info for the bootloader's `info` command */
    __sram_start = 0x00;
    __sram_length = 0x00;
    __dram_start = 0x00;
    __dram_length = 0x800000;
}
