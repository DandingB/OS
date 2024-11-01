#pragma once
#include <stdint.h>

uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

uint16_t getVendorID(uint16_t bus, uint16_t device, uint16_t function);
uint16_t getDeviceID(uint16_t bus, uint16_t device, uint16_t function);
uint16_t getClassId(uint16_t bus, uint16_t device, uint16_t function);
uint16_t getSubClassId(uint16_t bus, uint16_t device, uint16_t function);