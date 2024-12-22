#include <stdint.h>
int main()
{
	int test = 0;
	test += 69;

	uint32_t* pTest = (uint32_t*)0x00A00030;
	*pTest = test;
}