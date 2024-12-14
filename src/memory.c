#include "memory.h"

uint32_t mem_alloc = 0x00600000;

void* malloc(uint32_t size)
{
	uint32_t addr = mem_alloc;
	mem_alloc += size;
	return (void*)addr;
}

void* malloc_aligned(uint32_t alignment, uint32_t size)
{
	// First move pointer forward to aligned memory
	if (mem_alloc % alignment != 0)
		mem_alloc += (alignment - (mem_alloc % alignment));

	uint32_t addr = mem_alloc;
	mem_alloc += size;
	return (void*)addr;
}
