#include "stdio.h"
#include "pci.h"
#include "xhci.h"

#define DCBAA_BASE 	0x500000
#define CR_BASE 	0x600000
#define ER_BASE 	0x700000

#define TRB_SIZE 16  // Each TRB is 16 bytes
#define COMMAND_RING_SIZE 256  // Number of TRBs in the Command Ring

void populate_device_context(uint32_t* device_context);

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


XHCI_BASE init_xhci()
{
	void* xhci_base = 0;

    for (uint32_t device = 0; device < 32; device++)
	{
		uint16_t vendor = pci_get_vendor_id(0, device, 0);
		if (vendor == 0xffff) continue;

		uint8_t class = pci_get_class_id(0, device, 0);
		uint8_t subclass = pci_get_subclass_id(0, device, 0);
        uint8_t progif = pci_get_prog_if(0, device, 0);
		if (class == PCI_CLASS_SERIALBUS && subclass == PCI_SUBCLASS_USB && progif == PCI_PROGIF_XHCI)
		{
			xhci_base = (uint32_t*)get_xhci_base_address(0, device, 0);

			if (!pci_set_msix(0, device, 0, 0xFEE00000, 0x40))
			{
			}

			uint32_t pci_command = pci_config_read(0, device, 0, 0x05);
			pci_command &= ~PCI_COMMAND_IO;
			pci_command |= PCI_COMMAND_BUSMASTER | PCI_COMMAND_MEMORY;
			pci_config_write(0, device, 0, 4, pci_command);
		}
	}

	if (xhci_base)
	{
		volatile XHCI_REG_CAP* cap = (volatile  XHCI_REG_CAP*)xhci_base;
		volatile XHCI_REG_OP* op = (volatile XHCI_REG_OP*)(xhci_base + (cap->CAPLENGTH_HCIVERSION & 0xFF));
		volatile XHCI_REG_RT* rts = (volatile XHCI_REG_RT*)(xhci_base + cap->RTSOFF);
		volatile uint32_t* doorbell = (volatile uint32_t*)(xhci_base + cap->DBOFF);

		volatile void* command_ring = CR_BASE;

		// TODO: Claim ownership

		// TODO:
		// - Command Ring: Enable Slot Command, and get a Slot ID
		// - Alloc memory for a Device Context
		// - Set pointer to Device Context in DCBAAP (at the index corresponding to slot ID)
		// - Create a Input Context with route string (if hub: enumerate devices), speed (single usb: check PORTSC for speed)...
		// - Command Ring: Address Device Command, with pointer to Input Context and Slot ID.


		op->USBCMD |= (1 << 1);				// Reset controller
		while (op->USBCMD & (1 << 1));		// and wait
	
		op->CRCR = ((uint64_t)CR_BASE & ~0x3F) | 0x1;  // Mask lower 6 bits and set Set RCS = 1
		op->DCBAAP = DCBAA_BASE;  // Set Device Context Base Address Array Pointer

		// Alloc Event Ring
		rts->IR[0].ERSTSZ = 1;			// Table Size
		rts->IR[0].ERSTBA = ER_BASE;	// Table Base Address Pointer

		op->USBCMD |= (1 << 2);	  // Enable interrupts
		op->USBCMD |= (1 << 0);	  // Run

		// Reset all ports
		uint8_t maxPorts = (cap->HCSPARAMS1 >> 24) & 0xFF;
		for (uint8_t i = 0; i < maxPorts; i++)
		{
			op->PORTS[i].PORTSC |= (1 << 4);
		}

		

		// Build command
		EnableSlotCommand* esc = (EnableSlotCommand*)command_ring;
		esc->cmd = (1 << 0) | (9 << 10);
		
		// Ring doorbell
		doorbell[0] = (0 << 0);

		uint32_t test = doorbell[0];
		print_hexdump(&test, 4, 3);
		
		for (int i = 0; i < 1024; i++)
		{
			uint64_t test = rts->IR[i].IMAN;
			print_hexdump(&test, 8, 4+i);
		}


		// uint64_t* devices = op->DCBAAP & ~0x3F;
		// void* deviceContext1 = devices[1];
		// print_hexdump(deviceContext1, 16, 5);



		// uint64_t* dcbaa = (uint64_t*)DCBAA_BASE;
		// memset(dcbaa, 0, 256 * sizeof(uint64_t));
		
		// uint32_t* dc1 = (uint32_t*)0x501000;
		// populate_device_context(dc1);
		// dcbaa[0] = dc1;

		// print_hexdump(dc1, 64, 18);
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

		

		// uint32_t USBCMD = op->USBCMD;
		// uint32_t USBSTS = op->USBSTS;
		// uint32_t PAGESIZE = op->PAGESIZE;
		// uint32_t DNCTRL = op->DNCTRL;
		// uint32_t CRCR = op->CRCR;
		// uint64_t DCBAAP = op->DCBAAP;
		// print_hexdump(&USBCMD, 4, 9);
		// print_hexdump(&USBSTS, 4, 10);
		// print_hexdump(&PAGESIZE, 4, 11);
		// print_hexdump(&DNCTRL, 4, 12);
		// print_hexdump(&CRCR, 8, 13);
		// print_hexdump(&DCBAAP, 8, 14);


		// while(1)
		// {
		// 	int lineCount = 5;

		// 	for (uint8_t i = 0; i < maxPorts; i++)
		// 	{
		// 		if (op->PORTS[i].PORTSC & (1 << 0))
		// 		{
		// 			uint32_t PORTSC = op->PORTS[i].PORTSC;
		// 			print_hexdump(&PORTSC, 4, lineCount++);
		// 		}
		// 	}

		// 	// uint64_t* devices = op->DCBAAP & ~0x3F;
		// 	// for (int i = 0; i < 256; i++)
		// 	// {
		// 	// 	if (devices[i] != 0)
		// 	// 		print_hexdump(&devices[i], 8, lineCount++);
		// 	// }


		// 	for (uint8_t i = 0; i < 25; i++)
		// 	{
		// 		print("                                               ", lineCount++);
		// 	}
		// }

		return (XHCI_BASE)xhci_base;
	}

	return 0;
}

