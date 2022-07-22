#include "mackerel.h"

extern int main();

// Something terrible has happened, stop everything and wait
void panic(const char *err)
{
    mfp_puts("\r\nPANIC: ");
    mfp_puts(err);
    mfp_puts("\r\n");

    while (1) {}
}

void __attribute((interrupt)) exception_unhandled() { MEM(MFP_GPDR) = 0xAA; panic("Unhandled exception"); }
void __attribute((interrupt)) exception_bus_error() { MEM(MFP_GPDR) = 2; panic("Bus error"); }
void __attribute((interrupt)) exception_addr_error() { MEM(MFP_GPDR) = 3; panic("Address error"); }
void __attribute((interrupt)) exception_illegal_inst() { MEM(MFP_GPDR) = 4; panic("Illegal instruction"); }
void __attribute((interrupt)) exception_div_zero() { MEM(MFP_GPDR) = 5; panic("Divide by zero"); }
void __attribute((interrupt)) exception_uninit_int_vector() { MEM(MFP_GPDR) = 15; panic("Uninitialized interrupt vector"); }

void _start()
{
    // TODO: Setup .bss and .data sections correctly in RAM

    // Disable interrupts
    set_interrupts(false);

    // Initialize the vector table at the start of RAM
    for (int i = 0; i < VECTOR_TABLE_SIZE; i += 1)
    {
        // All exceptions start off pointing to a known handler
        set_exception_handler(i, &exception_unhandled);
    }

    set_exception_handler(2, exception_bus_error);
    set_exception_handler(3, exception_addr_error);
    set_exception_handler(4, exception_illegal_inst);
    set_exception_handler(5, exception_div_zero);

    set_exception_handler(15, exception_uninit_int_vector);

    // Setup the hardware peripherals
    mfp_init();

    // Call main
    main();
}
