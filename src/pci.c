#include "pci.h"
#include "stdio.h"
#include "i686/x86.h"

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

uint8_t find_capability(uint32_t bus, uint32_t device, uint32_t func, uint8_t cap) 
{
    uint8_t status = (uint8_t)(pci_config_read(bus, device, func, 0x06) >> 16);
    if (!(status & 0x10)) return 0;  // Check if the capability list is supported

    uint8_t cap_pointer = (uint8_t)(pci_config_read(bus, device, func, 0x34) & 0xFF);
    while (cap_pointer != 0) 
    {
        uint8_t cap_id = pci_config_read(bus, device, func, cap_pointer);
        if ((cap_id & 0xFF) == cap) return cap_pointer;
        cap_pointer = (uint8_t)((cap_id >> 8) & 0xFF);
    }
    return 0;
}

int pci_set_msi(uint32_t bus, uint32_t device, uint32_t func, uint32_t apic_base, uint32_t vector)
{
    uint8_t msi_cap_offset = find_capability(bus, device, func, PCI_CAP_MSI);
    if (msi_cap_offset == 0)
        return 0;

    // Write the MSI address and data
    pci_config_write(bus, device, 0, msi_cap_offset + PCI_MSI_ADDR_OFFSET, apic_base);
    pci_config_write(bus, device, 0, msi_cap_offset + PCI_MSI_DATA_OFFSET, vector);

     // Enable MSI by setting the MSI Enable bit in the control register
    uint32_t msi_control = pci_config_read(bus, device, func, msi_cap_offset);
    msi_control |= (1 << 16);  // Set MSI Enable bit
    pci_config_write(bus, device, func, msi_cap_offset, msi_control);

    return 1;
}

int pci_set_msix(uint32_t bus, uint32_t device, uint32_t func, uint32_t apic_base, uint32_t vector)
{
    uint8_t msix_cap_offset = find_capability(bus, device, func, PCI_CAP_MSIX);
    if (msix_cap_offset == 0)
        return 0;

    uint32_t msix_control   = pci_config_read(bus, device, func, msix_cap_offset);
    uint32_t msix_table     = pci_config_read(bus, device, func, msix_cap_offset + 0x04);
    uint8_t  bir            = msix_table & 0b111;     // Get the BAR index of the BAR that points to the message table
    uint32_t table_offset   = msix_table & ~0b111;    // Get the offset into the pointer where the message table lies
    uint16_t table_size     = ((msix_control >> 16) & 0x3FF) + 1;

    uint32_t bar            = pci_config_read(bus, device, func, PCI_BAR(bir));
    uint64_t table_base     = (bar & ~0xF) + table_offset;

    uint32_t* msix_table_base = (uint32_t*)((uint32_t)table_base); // Many static casts or the compiler may Yap

    for (uint32_t i = 0; i < table_size; i++)
    {
        volatile uint32_t* entry = (uint32_t*)(msix_table_base + i * 4);

        entry[0] = apic_base;
        entry[1] = 0;
        entry[2] = vector;
        entry[3] = 0;
    }

    msix_control |= (1 << 31);  // Set Enable bit
    pci_config_write(bus, device, func, msix_cap_offset, msix_control);

    return 1;
}
