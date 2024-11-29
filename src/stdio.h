#pragma once
#include <stdint.h>
#include "i686\x86.h"

#define WHITE_TXT 0x07
#define BLACK_TXT 0xF0

void clear_screen();
void setcursor(int x, int y);
void print(char* message, unsigned int line);
void print2(char* message, unsigned int line, unsigned int column, uint8_t style);
void set_char(char chr, unsigned int line, unsigned int column, uint8_t style);
void print_hexdump(void* loc, int size, unsigned int line);
void print_hexdump_volatile(volatile uint32_t* loc, unsigned int line);
void print_int(int num);
