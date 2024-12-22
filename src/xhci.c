#include "stdio.h"
#include "pci.h"
#include "xhci.h"
#include "memory.h"
#include "i686/x86.h"
#include "i686/paging.h"

uintptr_t xhci_base = 0;

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

uint8_t iTransferEnqueue = 0;
uint8_t bTransferCycle = 1;

uint8_t bEventCycle = 1;

void init_xhci()
{
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

			xhci_base = map_page_table(xhci_base);

			//if (!pci_set_msix(0, device, 0, 0xFEE00000, 0x41))
			//	print("Failed to setup MSI-X!", 2);

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
		uint8_t maxERST = (cap->HCSPARAMS2 >> 4) & 0b1111;
		uint8_t contextSize = (cap->HCCPARAMS1 >> 2) & 1;	// 0 = 32 byte contexts, 1 = 64 byte contexts

		command_ring = (XHCI_TRB*)malloc_aligned(64, sizeof(XHCI_TRB) * 16);
		event_ring = (XHCI_TRB*)malloc_aligned(64, sizeof(XHCI_TRB) * 16 * 2);
		dcbaa = (uint64_t*)malloc_aligned(64, sizeof(uint64_t) * (maxSlots+1));

		memset((void*)dcbaa, 0, sizeof(uint64_t) * (maxSlots+1));
		memset((void*)command_ring, 0, sizeof(XHCI_TRB) * 4);
		memset((void*)event_ring, 0, sizeof(XHCI_TRB) * 16 * 2);

		xhci_claim_ownership();
		xhci_reset_hc();
		xhci_alloc_scratchpad();
		xhci_setup_interrupter_register(0, event_ring);

		op->DCBAAP = (uintptr_t)dcbaa;  	  			// Set Device Context Base Address Array Pointer
		op->CRCR = ((uintptr_t)command_ring & ~0x3F) | 1;  		// The Command Ring Base & Set Cycle Bit = 1
		
		// Set Max Device Slots Enabled, or else we can't call Enable Slot Command
		op->CONFIG |= (maxSlots & 0xFF);

		op->USBCMD |= USBCMD_INTE;	// Enable interrupts
		op->USBCMD |= USBCMD_RS;	// Run

		xhci_reset_ports();

		// USB 1.0 and 2.0 need to be restarted to be enabled (PED)
		op->PORTS[2].PORTSC |= PORTSC_PR; // Mouse 
		op->PORTS[3].PORTSC |= PORTSC_PR; // Keyboard

		if (op->PORTS[3].PORTSC & PORTSC_CCS)
			while (!(op->PORTS[3].PORTSC & PORTSC_PED));


		// When receiving an interrupt:
		// 1) Clear the Status register bit
		// 2) Clear the Interrupt Management bits (Write the *_MAN_IE and *_MAN_IP bits)
		// 3) Process the TD(s)
		// 4) Update the Dequeue Pointer to point to the last TD processed, clearing the EHB bit at the same time


		 xhci_setup_device(5);

		 print_hexdump(&event_ring[0].status, 4, 4);
		 print_hexdump(&event_ring[1].status, 4, 5);
		 print_hexdump(&event_ring[2].status, 4, 6);
		 print_hexdump(&event_ring[3].status, 4, 7);
		 print_hexdump(&event_ring[4].status, 4, 8);
		 print_hexdump(&event_ring[5].status, 4, 9);
		 print_hexdump(&event_ring[6].status, 4, 10);
		 print_hexdump(&event_ring[7].status, 4, 11);
		 print_hexdump(&event_ring[8].status, 4, 12);
		 print_hexdump(&event_ring[9].status, 4, 13);
		 print_hexdump(&event_ring[10].status, 4, 14);
		 print_hexdump(&event_ring[11].status, 4, 15);
		 print_hexdump(&event_ring[12].status, 4, 16);
		 print_hexdump(&event_ring[13].status, 4, 17);
		 print_hexdump(&event_ring[14].status, 4, 18);
		 print_hexdump(&event_ring[15].status, 4, 19);


	}
}

void xhci_reset_hc()
{
	// Reset the Host Controller & Wait...
	op->USBCMD |= USBCMD_HCRST;
	//print("HC Resetting", 3); // TODO: Maybe a memory barrier here
	while (op->USBCMD & USBCMD_HCRST);
	//print("               ", 3);
}

void xhci_reset_ports()
{
	// Reset all ports
	uint8_t maxPorts = (cap->HCSPARAMS1 >> 24) & 0xFF;
	for (uint8_t i = 0; i < maxPorts; i++)
	{
		op->PORTS[i].PORTSC |= PORTSC_PR;
	}

	for (int i = 0; i < 0xFFFF; i++);

	for (uint8_t i = 0; i < maxPorts; i++)
	{
		uint32_t test = op->PORTS[i].PORTSC;
		print_hexdump(&test, 4, 4 + i);
	}
}

