#pragma once
#include <stdint.h>

void* malloc(uint32_t size);
void* malloc_aligned(uint32_t alignment, uint32_t size);