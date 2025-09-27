#include "mackerel.h"
#include "newlib_init.h"

extern char _sidata[], _sdata[], _edata[];

extern int main();

void panic(const char *err)
{
    duart_puts("\r\nPANIC: ");
    duart_puts(err);
    duart_puts("\r\n");

    while (1)
    {
    }
}

void __attribute((interrupt)) exception_unhandled() { panic("Unhandled exception"); }
void __attribute((interrupt)) exception_bus_error() { panic("Bus error"); }
void __attribute((interrupt)) exception_addr_error() { panic("Address error"); }
void __attribute((interrupt)) exception_illegal_inst() { panic("Illegal instruction"); }
void __attribute((interrupt)) exception_div_zero() { panic("Divide by zero"); }
void __attribute((interrupt)) exception_chk() { panic("Chk instruction out of bounds"); }
void __attribute((interrupt)) exception_trapv() { panic("Trap v"); }
void __attribute((interrupt)) exception_priv_violation() { panic("Privilege violation"); }
void __attribute((interrupt)) exception_unimp_inst() { panic("Unimplemented instruction"); }
void __attribute((interrupt)) exception_uninit_int_vector() { panic("Uninitialized interrupt vector"); }
void __attribute((interrupt)) exception_spurious_intr() { panic("Spurious interrupt"); }
void __attribute((interrupt)) autovector() { panic("Autovector"); }
void __attribute((interrupt)) trap() { panic("Trap"); }
void __attribute((interrupt)) user_interrupt() { panic("User interrupt"); }

void _start()
{
    // Disable interrupts
    set_interrupts(false);

    set_exception_handler(0, 0);
    set_exception_handler(1, 0);
    set_exception_handler(2, exception_bus_error);
    set_exception_handler(3, exception_addr_error);
    set_exception_handler(4, exception_illegal_inst);
    set_exception_handler(5, exception_div_zero);
    set_exception_handler(6, exception_chk);
    set_exception_handler(7, exception_trapv);
    set_exception_handler(8, exception_priv_violation);
    set_exception_handler(10, exception_unimp_inst);
    set_exception_handler(11, exception_unimp_inst);
    set_exception_handler(15, exception_uninit_int_vector);
    set_exception_handler(24, exception_spurious_intr);

    for (int i = 25; i < 32; i += 1)
    {
        set_exception_handler(i, &autovector);
    }

    // Trap block
    for (int i = 32; i < 48; i += 1)
    {
        set_exception_handler(i, &trap);
    }

    // User interrupts
    for (int i = 64; i <= 255; i += 1)
    {
        set_exception_handler(i, &user_interrupt);
    }

    // Copy initialized data from flash to RAM
    for (char *src = _sidata, *dst = _sdata; dst < _edata;)
        *dst++ = *src++;

    duart_init();
    newlib_init();
    main();
}
