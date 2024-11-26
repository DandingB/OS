#include <stdint.h>

uint32_t atoi(char* str)
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