void xhci_claim_ownership()
{
	// Claim ownership
	uintptr_t xecp = xhci_base + (((cap->HCCPARAMS1 >> 16) & 0xFFFF) << 2);
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
}

void xhci_setup_interrupter_register(uint16_t reg, XHCI_TRB* er)
{
	// Setup Event Ring Segment Table
	XHCI_ERST* segment_table = (XHCI_ERST*)malloc_aligned(64, sizeof(XHCI_ERST) * 1);
	memset(segment_table, 0, sizeof(XHCI_ERST) * 1);
	segment_table[0].address = (uintptr_t)&er[0];
	segment_table[0].size = 16;

	// Setup Interrupter Register Set 0
	rts->IR[reg].IMAN |= IMAN_IE;
	rts->IR[reg].IMOD = (0 & 0xFFFF) | ((0 & 0xFFFF) << 16);
	rts->IR[reg].ERSTSZ = 1;									// Table Size
	rts->IR[reg].ERSTBA = (uintptr_t)segment_table;				// Table Base Address Pointer
	rts->IR[reg].ERDP = (uintptr_t)(er) & ~ERDP_EHB;			// Dequeue pointer
}

void xhci_alloc_scratchpad()
{
	uint64_t pageSize = calculate_page_size(op->PAGESIZE);
	uint16_t maxScratchpad = ((cap->HCSPARAMS2 >> 16) & 0x3E0) | (cap->HCSPARAMS2 >> 27);

	spbaa = (uint64_t*)malloc_aligned(64, sizeof(uint64_t) * maxScratchpad);

	// Alloc scratchpad buffers
	for (int i = 0; i < maxScratchpad; i++)
	{
		uint32_t* page = malloc_aligned(64, pageSize);
		memset(page, 0, pageSize);
		spbaa[i] = (uintptr_t)page;
	}
	dcbaa[0] = (uintptr_t)spbaa;		// Index 0 in DCBAA is reserved for scratchpad array (if maxScratchpad > 0)
}

// Real ports: 3,4,22
void xhci_setup_device(uint8_t port)
{
	// Enable Slot Command
	XHCI_TRB trb;
	trb.address = 0;
	trb.status = 0;
	trb.control = TRB_SET_TYPE(ENABLE_SLOT);
	XHCI_TRB event1 = xhci_do_command(trb);

	// Alloc memory for Device Context
	XHCI_CONTEXT* dc = (XHCI_CONTEXT*)malloc_aligned(64, sizeof(XHCI_CONTEXT) * 32);
	memset(dc, 0, sizeof(XHCI_CONTEXT) * 32);
	uint8_t slot = TRB_GET_SLOT(event1.control);
	dcbaa[slot] = (uintptr_t)dc;
											
	uint8_t speed = (op->PORTS[port].PORTSC >> 10) & 0b1111;

	XHCI_TRB* transfer_ring = (XHCI_TRB*)malloc_aligned(64, sizeof(XHCI_TRB) * 16);
	XHCI_TRB* transfer_ring_1 = (XHCI_TRB*)malloc_aligned(64, sizeof(XHCI_TRB) * 16);

	// Construct Input Context
	XHCI_CONTEXT* inputContext = (XHCI_CONTEXT*)malloc_aligned(64, sizeof(XHCI_CONTEXT) * 33);
	memset(inputContext, 0, sizeof(XHCI_CONTEXT) * 33);
	// Input Control Context
	inputContext[0].data[1] = 0b011; 		 									
	// Slot Context
	inputContext[1].data[0] = SLOT_SET_CONTEXTENTRIES(1) | SLOT_SET_SPEED(speed) | SLOT_SET_ROUTE(0); 	
	inputContext[1].data[1] = SLOT_SET_ROOTPORT(port);
	// Endpoint Context
	inputContext[2].data[0] = (0 << 16) | (0 << 10) | (0 << 8);				// Interval , MaxPStreams , Mult
	inputContext[2].data[1] = (64 << 16) | (0 << 8) | (4 << 3) | (3 << 1);	// MaxPacketSize , MaxBurstSize , EPType , CErr // TODO: MaxPacketSize should be selected from device speed
	inputContext[2].data[2] = ((uintptr_t)transfer_ring) | (1 << 0);		// TRDequeuePointer , DCS
	inputContext[2].data[4] = (32 << 0);									// AverageTRBLength

	// Address Device Command
	XHCI_TRB trb2;
	trb2.address = (uintptr_t)inputContext;
	trb2.status = 0;
	trb2.control = TRB_SET_SLOT(slot) | TRB_SET_TYPE(ADDRESS_DEVICE) | TRB_SET_BSR(0);
	XHCI_TRB event2 = xhci_do_command(trb2);

	// Get Device Descriptor
	USB_DEVICE_DESCRIPTOR descriptor;
	xhci_get_descriptor(slot, transfer_ring, 16, &descriptor);

	// Update input context with correct MaxPacketSize
	inputContext[0].data[1] = 0b010;
	inputContext[2].data[1] = (descriptor.bMaxPacketSize0 << 16) | (0 << 8) | (4 << 3) | (3 << 1);
	// Evaluate Context Command
	XHCI_TRB trb3;
	trb3.address = (uintptr_t)inputContext;
	trb3.status = 0;
	trb3.control = TRB_SET_SLOT(slot) | TRB_SET_TYPE(EVALUATE_CONTEXT) | TRB_SET_BSR(0);
	XHCI_TRB event3 = xhci_do_command(trb3);


	inputContext[0].data[1] = 0b1001;
								// Remember to set
	inputContext[1].data[0] = SLOT_SET_CONTEXTENTRIES(3) | SLOT_SET_SPEED(speed) | SLOT_SET_ROUTE(0); 	
	inputContext[1].data[1] = SLOT_SET_ROOTPORT(port);

	inputContext[2].data[0] = 0;
	inputContext[2].data[1] = 0;
	inputContext[2].data[2] = 0;
	inputContext[2].data[4] = 0;
	// Endpoint 1 IN
	inputContext[4].data[0] = (0 << 16) | (0 << 10) | (0 << 8);				// Interval , MaxPStreams , Mult
	inputContext[4].data[1] = (8 << 16) | (0 << 8) | (4 << 3) | (3 << 1);	// MaxPacketSize , MaxBurstSize , EPType , CErr
	inputContext[4].data[2] = ((uintptr_t)transfer_ring_1) | (1 << 0);		// TRDequeuePointer , DCS
	inputContext[4].data[4] = (32 << 0);									// AverageTRBLength

	// Configure Endpoint Command
	XHCI_TRB trb4;
	trb4.address = (uintptr_t)inputContext;
	trb4.status = 0;
	trb4.control = TRB_SET_SLOT(slot) | TRB_SET_TYPE(CONFIGURE_ENDPOINT);
	XHCI_TRB event4 = xhci_do_command(trb4);

	xhci_set_configuration(slot, transfer_ring);


	XHCI_CONTEXT* deviceContext = (XHCI_CONTEXT*)dcbaa[1];
	print_hexdump(&deviceContext[1].data[1], 4, 22);

	print_hexdump(&descriptor, 16, 23);

}


