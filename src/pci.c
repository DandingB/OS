#include "pci.h"
#include "i686\x86.h"

uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	// Configure the address for PCI configuration space
	uint32_t address = (1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xfc);

	// Write the address to the configuration address port (0xCF8)
	outl(0xCF8, address);

	// Read the 32-bit value from the configuration data port (0xCFC)
	return inl(0xCFC);
}

uint16_t getVendorID(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_read_config_dword(bus, device, function, 0);
    return r0;
}

uint16_t getDeviceID(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_read_config_dword(bus, device, function, 0x2);
    return (r0 & 0xFFFF);
}

uint16_t getClassId(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_read_config_dword(bus, device, function, 0xA);
    return (r0 & 0xFF000000) >> 24;
}

uint16_t getSubClassId(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_read_config_dword(bus, device, function, 0xA);
    return (r0 & 0x00FF0000) >> 16;
}