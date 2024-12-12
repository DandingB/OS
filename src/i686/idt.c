#include "../stdio.h"
#include "idt.h"
#include "x86.h"
#include "pic.h"
#include "apic.h"

void key_press(uint8_t key);


__attribute__((aligned(0x10)))
static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance
static idtr_t idtr;

extern void* isr_stub_table[];




int sd = 0;

void interrupt_handler(uint32_t interrupt, uint32_t error)
{
	if (interrupt == 0x40)
		print("AHCI interrupt", 1);

	if (interrupt == 0x41) 
		print("XHCI interrupt", 1);

	if (interrupt == 0x20) // Timer interrupt
	{
		print_hexdump(&sd, 1, 24);
		sd++;
	}

	if (interrupt >= 32)
	{
		uint8_t status = inb(0x64);		// Status bits: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-11.html
		uint8_t scancode = inb(0x60);

		if (status & 0x1) // Check output buffer status. 1 == Output buffer full, can be read
			key_press(scancode);

		if (interrupt >= 8)
			outb(PIC_2_CTRL, PIC_CMD_END_OF_INTERRUPT);
		outb(PIC_1_CTRL, PIC_CMD_END_OF_INTERRUPT);

		//if (interrupt < 0x28) {
		//	outb(PIC_1_CTRL, PIC_CMD_END_OF_INTERRUPT);
		//}
		//else {
		//	outb(PIC_2_CTRL, PIC_CMD_END_OF_INTERRUPT);
		//}
		

		apic_send_eoi();
	}
	else
	{
		print("Exception!", 1);
		print_hexdump(&interrupt, 1, 2);
		__asm__ volatile ("cli; hlt"); // Completely hangs the computer
	}
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags)
{
	idt_entry_t* descriptor = &idt[vector];

	descriptor->isr_low = (uint32_t)isr & 0xFFFF;
	descriptor->kernel_cs = 0x08; // this value can be whatever offset your kernel interrupt selector is in your GDT
	descriptor->attributes = flags;
	descriptor->isr_high = (uint32_t)isr >> 16;
	descriptor->reserved = 0;
}

void init_idt() 
{
	idtr.base = (uintptr_t)&idt[0];
	idtr.limit = (uint16_t)sizeof(idt_entry_t) * 256 - 1;

	for (uint8_t i = 0; i < 255; i++) 
	{
		idt_set_descriptor(i, isr_stub_table[i], 0x8E);
	}


	__asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
	__asm__ volatile ("sti"); // set the interrupt flag
}