XHCI_TRB xhci_do_command(XHCI_TRB trb)
{
	XHCI_TRB* pCmd = xhci_queue_command(trb);

	// Ring doorbell
	doorbell[0] = 0;

	return *xhci_dequeue_event(COMMAND_COMP_EVENT);
}

XHCI_TRB xhci_get_descriptor(uint8_t slot, XHCI_TRB* transfer_ring, uint8_t length, volatile void* buffer)
{
	XHCI_TRB setup;
	setup.address = TRB_SET_wLength(length) | TRB_SET_wIndex(0) | TRB_SET_wValue(0x0100) | TRB_SET_bRequest(6) | TRB_SET_bmRequestType(0x80);
	setup.status = TRB_SET_INT_TARGET(0) | TRB_SET_TRANSFER_LENGTH(8);
	setup.control = TRB_SET_TT(IN_DATA) | TRB_SET_TYPE(SETUP_STAGE) | TRB_SET_IDT(1) | TRB_SET_IOC(0) | TRB_SET_CYCLE(1);
	xhci_queue_transfer(setup, transfer_ring);

	XHCI_TRB data;
	data.address = (uintptr_t)buffer;
	data.status = TRB_SET_INT_TARGET(0) | TRB_SET_TDSIZE(0) | TRB_SET_TRANSFER_LENGTH(length);
	data.control = TRB_SET_DIR(1) | TRB_SET_TYPE(DATA_STAGE) | TRB_SET_IOC(0) | TRB_SET_CYCLE(1);
	xhci_queue_transfer(data, transfer_ring);

	XHCI_TRB status;
	status.address = 0;
	status.status = TRB_SET_INT_TARGET(0);
	status.control = TRB_SET_DIR(0) | TRB_SET_TYPE(STATUS_STAGE) | TRB_SET_IOC(1) | TRB_SET_CYCLE(1);
	xhci_queue_transfer(status, transfer_ring);

	doorbell[slot] = 1; // Ring Default Endpoint

	xhci_dequeue_event(TRANSFER_EVENT);
}

