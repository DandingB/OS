#pragma once
#include <stdint.h>

#define PCI_CLASS_MASSSTORAGE   0x01
#define PCI_CLASS_NETWORK       0x02
#define PCI_CLASS_DISPLAY       0x03
#define PCI_CLASS_MULTIMEDIA    0x04
#define PCI_CLASS_SERIALBUS     0x0C

#define PCI_SUBCLASS_SATA       0x06
#define PCI_SUBCLASS_USB        0x03

#define PCI_PROGIF_XHCI         0x30

#define PCI_COMMAND_IO          0x001
#define PCI_COMMAND_MEMORY      0x002
#define PCI_COMMAND_BUSMASTER   0x004

#define PCI_CAP_MSI             0x05
#define PCI_CAP_MSIX            0x11

#define PCI_MSI_ADDR_OFFSET     0x04
#define PCI_MSI_DATA_OFFSET     0x0C


#define PCI_BAR(i) (0x10 + (0x4 * i))



uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

uint16_t pci_get_vendor_id(uint16_t bus, uint16_t device, uint16_t function);
uint16_t pci_get_device_id(uint16_t bus, uint16_t device, uint16_t function);
uint8_t pci_get_class_id(uint16_t bus, uint16_t device, uint16_t function);
uint8_t pci_get_subclass_id(uint16_t bus, uint16_t device, uint16_t function);
uint8_t pci_get_prog_if(uint16_t bus, uint16_t device, uint16_t function);

uint8_t find_capability(uint32_t bus, uint32_t device, uint32_t func, uint8_t cap);
int pci_set_msi(uint32_t bus, uint32_t device, uint32_t func, uint32_t apic_base, uint32_t vector);
int pci_set_msix(uint32_t bus, uint32_t device, uint32_t func, uint32_t apic_base, uint32_t vector);
