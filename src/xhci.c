#include "stdio.h"
#include "pci.h"
#include "xhci.h"
#include "memory.h"
#include "i686/x86.h"

XHCI_REG_CAP* cap;		// Capability registers
XHCI_REG_OP* op;		// Operational registers
XHCI_REG_RT* rts;		// Runtime registers
uint32_t* doorbell;		// Doorbell registers

XHCI_TRB* command_ring;	// Command Ring
XHCI_TRB* event_ring;	// Event Ring
uint64_t* dcbaa;		// Device Context Base Address Array
uint64_t* spbaa;		// Scratchpad Base Address Array

uint8_t iCmdEnqueue = 0;
uint8_t bCmdCycle = 1;


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



void init_xhci()
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
		cap = (XHCI_REG_CAP*)xhci_base;
		op = (XHCI_REG_OP*)(xhci_base + (cap->CAPLENGTH_HCIVERSION & 0xFF));
		rts = (XHCI_REG_RT*)(xhci_base + cap->RTSOFF);
		doorbell = (uint32_t*)(xhci_base + cap->DBOFF);

		uint8_t maxSlots = cap->HCSPARAMS1 & 0xFF; // Max 256
		uint64_t pageSize = calculate_page_size(op->PAGESIZE);
		uint16_t maxScratchpad = ((cap->HCSPARAMS2 >> 16) & 0x3E0) | (cap->HCSPARAMS2 >> 27);
		uint8_t maxERST = (cap->HCSPARAMS2 >> 4) & 0b1111;
		uint8_t contextSize = (cap->HCCPARAMS1 >> 2) & 1;	// 0 = 32 bit contexts, 1 = 64 bit contexts

		print_hexdump(&maxSlots, 1, 3);

		command_ring = (XHCI_TRB*)malloc_aligned(64, sizeof(XHCI_TRB) * 16);
		event_ring = (XHCI_TRB*)malloc_aligned(64, sizeof(XHCI_TRB) * 16 * 2);
		dcbaa = (uint64_t*)malloc_aligned(64, sizeof(uint64_t) * (maxSlots+1));
		spbaa = (uint64_t*)malloc_aligned(64, sizeof(uint64_t) * maxScratchpad);

		memset((void*)dcbaa, 0, sizeof(uint64_t) * (maxSlots+1));
		memset((void*)command_ring, 0, sizeof(XHCI_TRB) * 4);
		memset((void*)event_ring, 0, sizeof(XHCI_TRB) * 16);


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

		// Reset the Host Controller & Wait...
		op->USBCMD |= USBCMD_HCRST;	
		while (op->USBCMD & USBCMD_HCRST);

		// Setup Event Ring Segment Table
		XHCI_ERST* segment_table = (XHCI_ERST*)malloc_aligned(64, sizeof(XHCI_ERST) * 2);
		memset(segment_table, 0, sizeof(XHCI_ERST) * 2);
		segment_table[0].address = (uintptr_t)&event_ring[0];
		segment_table[0].size = 16;
		// segment_table[1].address = (uintptr_t)&event_ring[16];
		// segment_table[1].size = 16;

		// Setup Interrupter Register Set 0
		rts->IR[0].IMAN |= IMAN_IE; 
		rts->IR[0].IMOD = (4000 & 0xFFFF) | ((10 & 0xFFFF) << 16);
		rts->IR[0].ERSTSZ = 1;									// Table Size
		rts->IR[0].ERSTBA = (uintptr_t)segment_table;			// Table Base Address Pointer
		rts->IR[0].ERDP = (uintptr_t)event_ring & ~ERDP_EHB;	// Dequeue pointer

		op->DCBAAP = (uintptr_t)dcbaa;  	  			// Set Device Context Base Address Array Pointer
		op->CRCR = ((uintptr_t)command_ring & ~0x3F) | 1;  		// The Command Ring Base & Set Cycle Bit = 1

		
		// Set Max Device Slots Enabled, or else we can't call Enable Slot Command
		op->CONFIG |= (maxSlots & 0xFF);

		// Alloc scratchpad buffers
		for (int i = 0; i < maxScratchpad; i++)
		{
			uint32_t* page = malloc_aligned(64, pageSize);
			memset(page, 0, pageSize);
			spbaa[i] = (uintptr_t)page;
		}
		dcbaa[0] = (uintptr_t)spbaa;		// Index 0 in DCBAA is reserved for scratchpad array (if maxScratchpad > 0)

		op->USBCMD |= USBCMD_INTE;	// Enable interrupts
		op->USBCMD |= USBCMD_RS;	// Run
		
		// Reset all ports
		uint8_t maxPorts = (cap->HCSPARAMS1 >> 24) & 0xFF;
		for (uint8_t i = 0; i < maxPorts; i++)
		{
			op->PORTS[i].PORTSC |= PORTSC_PR;
		}


		for (int i = 0; i < 100000000; i++);
		// op->USBSTS = 0;
		// rts->IR[0].IMAN = 0;

		// USB 1.0 and 2.0 need to be restarted to be enabled (PED)
		op->PORTS[2].PORTSC |= PORTSC_PR; // Mouse 
		op->PORTS[3].PORTSC |= PORTSC_PR; // Keyboard

		if (op->PORTS[3].PORTSC & PORTSC_CCS)
			while (!(op->PORTS[3].PORTSC & PORTSC_PED));


		//xhci_setup_device();
		
		// uint64_t CRCR = op->CRCR;
		// uint32_t IMAN = rts->IR[0].IMAN;
		// uint64_t ERDP = rts->IR[0].ERDP;

		// print_hexdump(&CRCR, 8, 5);
		// print_hexdump(&IMAN, 4, 6);
		// print_hexdump(&ERDP, 8, 7);

		XHCI_TRB trb;
		trb.address = 0;
		trb.status = 0;
		trb.control = (9 << 10);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);
		xhci_do_command(trb);





		print_hexdump(&event_ring[0].control, 4, 4);
		print_hexdump(&event_ring[1].control, 4, 5);
		print_hexdump(&event_ring[2].control, 4, 6);
		print_hexdump(&event_ring[3].control, 4, 7);
		print_hexdump(&event_ring[4].control, 4, 8);
		print_hexdump(&event_ring[5].control, 4, 9);
		print_hexdump(&event_ring[6].control, 4, 10);
		print_hexdump(&event_ring[7].control, 4, 11);
		print_hexdump(&event_ring[8].control, 4, 12);
		print_hexdump(&event_ring[9].control, 4, 13);
		print_hexdump(&event_ring[10].control, 4, 14);
		print_hexdump(&event_ring[11].control, 4, 15);
		print_hexdump(&event_ring[12].control, 4, 16);
		print_hexdump(&event_ring[13].control, 4, 17);
		print_hexdump(&event_ring[14].control, 4, 18);
		print_hexdump(&event_ring[15].control, 4, 19);
		print_hexdump(&event_ring[16].control, 4, 20);
		print_hexdump(&event_ring[17].control, 4, 21);
		print_hexdump(&event_ring[18].control, 4, 22);


		// When receiving an interrupt:
		// 1) Clear the Status register bit
		// 2) Clear the Interrupt Management bits (Write the *_MAN_IE and *_MAN_IP bits)
		// 3) Process the TD(s)
		// 4) Update the Dequeue Pointer to point to the last TD processed, clearing the EHB bit at the same time
	}
}


