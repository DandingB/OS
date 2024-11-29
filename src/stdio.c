#include "stdio.h"

void clear_screen()
{
	char* vidmem = (char*)0xB8000;
	unsigned int i = 0;
	while (i < (80 * 25 * 2))
	{
		vidmem[i] = ' ';
		i++;
		vidmem[i] = WHITE_TXT;
		i++;
	}
}

void setcursor(int x, int y)
{
	uint16_t pos = y * 80 + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void print(char* message, unsigned int line)
{
	char* vidmem = (char*)0xB8000;
	unsigned int i = 0;

	i = (line * 80 * 2);

	while (*message != 0)
	{
		if (*message == '\n') // check for a new line
		{
			line++;
			i = (line * 80 * 2);
			*message++;
		}
		else {
			vidmem[i] = *message;
			*message++;
			i++;
			vidmem[i] = WHITE_TXT;
			i++;
		};
	};
}

void print2(char* message, unsigned int line, unsigned int column, uint8_t style)
{
	char* vidmem = (char*)0xB8000;
	unsigned int i = 0;

	i = (line * 80 * 2) + (column * 2);

	while (*message != 0)
	{
		if (*message == '\n') // check for a new line
		{
			line++;
			i = (line * 80 * 2);
			*message++;
		}
		else {
			vidmem[i] = *message;
			*message++;
			i++;
			vidmem[i] = style;
			i++;
		};
	};
}

void set_char(char chr, unsigned int line, unsigned int column, uint8_t style)
{
	char* vidmem = (char*)0xB8000;
	unsigned int i = (line * 80 * 2) + (column * 2);

	vidmem[i++] = chr;
	vidmem[i] = style;
}



void print_int(int num)
{
	// Define video memory address
	char* vidmem = (char*)0xB8000;

	// Convert integer to string
	char str[12]; // Max length of integer is 11 digits
	int i = 0;
	if (num == 0) {
		str[i++] = '0';
	}
	else {
		if (num < 0) {
			vidmem[0] = '-'; // If number is negative, display '-' sign
			num = -num;
		}
		while (num != 0) {
			str[i++] = num % 10 + '0'; // Convert each digit to character
			num /= 10;
		}
	}

	// Reverse the string
	int j = 0;
	if (str[0] == '-') j = 1; // Skip the '-' sign if present
	i--; // Decrement i to point to last character
	while (j < i) {
		char temp = str[j];
		str[j] = str[i];
		str[i] = temp;
		j++;
		i--;
	}

	// Write characters to video memory
	i = 0;
	while (str[i] != '\0') {
		vidmem[i * 2] = str[i]; // Character
		vidmem[i * 2 + 1] = 0x07; // Attribute (white text on black background)
		i++;
	}
}



void print_hexdump(void* loc, int size, unsigned int line) {

	char* vidmem = (char*)(0xB8000 + (line * 80 * 2));
	const char hex_chars[] = "0123456789ABCDEF";

	// Write "0x" to the video memory
	*vidmem++ = '0';
	*vidmem++ = WHITE_TXT;
	*vidmem++ = 'x';

	while (size > 0)
	{
		//uint32_t test = *(uint32_t*)loc
		uint8_t byte = *(uint8_t*)loc;

		vidmem++;
		*vidmem++ = hex_chars[(byte >> 4) & 0xF];
		vidmem++;
		*vidmem++ = hex_chars[byte & 0xF];

		loc++;
		size--;
	}
}

void print_hexdump_volatile(volatile uint32_t* loc, unsigned int line) 
{
    char* vidmem = (char*)(0xB8000 + (line * 80 * 2));
    const char hex_chars[] = "0123456789ABCDEF";

    *vidmem++ = '0';
    *vidmem++ = WHITE_TXT;
    *vidmem++ = 'x';

    uint32_t value = *loc;
    for (int i = 0; i < 4; i++) 
	{
        uint8_t byte = (value >> (i * 8)) & 0xFF;

		*vidmem++ = WHITE_TXT;
        *vidmem++ = hex_chars[(byte >> 4) & 0xF];
        *vidmem++ = WHITE_TXT;
        *vidmem++ = hex_chars[byte & 0xF];
    }
}