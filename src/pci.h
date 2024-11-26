#pragma once
#include <stdint.h>

// Define PCI MSI capability registers offsets
#define PCI_MSI_CAP_ID 0x05
#define PCI_MSI_ADDR_OFFSET 0x04
#define PCI_MSI_DATA_OFFSET 0xC

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

uint16_t pci_get_vendor_id(uint16_t bus, uint16_t device, uint16_t function);
uint16_t pci_get_device_id(uint16_t bus, uint16_t device, uint16_t function);
uint16_t pci_get_class_id(uint16_t bus, uint16_t device, uint16_t function);
uint16_t pci_get_subclass_id(uint16_t bus, uint16_t device, uint16_t function);

uint8_t find_msi_capability(uint32_t bus, uint32_t slot, uint32_t func) ;