void xhci_setup_device()
{
	// Enable Slot Command
	XHCI_TRB trb;
	trb.address = 0;
	trb.status = 0;
	trb.control = (9 << 10);
	XHCI_TRB event1 = xhci_do_command(trb);

	// Alloc memory for Device Context
	XHCI_CONTEXT* dc = (XHCI_CONTEXT*)malloc_aligned(64, sizeof(XHCI_CONTEXT) * 32);
	memset(dc, 0, sizeof(XHCI_CONTEXT) * 32);
	uint8_t slot = (event1.control >> 24);
	dcbaa[slot] = (uintptr_t)dc;


	uint8_t activePort = 5;   											// Real: 3,4,22
	uint8_t speed = (op->PORTS[activePort].PORTSC >> 10) & 0b1111;

	XHCI_TRB* transfer_ring = (XHCI_TRB*)malloc_aligned(64, sizeof(XHCI_TRB) * 16);

	// Construct Input Context
	XHCI_CONTEXT* inputContext = (XHCI_CONTEXT*)0x450000;
	memset(inputContext, 0, sizeof(XHCI_CONTEXT) * 33);
	// Input Control Context
	inputContext[0].data[1] = 0b11; 		 									

	// Slot Context
	inputContext[1].data[0] = (1 << 27) | (speed << 20) | (0 << 0); 	
	inputContext[1].data[1] = (activePort << 16);

	// Endpoint Context
	inputContext[2].data[0] = (0 << 16) | (0 << 10) | (0 << 8);				// Interval , MaxPStreams , Mult
	inputContext[2].data[1] = (64 << 16) | (0 << 8) | (4 << 3) | (3 << 1);	// MaxPacketSize , MaxBurstSize , EPType , CErr
	inputContext[2].data[2] = ((uintptr_t)transfer_ring) | (1 << 0);		// TRDequeuePointer , DCS
	inputContext[2].data[4] = (32 << 0);									// AverageTRBLength

	// Address Device Command
	XHCI_TRB trb2;
	trb2.address = (uintptr_t)inputContext;
	trb2.status = 0;
	trb2.control =  (slot << 24) | (11 << 10) | (0 << 9);
	XHCI_TRB event2 = xhci_do_command(trb2);

	// Create Transfer TRB (Setup Stage)
	transfer_ring[0].address = ((uint64_t)8 << 48) | ((uint64_t)0 << 32) | (0x100 << 16) | (6 << 8) | (0x80 << 0);
	transfer_ring[0].status = (0 << 22) | (8 << 0);
	transfer_ring[0].control = (3 << 16) | (2 << 10) | (1 << 6) | (0 << 5) | (1 << 0);

	// Data Stage
	uint32_t* data = malloc_aligned(64, 8);
	transfer_ring[1].address = (uintptr_t)data;
	transfer_ring[1].status = (0 << 22) | (0 << 17) | (8 << 0);
	transfer_ring[1].control = (1 << 16) | (3 << 10) | (1 << 0);

	// Status Stage
	transfer_ring[2].address = 0;
	transfer_ring[2].status = 0;
	transfer_ring[2].control = (0 << 16) | (4 << 10) | (1 << 5) | (1 << 0);

	doorbell[slot] = 1;

	XHCI_TRB* next_event = (XHCI_TRB*)((uintptr_t)rts->IR[0].ERDP & ~0x08);
	while (!(next_event->control & (1 << 0)));

	// TODO: Find maxPacketSize from the device descriptor and update it in the host data
	print_hexdump(data, 8, 23);
}

