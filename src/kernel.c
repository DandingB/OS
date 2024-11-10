#include <stdint.h>
#include "stdio.h"
#include "ahci.h"
#include "pci.h"
#include "paging.h"
#include "i686\x86.h"
#include "i686\idt.h"
#include "i686\pic.h"

#define PCI_COMMAND_INTERRUPT_DISABLE (1 << 10)
#define PCI_INTERRUPT_LINE 0x3C

int strcmp(char* str1, char* str2);
void process_cmd();
uint32_t stoi(char* str);

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
	init_paging();

	//hba = init_ahci();	

	clear_screen();
	print2(" BandidOS                                                         Esc to reboot ", 0, 0, BLACK_TXT);

	setcursor(0, 3);

	//uint32_t command = pci_read_config_dword(0, 4, 0, 0x4);
	//command |= PCI_COMMAND_INTERRUPT_ENABLE;
	//pci_write_config_dword(0, 4, 0, 0x4, command);

	//uint32_t command2 = pci_read_config_dword(0, 4, 0, PCI_INTERRUPT_LINE);
	//print_hexdump(&command2, 4, 13);

	//__asm("int $0x2");

	int i = 0;
	while (1)
	{
		//print_int(i);
		//i++;

		__asm("hlt");
	}
}

int strcmp(char* str1, char* str2)
{
	while (*str1 != '\0')
	{
		if (*str1 != *str2)
			return 1;

		str1++;
		str2++;
	}

	if (*str1 != *str2)
		return 1;

	return 0;
}

uint32_t stoi(char* str)
{
	uint32_t value = 0;
	while (*str != '\0')
	{
		uint32_t digit = *str-0x30;
		value *= 10;	
		value += digit;
		str++;
	}
	return value;
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

	if (strcmp(cli_temp, "read") == 0)
	{
		if (numArgs == 1)
		{
			char data[512] __attribute__((aligned(16))); // Memory to be read to shall be aligned

			HBA_PORT* hba_port0 = (HBA_PORT*)&hba->ports[0];
			//hba_port0->ie = 0xFFFFFFFF;		// Enable all interrupt types
			hba_port0->serr = 0xFFFFFFFF;	// Clear the port error register to 0xFFFFFFFF (otherwise again it will be stuck in BSY forever)

			//write(hba_port0, data, 21, 1);
			uint32_t sector = stoi(args[0]);
			read(hba_port0, data, (uint64_t)sector, 1);
			print("                                            ", 23);
			print(data, 23);
		}

		return;
	}
}



