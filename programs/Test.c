#include <stdint.h>
int main()
{
	int test = 0;
	test += 69;

	uint32_t* pTest = (uint32_t*)0x00600060;
	*pTest = test;

	//__asm volatile ("int $0x2");

	__asm volatile ("syscall");

	while (1);
}