int ln = 2;

XHCI_TRB xhci_do_command(XHCI_TRB trb)
{
	XHCI_TRB* pCmd = xhci_queue_command(trb);

	// Ring doorbell
	doorbell[0] = 0;

	// Wait for completion
	XHCI_TRB* next_event = (XHCI_TRB*)((uintptr_t)rts->IR[0].ERDP & ~0x08);
	for (int i = 0; i < 0xFFFF; i++)
	{
		// Is there a new event in the ring? // TODO: Is this really how you check if a new event is present?
		if (next_event->control & (1 << 0))
		{
			// We have a event, but check TRB type. Must be Command Completion Event
			uint16_t trb_type = (next_event->control >> 10) & 0x3F;
			if (trb_type == 33)
				break;
						
			// It was not the right type. Let's try the next. Increments with sizeof(XHCI_TRB)
			next_event++; 
		}
	}

	uint16_t completion_code = (next_event->status >> 24) & 0xFF;
	uint16_t trb_type 		 = (next_event->control >> 10) & 0x3F;

	// print_hexdump(&completion_code, 2, ln++);
	// print_hexdump(&trb_type, 2, ln++);

	// TODO: It is recommended that software process as many Events as possible before writing the ERDP
	next_event++;
	rts->IR[0].ERDP = (uintptr_t)next_event; // TODO: (Idea) do_command should not change the dequeue pointer. A interrupt handler should do that, but ignore Command Completion Events
	return *(next_event-1);
}

XHCI_TRB* xhci_queue_command(XHCI_TRB trb)
{
	// If last TRB in Command Ring
	if (iCmdEnqueue == 15)
	{
		// Set Link TRB that points to the top
		command_ring[15].address = (uintptr_t)command_ring;
		command_ring[15].status = (0 << 22);
		command_ring[15].control = (6 << 10) | (1 << 5) | (1 << 1) | (bCmdCycle << 0);

		iCmdEnqueue = 0;				// Reset enqueue
		bCmdCycle = !bCmdCycle;			// Flip Cycle
	}

	command_ring[iCmdEnqueue].address = trb.address;
	command_ring[iCmdEnqueue].status = trb.status;
	command_ring[iCmdEnqueue].control = (trb.control & ~1) | (bCmdCycle << 0);

	return &command_ring[iCmdEnqueue++];
}

void xhci_enable_slot_command()
{
	// Build command
	command_ring[0].address = 0;
	command_ring[0].status = 0;
	command_ring[0].control = (1 << 0) | (9 << 10);

	// Ring doorbell
	doorbell[0] = 0;

	while (!(event_ring[0].control & (1 << 0)));
}
