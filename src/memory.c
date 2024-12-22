#include "memory.h"
#include "stdio.h"

uintptr_t mem_alloc = 0x00500000;

void* malloc(uint32_t size)
{
	uintptr_t addr = mem_alloc;
	mem_alloc += size;
	return (void*)addr;
}

void* malloc_aligned(uint32_t alignment, uint32_t size)
{
	// First move pointer forward to aligned memory
	if (mem_alloc % alignment != 0)
		mem_alloc += (alignment - (mem_alloc % alignment));

	uintptr_t addr = mem_alloc;
	mem_alloc += size;
	return (void*)addr;
}
