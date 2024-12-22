#pragma once
#include <stdint.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

void outl(uint32_t port, uint32_t value);
uint32_t inl(uint32_t port);

void* memset(void* dest, uint8_t val, uint32_t size);

void reboot();



