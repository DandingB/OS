#include <stdint.h>
#include "stdlib.h"
#include "stdio.h"
#include "ahci.h"
#include "xhci.h"
#include "pci.h"
#include "i686\paging.h"
#include "i686\x86.h"
#include "i686\idt.h"
#include "i686\pic.h"
#include "i686\apic.h"

#define PCI_COMMAND_INTERRUPT_DISABLE (1 << 10)
#define PCI_INTERRUPT_LINE 0x3C

void process_cmd();

int cursor = 0;
int line = 3;
char cli_temp[75];
uint32_t mem_alloc = 0x00600000;
HBA_MEM* hba = 0;

void* malloc_dumb(uint32_t size)
{
	uint32_t addr = mem_alloc;
	mem_alloc += size;
	return (void*)addr;
}

const char ASCIITable[] = {
	 0 ,  0 , '1', '2',
	'3', '4', '5', '6',
	'7', '8', '9', '0',
	'-', '=',  0 ,  0 ,
	'q', 'w', 'e', 'r',
	't', 'y', 'u', 'i',
	'o', 'p', '[', ']',
	 0 ,  0 , 'a', 's',
	'd', 'f', 'g', 'h',
	'j', 'k', 'l', ';',
	'\'', '`',  0 , '\\',
	'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',',
	'.', '/',  0 , '*',
	 0 , ' '
};

char translate(uint8_t scancode)
{
	if (scancode > 58) return 0;
	return ASCIITable[scancode];
}

void put_char(char chr)
{
	if (cursor < 0 || cursor > 75)
		return;

	cli_temp[cursor] = chr;

	set_char(chr, line, cursor++, WHITE_TXT);
	setcursor(cursor, line);
}

void erase_char()
{
	if (cursor <= 0)
		return;

	set_char(' ', line, --cursor, WHITE_TXT);
	setcursor(cursor, line);

	cli_temp[cursor] = '\0';
}

void key_press(uint8_t key)
{
	if (key == 0x01) // Esc
	{
		print("Restarting!", 0);
		reboot();
	}
	else if (key == 0x48) // Up
	{
	}
	else if (key == 0x50) // Down
	{
	}
	else if (key == 0x0E) // Backspace
	{
		erase_char();
	}
	else if (key == 0x1C) // Enter
	{
		process_cmd();
		cursor = 0;
		line++;
		setcursor(cursor, line);
		memset(cli_temp, 0x0, 75);
	}
	else if (key < 0x3A)
	{
		//released = 0;

		char ascii = translate(key);
		if (ascii != 0)
			put_char(ascii);
	}
	else if (key > 0x81 && key < 0xD8)
	{
	}
}


void CDECL kmain(uint16_t bootDrive)
{
	init_pic();
	init_idt();
	//init_paging();

	init_apic();
	
	hba = init_ahci();

	clear_screen();
	print2(" BandidOS                                                         Esc to reboot ", 0, 0, BLACK_TXT);
	setcursor(0, 3);

	init_xhci();

	//__asm("int $0x2");

	int i = 0;
	while (1)
	{
		//print_int(i);
		//i++;

		__asm("hlt");
	}
}

void process_cmd()
{
	int	numArgs = 0;
	char* args[16];
	char* tmp = cli_temp;

	while (*tmp != '\0')
	{
		if (*tmp == ' ')
		{	
			*tmp = '\0'; // Replace spaces with null
			args[numArgs] = tmp + 1;
			numArgs++;
		}
		tmp++;
	}

	if (strcmp(cli_temp, "hello") == 0)
	{
		print2(" Hello!                 ", 0, 0, BLACK_TXT);
		for (int i = 0; i < numArgs; i++)
		{
			if (strcmp(args[i], "again") == 0)
			{
				print2(" my friend", 0, 10, BLACK_TXT);
				break;
			}
		}
		return;
	}

	if (strcmp(cli_temp, "clear") == 0)
	{
		if (numArgs == 0)
		{
			clear_screen();
			print2(" BandidOS                                                         Esc to reboot ", 0, 0, BLACK_TXT);
			line = 3;
		}
	}

	if (strcmp(cli_temp, "read") == 0)
	{
		if (numArgs == 1)
		{
			char data[512] __attribute__((aligned(16))); // Memory to be read to shall be aligned

			HBA_PORT* hba_port0 = (HBA_PORT*)&hba->ports[0];
			//hba_port0->ie = 0xFFFFFFFF;		// Enable all interrupt types
			hba_port0->serr = 0xFFFFFFFF;	// Clear the port error register to 0xFFFFFFFF (otherwise again it will be stuck in BSY forever)

			//write(hba_port0, data, 21, 1);
			uint32_t sector = atoi(args[0]);
			read(hba_port0, data, (uint64_t)sector, 1);
			print("                                            ", 23);
			print(data, 23);
		}

		return;
	}

	if (strcmp(cli_temp, "pci") == 0)
	{
		if (numArgs == 1)
		{
			if (strcmp(args[0], "list") == 0)
			{

				int tempLine = line + 1;
				for (uint32_t device = 0; device < 32; device++)
				{
					uint16_t vendor = pci_get_vendor_id(0, device, 0);
					if (vendor == 0xffff) continue;

					uint8_t class = pci_get_class_id(0, device, 0);
					uint8_t subclass = pci_get_subclass_id(0, device, 0);

					print("-----------------", tempLine); tempLine++;
					print_hexdump(&class, 1, tempLine); tempLine++;
					print_hexdump(&subclass, 1, tempLine); tempLine++;

					//tempLine++;
				}
			}
		}

		if (numArgs == 2)
		{
			if (strcmp(args[0], "device") == 0)
			{
				int tempLine = line + 1;

				uint32_t device = atoi(args[1]);

				uint8_t class = pci_get_class_id(0, device, 0);
				uint8_t subclass = pci_get_subclass_id(0, device, 0);
				uint8_t progif = pci_get_prog_if(0, device, 0);

				print_hexdump(&class, 1, tempLine); tempLine++;
				print_hexdump(&subclass, 1, tempLine); tempLine++;
				print_hexdump(&progif, 1, tempLine); tempLine++;
			}
		}
	}
}



