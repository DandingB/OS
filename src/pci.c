#include "pci.h"
#include "i686\x86.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	// Configure the address for PCI configuration space
	uint32_t address = (1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC);

	// Write the address to the configuration address port (0xCF8)
	outl(PCI_CONFIG_ADDRESS, address);

	// Read the 32-bit value from the configuration data port (0xCFC)
	return inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) 
{
    uint32_t address = (1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC);

    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

uint16_t pci_get_vendor_id(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_config_read(bus, device, function, 0);
    return r0;
}

uint16_t pci_get_device_id(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_config_read(bus, device, function, 0x2);
    return (r0 & 0xFFFF);
}

uint8_t pci_get_class_id(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_config_read(bus, device, function, 0xA);
    return (r0 & 0xFF000000) >> 24;
}

uint8_t pci_get_subclass_id(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_config_read(bus, device, function, 0xA);
    return (r0 & 0x00FF0000) >> 16;
}

uint8_t pci_get_prog_if(uint16_t bus, uint16_t device, uint16_t function)
{
    uint32_t r0 = pci_config_read(bus, device, function, 0xA);
    return (r0 & 0x0000FF00) >> 8;
}

uint8_t find_msi_capability(uint32_t bus, uint32_t slot, uint32_t func) 
{
    uint8_t status = (uint8_t)(pci_config_read(bus, slot, func, 0x06) >> 16);
    if (!(status & 0x10)) return 0;  // Check if the capability list is supported

    uint8_t cap_pointer = (uint8_t)(pci_config_read(bus, slot, func, 0x34) & 0xFF);
    while (cap_pointer != 0) {
        uint32_t cap_id = pci_config_read(bus, slot, func, cap_pointer);
        if ((cap_id & 0xFF) == PCI_MSI_CAP_ID) return cap_pointer;
        cap_pointer = (uint8_t)((cap_id >> 8) & 0xFF);
    }
    return 0;
}