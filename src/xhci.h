#include <stdint.h>

#define USBCMD_RS       (1 << 0)    // Run/Stop
#define USBCMD_HCRST    (1 << 1)    // Reset
#define USBCMD_INTE     (1 << 2)    // Interrupter Enable

#define USBSTS_HCH      (1 << 0)    // HCHalted  This bit is a ‘0’ whenever the Run/Stop (R/S) bit is a ‘1’.
#define USBSTS_CNR      (1 << 11)   // Controller not ready

#define PORTSC_CCS      (1 << 0)    // Current Connect Status
#define PORTSC_PED      (1 << 1)    // Port Enabled/Disabled
#define PORTSC_PR       (1 << 4)    // Port Reset

#define IMAN_IP         (1 << 0)    // Interrupt Pending
#define IMAN_IE         (1 << 1)    // Interrupt Enable

#define ERDP_EHB        (1 << 3)    // Event Handler Busy

// TRB Types
#define SETUP_STAGE         2
#define DATA_STAGE          3
#define STATUS_STAGE        4
#define ENABLE_SLOT         9
#define DISABLE_SLOT        10
#define ADDRESS_DEVICE      11
#define CONFIGURE_ENDPOINT  12
#define EVALUATE_CONTEXT    13

#define TRANSFER_EVENT      32
#define COMMAND_COMP_EVENT  33
#define PORT_STATUS_EVENT   34

// Transfer Types
#define NO_DATA 0
#define OUT_DATA 2
#define IN_DATA 3

// Bit fields
#define TRB_SET_BSR(bsr)                ((bsr & 1) << 9)
#define TRB_SET_TYPE(type)              ((type & 0x3F) << 10)
#define TRB_GET_TYPE(type)              ((type >> 10) & 0x3F)
#define TRB_SET_SLOT(slot)              (slot << 24)
#define TRB_GET_SLOT(slot)              (slot >> 24)
#define TRB_SET_CYCLE(state)            (state & 1)

#define TRB_SET_bmRequestType(type)     (type & 0xFF)
#define TRB_SET_bRequest(req)           ((req & 0xFF) << 8)
#define TRB_SET_wValue(value)           ((value & 0xFFFF) << 16)
#define TRB_SET_wIndex(index)           ((uint64_t)(index & 0xFFFF) << 32)
#define TRB_SET_wLength(length)         ((uint64_t)(length & 0xFFFF) << 48)
#define TRB_SET_TRANSFER_LENGTH(length) (length & 0x1FFFF)
#define TRB_SET_INT_TARGET(target)      (target << 22)          // Interrupter Target
#define TRB_SET_IOC(bool)               ((bool & 1) << 5)       // Interrupt on Completion
#define TRB_SET_IDT(bool)               ((bool & 1) << 6)       // Immediate Data
#define TRB_SET_TT(bool)                ((bool & 3) << 16)      // Transfer Type
#define TRB_SET_TDSIZE(size)            ((size & 0x1F) << 17)   // TD Size
#define TRB_SET_DIR(dir)                ((dir & 1) << 16)       // Direction
#define TRB_SET_CH(chain)               ((chain & 1) << 4)        // Chain

#define SLOT_SET_ROUTE(string)          (string)                // Route String
#define SLOT_SET_SPEED(speed)           ((speed & 0xF) << 20)   // This field indicates the speed of the device.
#define SLOT_SET_MTT(mtt)               ((mtt & 0x1) << 25)     // Multi-TT
#define SLOT_SET_HUB(hub)               ((hub & 0x1) << 26)     // This flag is set to '1' by software if this device is a USB hub, or '0' if it is a USB function. 
#define SLOT_SET_CONTEXTENTRIES(n)      (n << 27)               // Context entries
#define SLOT_SET_ROOTPORT(port)         ((port & 0xFF) << 16)   // Root hub port number
#define SLOT_SET_NPORTS(port)           ((port & 0xFF) << 16)   // Number of ports




typedef void* XHCI_BASE;

// Reading from this MMIO is 4-byte aligned, so some fields a consolidated to fit this requirement.
typedef volatile struct tagXHCI_CAP
{
    uint32_t CAPLENGTH_HCIVERSION;
    uint32_t HCSPARAMS1;    // Structural Parameters 1
    uint32_t HCSPARAMS2;    // Structural Parameters 2
    uint32_t HCSPARAMS3;    // Structural Parameters 3
    uint32_t HCCPARAMS1;    // Capability Parameters 1 
    uint32_t DBOFF;         // Doorbell Offset 
    uint32_t RTSOFF;        // Runtime Register Space Offset
    uint32_t HCCPARAMS2;    // Capability Parameters 2
} XHCI_REG_CAP;

typedef volatile struct tagXHCI_PORT
{
    uint32_t PORTSC;    // Port Status and Control
    uint32_t PORTPMSC;  // Port Power Management Status and Control
    uint32_t PORTLI;    // Port Link Info
    uint32_t PORTHLPMC; // Port Hardware LPM Control

} XHCI_PORT;

typedef volatile struct tagXHCI_OP
{
    uint32_t USBCMD;     // USB Command
    uint32_t USBSTS;     // USB Status
    uint32_t PAGESIZE;   // Page Size
    uint32_t rsv1[2];
    uint32_t DNCTRL;     // Device Notification Control
    uint64_t CRCR;       // Command Ring Control
    uint32_t rsv2[4];
    uint64_t DCBAAP;     // Device Context Base Address Array Pointer
    uint32_t CONFIG;     // Configure
    uint32_t rsv3[241];
    XHCI_PORT PORTS[1];  // Port Register Sets (Size in HCSPARAMS1 > MaxPorts)
} XHCI_REG_OP;

typedef volatile struct tagXHCI_IRS
{
    uint32_t IMAN;      // Interrupter Management
    uint32_t IMOD;      // Interrupter Moderation
    uint32_t ERSTSZ;    // Event Ring Segment Table Size
    uint32_t rsvdp;
    uint64_t ERSTBA;    // Event Ring Segment Table Base Address 
    uint64_t ERDP;      // Event Ring Dequeue Pointer
} XHCI_IRS;

typedef volatile struct tagXHCI_RUNTIME
{
    uint32_t MFINDEX;   // Microframe Index 
    uint32_t rsvdz[7];
    XHCI_IRS IR[1];     // Interrupter Register Sets (Size in HCSPARAMS1 > MaxIntrs)
} XHCI_REG_RT;


typedef struct tagXHCI_ERST
{
    uint64_t address;
    uint16_t size;
    uint16_t rsvdz1;
    uint32_t rsvdz2;
} XHCI_ERST;

typedef volatile struct tagXHCI_TRB
{
	uint64_t address;
	uint32_t status;
	uint32_t control;
} XHCI_TRB;

typedef struct tagXHCI_CONTEXT
{
    uint32_t data[8];
} XHCI_CONTEXT;



typedef volatile struct tagUSB_DEVICE_DESCRIPTOR
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) USB_DEVICE_DESCRIPTOR;


void init_xhci();
void xhci_setup_device(uint8_t port);

XHCI_TRB xhci_do_command(XHCI_TRB trb);

XHCI_TRB xhci_get_descriptor(uint8_t slot, XHCI_TRB* transfer_ring, uint8_t length, volatile void* buffer);
void xhci_set_configuration(uint8_t slot, XHCI_TRB* transfer_ring);

XHCI_TRB* xhci_queue_command(XHCI_TRB trb);
XHCI_TRB* xhci_queue_transfer(XHCI_TRB trb, XHCI_TRB* transfer_ring);
XHCI_TRB* xhci_dequeue_event(uint8_t trb_type);
