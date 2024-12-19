#include "pic.h"
#include "x86.h"

void init_pic()
{
	/* ICW1 - begin initialization */
	outb(PIC_1_CTRL, ICW1_INIT | ICW1_ICW4);
	outb(PIC_2_CTRL, ICW1_INIT | ICW1_ICW4);

	/* ICW2 - remap offset address of idt_table */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	outb(PIC_1_DATA, 0x20);
	outb(PIC_2_DATA, 0x28);

	/* ICW3 - setup cascading */
	outb(PIC_1_DATA, 4);
	outb(PIC_2_DATA, 2);

	/* ICW4 - environment info */
	outb(PIC_1_DATA, ICW4_8086);
	outb(PIC_2_DATA, ICW4_8086);
	/* Initialization finished */

	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	outb(PIC_1_DATA, 0b11111101);
	outb(PIC_2_DATA, 0b11111111);
}

void disable_pic()
{
	outb(PIC_1_CTRL, ICW1_INIT | ICW1_ICW4);
	outb(PIC_2_CTRL, ICW1_INIT | ICW1_ICW4);

	outb(PIC_1_DATA, 0x20);
	outb(PIC_2_DATA, 0x28);

	outb(PIC_1_DATA, 4);
	outb(PIC_2_DATA, 2);

	outb(PIC_1_DATA, ICW4_8086);
	outb(PIC_2_DATA, ICW4_8086);

	outb(PIC_1_DATA, 0xFF);
	outb(PIC_2_DATA, 0xFF);
}