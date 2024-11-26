#include "stdio.h"
#include "pci.h"

void init_xhci()
{
    for (uint32_t device = 0; device < 32; device++)
	{
		uint16_t vendor = pci_get_vendor_id(0, device, 0);
		if (vendor == 0xffff) continue;

		uint8_t class = pci_get_class_id(0, device, 0);
		uint8_t subclass = pci_get_subclass_id(0, device, 0);
        uint8_t progif = pci_get_prog_if(0, device, 0);
		if (class == 0x0C && subclass == 0x03 && progif == 0x30)
		{
			uint32_t xhci_base = pci_config_read(0, device, 0, 0x10);
            print_hexdump(&xhci_base, 4, 2);

            // Set Bus Master in PCI config register
			uint32_t pci_command = pci_config_read(0, device, 0, 4);
			pci_command |= (1 << 2); 
			pci_config_write(0, device, 0, 4, pci_command);
		}
	}
}