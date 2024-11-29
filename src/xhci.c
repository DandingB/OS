#include "stdio.h"
#include "pci.h"
#include "xhci.h"

#define XHCI_CAPLENGTH 0x00 // Capability Register Length
#define XHCI_USBCMD    0x00 // USB Command Register
#define XHCI_USBSTS    0x04 // USB Status Register
#define XHCI_CONFIG    0x38 // Configure Register

// Function to get the base address of the xHCI controller
uint32_t get_xhci_base_address(uint8_t bus, uint8_t device, uint8_t function) 
{
    uint32_t bar = pci_config_read(bus, device, function, 0x10); // Read the first BAR

    // Check if it is a memory-mapped BAR
    if (bar & 0x01) {
        // I/O port BAR (not common for xHCI)
        return bar & 0xFFFFFFFC;
    } else {
        // Memory-mapped BAR
        return bar & 0xFFFFFFF0;
    }
}

uint32_t* init_xhci()
{
	uint32_t* xhci_base = 0;

    for (uint32_t device = 0; device < 32; device++)
	{
		uint16_t vendor = pci_get_vendor_id(0, device, 0);
		if (vendor == 0xffff) continue;

		uint8_t class = pci_get_class_id(0, device, 0);
		uint8_t subclass = pci_get_subclass_id(0, device, 0);
        uint8_t progif = pci_get_prog_if(0, device, 0);
		if (class == PCI_CLASS_SERIALBUS && subclass == PCI_SUBCLASS_USB && progif == PCI_PROGIF_XHCI)
		{
			//xhci_base = (uint32_t*)(pci_config_read(0, device, 0, 0x10) & 0xFFFFFF00);
			xhci_base = get_xhci_base_address(0, device, 0);

            // Enable bus master in PCI config register
			uint32_t pci_command = pci_config_read(0, device, 0, 0x04);

			pci_command &= ~PCI_COMMAND_IO;
			pci_command |= PCI_COMMAND_BUSMASTER | PCI_COMMAND_MEMORY;

			pci_config_write(0, device, 0, 4, pci_command);
		}
	}

	if (xhci_base)
	{
		volatile XHCI_CAP* cap = (volatile  XHCI_CAP*)xhci_base;
		volatile XHCI_OP* op = (volatile XHCI_OP*)(xhci_base + ((cap->CAPLENGTH_HCIVERSION & 0xFF) / 4));

		// uint32_t* addr = (uint32_t*)(0xFEBF0000);
		// uint32_t test = *(addr);
		// print_hexdump(&test, 4, 7);

		//*addr = 0xFFFFFF80;
		
		// test |= (1 << 2) | (1 << 3);
		// *addr = test;

		// uint32_t test2 = *(addr);
		// print_hexdump(&test2, 4, 10);

		// uint16_t ecp = (test >> 16) & 0xFF;
		// print_hexdump(&ecp, 2, 10);

		// uint32_t CAPLENGTH_HCIVERSION = cap->CAPLENGTH_HCIVERSION;
		// uint32_t HCSPARAMS1 = cap->HCSPARAMS1;
		// uint32_t HCSPARAMS2 = cap->HCSPARAMS2;
		// uint32_t HCSPARAMS3 = cap->HCSPARAMS3;
		// uint32_t HCCPARAMS1 = cap->HCCPARAMS1;
		// uint32_t DBOFF = cap->DBOFF;
		// uint32_t RTSOFF = cap->RTSOFF;
		// print_hexdump(&CAPLENGTH_HCIVERSION, 4, 10);
		// print_hexdump(&HCSPARAMS1, 4, 11);
		// print_hexdump(&HCSPARAMS2, 4, 12);
		// print_hexdump(&HCSPARAMS3, 4, 13);
		// print_hexdump(&HCCPARAMS1, 4, 14);
		// print_hexdump(&DBOFF, 4, 15);
		// print_hexdump(&RTSOFF, 4, 16);

		uint32_t USBCMD = op->USBCMD;
		uint32_t USBSTS = op->USBSTS;
		uint32_t PAGESIZE = op->PAGESIZE;
		uint32_t DNCTRL = op->DNCTRL;
		uint32_t CRCR = op->CRCR;
		uint64_t DCBAAP = op->DCBAAP;
		print_hexdump(&USBCMD, 4, 9);
		print_hexdump(&USBSTS, 4, 10);
		print_hexdump(&PAGESIZE, 4, 11);
		print_hexdump(&DNCTRL, 4, 12);
		print_hexdump(&CRCR, 8, 13);
		print_hexdump(&DCBAAP, 8, 14);


	}

	return (uint32_t*)xhci_base;
}

void reset_xhci_controller(uint32_t* mmio_base) 
{
    mmio_base[XHCI_USBCMD / 4] &= ~1; // Stop the controller
    while (mmio_base[XHCI_USBSTS / 4] & (1 << 0)); // Wait until halted

    mmio_base[XHCI_USBCMD / 4] |= (1 << 1); // Reset the controller
    while (mmio_base[XHCI_USBCMD / 4] & (1 << 1)); // Wait for reset to complete
}