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
	uint32_t flags;
} XHCI_TRB;

XHCI_BASE init_xhci();
void xhci_reset(XHCI_BASE xhci_base);
void xhci_ports_list(XHCI_BASE xhci_base);