void xhci_reset(XHCI_BASE xhci_base)
{
	volatile XHCI_REG_CAP* cap = (volatile  XHCI_REG_CAP*)xhci_base;
	volatile XHCI_REG_OP* op = (volatile XHCI_REG_OP*)(xhci_base + (cap->CAPLENGTH_HCIVERSION & 0xFF));

	op->USBCMD |= (1 << 1);	 // Reset controller
	while ((op->USBCMD & (1 << 1)) && (op->USBSTS & (1 << 11)));   // and wait

	op->DCBAAP = DCBAA_BASE;  			// Set Device Context Base Address Array Pointer
	op->USBCMD |= (1 << 2);	  			// Enable interrupts
	op->USBCMD |= (1 << 0);	  			// Run


	uint8_t maxPorts = (cap->HCSPARAMS1 >> 24) & 0xFF;
	for (uint8_t i = 0; i < maxPorts; i++)
	{
		op->PORTS[i].PORTSC |= (1 << 4);
	}
	xhci_ports_list(xhci_base);
}

void xhci_ports_list(XHCI_BASE xhci_base)
{
	volatile XHCI_REG_CAP* cap = (volatile  XHCI_REG_CAP*)xhci_base;
	volatile XHCI_REG_OP* op = (volatile XHCI_REG_OP*)(xhci_base + (cap->CAPLENGTH_HCIVERSION & 0xFF));

	uint8_t maxPorts = (cap->HCSPARAMS1 >> 24) & 0xFF;
	for (uint8_t i = 0; i < maxPorts; i++)
	{
		uint32_t PORTSC = op->PORTS[i].PORTSC;
		print_hexdump(&PORTSC, 4, 5+i);
	}
}