#pragma once
#include <stdint.h>

#define CDECL __attribute__((cdecl))

void CDECL outb(uint16_t port, uint8_t value);
uint8_t CDECL inb(uint16_t port);

void CDECL outl(uint32_t port, uint32_t value);
uint32_t CDECL inl(uint32_t port);

void* CDECL memset(void* dest, uint8_t val, uint32_t size);

void CDECL reboot();



