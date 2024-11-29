#include <stdint.h>

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
} XHCI_CAP;

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
} XHCI_OP;

uint32_t* init_xhci();
void reset_xhci_controller(uint32_t* mmio_base);