void xhci_set_configuration(uint8_t slot, XHCI_TRB* transfer_ring)
{
	XHCI_TRB setup;
	setup.address = TRB_SET_wLength(0) | TRB_SET_wIndex(0) | TRB_SET_wValue(0x0001) | TRB_SET_bRequest(9) | TRB_SET_bmRequestType(0x00);
	setup.status = TRB_SET_INT_TARGET(0) | TRB_SET_TRANSFER_LENGTH(8);
	setup.control = TRB_SET_TT(NO_DATA) | TRB_SET_TYPE(SETUP_STAGE) | TRB_SET_IDT(1) | TRB_SET_IOC(0) | TRB_SET_CYCLE(1);
	xhci_queue_transfer(setup, transfer_ring);

	XHCI_TRB status;
	status.address = 0;
	status.status = TRB_SET_INT_TARGET(0);
	status.control = TRB_SET_DIR(1) | TRB_SET_TYPE(STATUS_STAGE) | TRB_SET_IOC(1) | TRB_SET_CYCLE(1);
	xhci_queue_transfer(status, transfer_ring);

	doorbell[slot] = 1; // Ring Default Endpoint

	//xhci_dequeue_event(TRANSFER_EVENT);
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

XHCI_TRB* xhci_queue_transfer(XHCI_TRB trb, XHCI_TRB* transfer_ring)
{
	// If last TRB in Transfer Ring
	if (iTransferEnqueue == 15)
	{
		// Set Link TRB that points to the top
		transfer_ring[15].address = (uintptr_t)transfer_ring;
		transfer_ring[15].status = (0 << 22);
		transfer_ring[15].control = (6 << 10) | (1 << 5) | (1 << 1) | (bTransferCycle << 0);

		iTransferEnqueue = 0;				// Reset enqueue
		bTransferCycle = !bTransferCycle;	// Flip Cycle
	}

	transfer_ring[iTransferEnqueue].address = trb.address;
	transfer_ring[iTransferEnqueue].status = trb.status;
	transfer_ring[iTransferEnqueue].control = (trb.control & ~1) | (bTransferCycle << 0);

	return &transfer_ring[iTransferEnqueue++];
}

XHCI_TRB* xhci_dequeue_event(uint8_t trb_type)
{
	XHCI_TRB* next_event = (XHCI_TRB*)((uintptr_t)rts->IR[0].ERDP & ~0xF);
	
	//for (int i = 0; i < 0xFFFF; i++)
	while (1)
	{
		uint16_t completion_code = (next_event->status >> 24) & 0xFF;
		// Is there a new event in the ring? // TODO: Is this really how you check if a new event is present?
		if ((next_event->control & 1) == bEventCycle && completion_code != 0)
		{
			// We have a event, but check TRB type. Must be Command Completion Event
			if (TRB_GET_TYPE(next_event->control) == trb_type)
				break;
						
			// It was not the right type. Let's try the next. Increments with sizeof(XHCI_TRB)
			next_event++;
			if (next_event >= event_ring + 16)
			{
				next_event = event_ring;
				bEventCycle = !bEventCycle;
			}
		}
	}

	XHCI_TRB* event = next_event;
	uint16_t completion_code = (next_event->status >> 24) & 0xFF;
	uint16_t trb_type2 		 = (next_event->control >> 10) & 0x3F;

	// print_hexdump(&completion_code, 2, ln++);
	// print_hexdump(&trb_type2, 2, ln++);

	// TODO: It is recommended that software process as many Events as possible before writing the ERDP
	next_event++;
	if (next_event >= event_ring + 16)
	{
		next_event = event_ring;
		bEventCycle = !bEventCycle;
	}

	rts->IR[0].ERDP = (uintptr_t)next_event; // TODO: (Idea) do_command should not change the dequeue pointer. A interrupt handler should do that, but ignore Command Completion Events
	return event;
}


uint32_t get_xhci_base_address(uint8_t bus, uint8_t device, uint8_t function)
{
	uint32_t bar = pci_config_read(bus, device, function, 0x10); // Read the first BAR

	// Check if it is a memory-mapped BAR
	if (bar & 0x01) {
		// I/O port BAR (not common for xHCI)
		return bar & 0xFFFFFFFC;
	}
	else {
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
int asd = 0;
void xhci_interrupt_handler()
{
	//XHCI_TRB* next_event = (XHCI_TRB*)((uintptr_t)rts->IR[0].ERDP & ~0xF);

	////if ((rts->IR[0].ERDP & ~0xF) != event_ring)
	//	next_event++;

	//if (next_event >= event_ring + 16)
	//{
	//	next_event = event_ring;
	//	bEventCycle = !bEventCycle;
	//}

	//print("XHCI interrupt!", 1);



	//// Write 1 to clear and set dequeue pointer
	//rts->IR[0].IMAN = IMAN_IE | IMAN_IP;
	//rts->IR[0].ERDP = (uintptr_t)(next_event) | ERDP_EHB;

	//uint64_t test = rts->IR[0].ERDP & ~0xF;
	//print_hexdump(&test, 8, 12+(asd++));
}
