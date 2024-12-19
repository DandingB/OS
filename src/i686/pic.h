#pragma once
#include <stdint.h>

#define PIC_1_CTRL 0x20
#define PIC_1_DATA 0x21
#define PIC_2_CTRL 0xA0
#define PIC_2_DATA 0xA1

#define ICW1_ICW4		0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE		0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL		0x08		/* Level triggered (edge) mode */
#define ICW1_INIT		0x10		/* Initialization - required! */

#define ICW4_8086		0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO		0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM		0x10		/* Special fully nested (not) */


#define PIC_CMD_END_OF_INTERRUPT	0x20
#define PIC_CMD_READ_IRR			0x0A
#define PIC_CMD_READ_ISR			0x0B


void init_pic();
void disable_pic();