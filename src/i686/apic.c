#include <stdint.h>
#include "paging.h"
#include "../stdio.h"

#define MSR_APIC_BASE 0x1B
#define APIC_ENABLE (1 << 11)

// APIC Register Offsets
#define APIC_SVR            0xF0    // Spurious Interrupt Vector Register
#define APIC_EOI            0xB0    // End of Interrupt
#define APIC_TIMER_REGISTER 0x320   // Local APIC Timer Register
#define APIC_TIMER_INITIAL  0x380   // Timer Initial Count Register
#define APIC_TIMER_CURRENT  0x390   // Timer Current Count Register
#define APIC_TIMER_DIVIDER  0x3E0   // Timer Divide Configuration Register

// Configuration Values
#define APIC_TIMER_PERIODIC 0x20000 // Set Timer to Periodic Mode
#define APIC_DIV_16         0x3     // Divide by 16 for timer frequency
#define APIC_TIMER_VECTOR   0x20    // Interrupt vector for the timer
#define APIC_SVR_VECTOR     0xFF    // Spurious interrupt vector

uintptr_t apic_base = 0;

void read_msr(uint32_t msr, uint32_t* lo, uint32_t* hi) 
{
    asm volatile ("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void write_msr(uint32_t msr, uint32_t lo, uint32_t hi) 
{
    asm volatile ("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

// Write to an APIC register
void write_apic(uint32_t reg, uint32_t value) 
{
    volatile uint32_t* addr = (volatile uint32_t*)(apic_base + reg);
    *addr = value;
    asm volatile("" : : : "memory");  // Memory barrier
}

// Read from an APIC register
uint32_t read_apic(uint32_t reg) 
{
    volatile uint32_t* addr = (volatile uint32_t*)(apic_base + reg);
    return *addr;
}

// Enable the APIC
void enable_apic()
{
    uint32_t lo, hi;
    read_msr(MSR_APIC_BASE, &lo, &hi);

    // Set APIC enable flag
    lo |= APIC_ENABLE;
    write_msr(MSR_APIC_BASE, lo, hi);

    // Enable the APIC in the Spurious Interrupt Vector Register (SVR)
    write_apic(APIC_SVR, APIC_SVR_VECTOR | 0x100);
}

// Send End of Interrupt (EOI) to APIC
void apic_send_eoi() 
{
    write_apic(APIC_EOI, 0);
}

// Initialize APIC Timer
void setup_apic_timer(uint32_t interval) 
{
    // Set the timer divisor to 16
    write_apic(APIC_TIMER_DIVIDER, APIC_DIV_16);

    // Set the timer mode to periodic and assign the interrupt vector
    write_apic(APIC_TIMER_REGISTER, APIC_TIMER_PERIODIC | APIC_TIMER_VECTOR);

    // Set the initial count for the timer, determining the interval
    write_apic(APIC_TIMER_INITIAL, interval);
}


void init_apic() 
{
    apic_base = (uintptr_t)map_page_table(0xFEE00000);
    enable_apic(); // Enable the APIC
}

uintptr_t get_apic_virtual_base()
{
    return apic_base;
}