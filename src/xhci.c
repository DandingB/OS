#include "stdio.h"
#include "pci.h"
#include "xhci.h"
#include "i686/x86.h"

#define DCBAA_BASE 	0x500000
#define SPBAA_BASE 	0x510000
#define SPBA_BASE 	0x520000
#define CR_BASE 	0x600000
#define ERST_BASE 	0x700000
#define ER_BASE 	0x710000

#define TRB_SIZE 16  // Each TRB is 16 bytes
#define COMMAND_RING_SIZE 256  // Number of TRBs in the Command Ring

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

uint64_t calculate_page_size(uint32_t field) 
{
    for (int n = 0; n < 32; ++n) 
	{
        if (field & (1ULL << n)) 
            return 1ULL << (n + 12); // Page size: 2^(n+12)
    }
    return 0;
}



XHCI_BASE init_xhci()
{
	uint32_t xhci_base = 0;

    for (uint32_t device = 0; device < 32; device++)
	{
		uint16_t vendor = pci_get_vendor_id(0, device, 0);
		if (vendor == 0xffff) continue;

		uint8_t class = pci_get_class_id(0, device, 0);
		uint8_t subclass = pci_get_subclass_id(0, device, 0);
        uint8_t progif = pci_get_prog_if(0, device, 0);
		if (class == PCI_CLASS_SERIALBUS && subclass == PCI_SUBCLASS_USB && progif == PCI_PROGIF_XHCI)
		{
			xhci_base = get_xhci_base_address(0, device, 0);

			if (!pci_set_msix(0, device, 0, 0xFEE00000, 0x41))
			{
			}

			// Enable the bus master
			uint32_t pci_command = pci_config_read(0, device, 0, 0x05);
			pci_command &= ~PCI_COMMAND_IO;
			pci_command |= PCI_COMMAND_BUSMASTER | PCI_COMMAND_MEMORY;
			pci_config_write(0, device, 0, 4, pci_command);
		}
	}

	if (xhci_base != 0)
	{
		XHCI_REG_CAP* cap = (XHCI_REG_CAP*)xhci_base;
		XHCI_REG_OP* op = (XHCI_REG_OP*)(xhci_base + (cap->CAPLENGTH_HCIVERSION & 0xFF));
		XHCI_REG_RT* rts = (XHCI_REG_RT*)(xhci_base + cap->RTSOFF);
		uint32_t* doorbell = (uint32_t*)(xhci_base + cap->DBOFF);

		XHCI_TRB* command_ring = (XHCI_TRB*)(CR_BASE);	// Command Ring
		XHCI_TRB* event_ring = (XHCI_TRB*)(ER_BASE);	// Event Ring
		uint64_t* dcbaa = (uint64_t*)(DCBAA_BASE);		// Device Context Base Address Array
		uint64_t* spbaa = (uint64_t*)(SPBAA_BASE);		// Scratchpad Base Address Array


		// Claim ownership
		uint32_t xecp = xhci_base + (((cap->HCCPARAMS1 >> 16) & 0xFFFF) << 2);
		while (1)
		{
			uint32_t* cap = (uint32_t*)xecp;
			uint16_t capId = (*cap & 0xFF);
			uint16_t offset = ((*cap >> 8) & 0xFF) << 2;

			// We found a "USB Legacy Support" Capability
			if (capId == 0x01)				
			{
				*cap |= (1 << 24);			// Set OS Ownership bit
				while ((*cap) & (1 << 16));	// Wait for BIOS Ownership bit to clear
				break;
			}

			if (offset == 0)
				break;

			xecp += offset;
		}

		// TODO:
		// - Command Ring: Enable Slot Command, and get a Slot ID
		// - Alloc memory for a Device Context
		// - Set pointer to Device Context in DCBAAP (at the index corresponding to slot ID)
		// - Create a Input Context with route string (if hub: enumerate devices), speed (single usb: check PORTSC for speed)...
		// - Command Ring: Address Device Command, with pointer to Input Context and Slot ID.

		// Reset the Host Controller & Wait...
		op->USBCMD |= USBCMD_HCRST;	
		while (op->USBCMD & USBCMD_HCRST);

		// Setup Event Ring Segment Table
		XHCI_ERST* segment_table = (XHCI_ERST*)(ERST_BASE);
		memset(segment_table, 0, sizeof(XHCI_ERST));
		segment_table[0].address = ER_BASE;
		segment_table[0].size = 16;

		// Setup Interrupter Register Set 0
		rts->IR[0].IMAN |= IMAN_IE; 
		rts->IR[0].IMOD = (4000 & 0xFFFF) | ((10 & 0xFFFF) << 16);
		rts->IR[0].ERSTSZ = 1;							// Table Size
		rts->IR[0].ERSTBA = (uint32_t)segment_table;	// Table Base Address Pointer
		rts->IR[0].ERDP = ER_BASE & ~ERDP_EHB;			// Dequeue pointer

		op->DCBAAP = DCBAA_BASE;  	  			// Set Device Context Base Address Array Pointer
		op->CRCR = (CR_BASE & ~0x3F) | 1;  		// The Command Ring Base & Set Cycle Bit = 1

		uint8_t maxSlots = cap->HCSPARAMS1 & 0xFF;
		memset(dcbaa, 0, sizeof(uint64_t) * maxSlots);
		memset((void*)CR_BASE, 0, sizeof(XHCI_TRB) * 16);
		memset((void*)ER_BASE, 0, sizeof(XHCI_TRB) * 16);

		// Set Max Device Slots Enabled, or else we can't call Enable Slot Command
		op->CONFIG |= (maxSlots & 0xFF);

		// Alloc scratchpad buffers
		uint64_t pageSize = calculate_page_size(op->PAGESIZE);
		uint16_t maxScratchpad = ((cap->HCSPARAMS2 >> 16) & 0x3E0) | (cap->HCSPARAMS2 >> 27);
		for (int i = 0; i < maxScratchpad; i++)
		{
			spbaa[i] = (uint64_t)(SPBA_BASE + (pageSize * i));
			memset(spbaa[i], 0, pageSize);
		}
		dcbaa[0] = SPBAA_BASE;		// Index 0 in DCBAA is reserved for scratchpad array (if maxScratchpad > 0)

		op->USBCMD |= USBCMD_INTE;	// Enable interrupts
		op->USBCMD |= USBCMD_RS;	// Run
		

		// Reset all ports
		uint8_t maxPorts = (cap->HCSPARAMS1 >> 24) & 0xFF;
		for (uint8_t i = 0; i < maxPorts; i++)
		{
			op->PORTS[i].PORTSC |= PORTSC_PR;
		}


		// Build command
		command_ring[0].address = 0;
		command_ring[0].status = 0;
		command_ring[0].flags = (1 << 0) | (9 << 10);

		// Ring doorbell
		doorbell[0] = 0;

		while (!(event_ring->flags & (1 << 0)));
		
		uint64_t CRCR = op->CRCR;
		uint32_t IMAN = rts->IR[0].IMAN;
		print_hexdump(&CRCR, 8, 7);
		print_hexdump(&IMAN, 4, 8);

		print_hexdump(&event_ring[0].address, 8, 10);
		print_hexdump(&event_ring[0].status, 4, 11);
		print_hexdump(&event_ring[0].flags, 4, 12);

		print_hexdump(&event_ring[1].address, 8, 14);
		print_hexdump(&event_ring[1].status, 4, 15);
		print_hexdump(&event_ring[1].flags, 4, 16);

		print_hexdump(&event_ring[2].address, 8, 18);
		print_hexdump(&event_ring[2].status, 4, 19);
		print_hexdump(&event_ring[2].flags, 4, 20);

		// When receiving an interrupt:
		// 1) Clear the Status register bit
		// 2) Clear the Interrupt Management bits (Write the *_MAN_IE and *_MAN_IP bits)
		// 3) Process the TD(s)
		// 4) Update the Dequeue Pointer to point to the last TD processed, clearing the EHB bit at the same time


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