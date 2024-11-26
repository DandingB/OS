#include "ahci.h"
#include "pci.h"
#include "stdio.h"
#include "i686\x86.h"

#define	AHCI_BASE 0x400000	// 4M

#define APIC_BASE 0xFEE00000  // Base address of the APIC
#define MSI_VECTOR 0x40       // Vector to handle MSI in the IDT

HBA_MEM* init_ahci()
{
	HBA_MEM* hba = 0;

	for (uint32_t device = 0; device < 32; device++)
	{
		uint16_t vendor = pci_get_vendor_id(0, device, 0);
		if (vendor == 0xffff) continue;

		uint8_t class = pci_get_class_id(0, device, 0);
		uint8_t subclass = pci_get_subclass_id(0, device, 0);
		if (class == 0x01 && subclass == 0x06) // We found a PCI device which is a Mass storage and AHCI controller
		{
			uint32_t ahci_base = pci_config_read(0, device, 0, 0x24); // Get the ABAR (BAR5)
			uint8_t msi_cap_offset = find_msi_capability(0, device, 0);

			// Set the MSI address (APIC base with destination ID) and data (vector)
			uint32_t msi_address = APIC_BASE;  // Local APIC base for this CPU
			uint32_t msi_data = MSI_VECTOR;    // Interrupt vector to use for MSI

			// Write the MSI address and data
			pci_config_write(0, device, 0, msi_cap_offset + PCI_MSI_ADDR_OFFSET, msi_address);
			pci_config_write(0, device, 0, msi_cap_offset + PCI_MSI_DATA_OFFSET, msi_data);

			// Enable MSI by setting the MSI Enable bit in the control register
			uint32_t msi_control = pci_config_read(0, device, 0, msi_cap_offset);
			msi_control |= (1 << 16);  // Set MSI Enable bit
			pci_config_write(0, device, 0, msi_cap_offset, msi_control);

			hba = (HBA_MEM*)ahci_base;
		}
	}

	if (hba)
	{	
		hba->ghc |= 1 << 0; // Reset HBA
		while ((hba->ghc & 0x1) != 0); // Wait for Reset bit to become 0 again
		hba->ghc |= 1 << 1; // Enable interrupts

		for (int i = 0; i < 32; i++)
		{
			if ((hba->pi & (1 << i)) != 0) // Check if port i is implemented
			{
				HBA_PORT* port = (HBA_PORT*)&hba->ports[i];
				port_rebase(port, i);
			}
		}
	}
	else
	{
		print("No AHCI found!", 1);
	}

	return hba;
}

void port_rebase(HBA_PORT* port, int portno)
{
	stop_cmd(port);	// Stop command engine

	port->sctl = 0x1; // Reset port
	int i = 0;  while (i < 0xFFFFFF) { i++; } // Wait a little
	port->sctl = 0x0;
	port->ie = 0x01;

	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	port->clb = AHCI_BASE + (portno << 10);
	port->clbu = 0;
	memset((void*)(port->clb), 0, 1024);

	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = AHCI_BASE + (32 << 10) + (portno << 8);
	port->fbu = 0;
	memset((void*)(port->fb), 0, 256);

	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)(port->clb);
	for (int i = 0; i < 32; i++)
	{
		cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
		// 256 bytes per command table, 64+16+48+16*8
		// Command table offset: 40K + 8K*portno + cmdheader_index*256
		cmdheader[i].ctba = AHCI_BASE + (40 << 10) + (portno << 13) + (i << 8);
		cmdheader[i].ctbau = 0;
		memset((void*)cmdheader[i].ctba, 0, 256);
	}

	start_cmd(port);	// Start command engine
}

// Start command engine
void start_cmd(HBA_PORT* port)
{
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR);

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
}

// Stop command engine
void stop_cmd(HBA_PORT* port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;

	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	while (1)
	{
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}

void write(HBA_PORT* port, void* data, uint64_t lba, uint16_t n_sectors)
{
	//port->is = 0xFFFF;

	HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)(port->clb);
	cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);	// Command FIS size in dwords
	cmdheader->prdtl = 1;	// PRDT entries count
	cmdheader->w = 1;		//Write 
	cmdheader->c = 1;

	// Set up command table entry
	HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));
	cmdtbl->prdt_entry[0].dba = (uint32_t)(data); // Buffer address
	cmdtbl->prdt_entry[0].dbc = n_sectors * 512 - 1; // Byte count, should be one less than the actual number (important)
	cmdtbl->prdt_entry[0].i = 1; // Interrupt on completion

	// Set command FIS
	FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(cmdtbl->cfis);
	memset(cmdfis, 0, sizeof(FIS_REG_H2D));

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->command = ATA_CMD_WRITE_DMA_EX;
	cmdfis->control = 0x8;
	cmdfis->device = 0xA0 | (1 << 6);
	cmdfis->c = 1;				// Write command register

	cmdfis->lba0 = lba & 0xFF;
	cmdfis->lba1 = (lba >> 8) & 0xFF;
	cmdfis->lba2 = (lba >> 16) & 0xFF;
	cmdfis->lba3 = (lba >> 24) & 0xFF;
	cmdfis->lba4 = (lba >> 32) & 0xFF;
	cmdfis->lba5 = (lba >> 40) & 0xFF;

	cmdfis->countl = n_sectors & 0xFF;
	cmdfis->counth = (n_sectors >> 8) & 0xFF;

	while (port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)); // Spin on BSY and DRQ if 1

	port->ci = 1 << 0; // Issue command by setting bit 0 in command issued register

	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1 << 0)) == 0)
		{
			break;
		}
		if (port->is & HBA_PxIS_TFES)   // Task file error
		{
			print("Write disk error!    ", 1);
			break;
		}
	}
}


void read(HBA_PORT* port, void* data, uint64_t lba, uint16_t n_sectors)
{
	//port->is = 0xFFFF;

	HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)(port->clb);
	cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);	// Command FIS size in dwords
	cmdheader->prdtl = 1;	// PRDT entries count
	cmdheader->w = 0;
	cmdheader->c = 1;

	// Set up command table entry
	HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));
	cmdtbl->prdt_entry[0].dba = (uint32_t)(data); // Buffer address
	cmdtbl->prdt_entry[0].dbc = n_sectors * 512 - 1; // Byte count, should be one less than the actual number (important)
	cmdtbl->prdt_entry[0].i = 1; // Interrupt on completion

	// Set command FIS
	FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(cmdtbl->cfis);
	memset(cmdfis, 0, sizeof(FIS_REG_H2D));
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->command = ATA_CMD_READ_DMA_EX;
	cmdfis->control = 0x8;
	cmdfis->device = 0xA0 | (1 << 6);	// LBA mode
	cmdfis->c = 1;						// Write command register

	cmdfis->lba0 = lba & 0xFF;
	cmdfis->lba1 = (lba >> 8) & 0xFF;
	cmdfis->lba2 = (lba >> 16) & 0xFF;
	cmdfis->lba3 = (lba >> 24) & 0xFF;
	cmdfis->lba4 = (lba >> 32) & 0xFF;
	cmdfis->lba5 = (lba >> 40) & 0xFF;

	cmdfis->countl = n_sectors & 0xFF;
	cmdfis->counth = (n_sectors >> 8) & 0xFF;

	while (port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)); // Spin on BSY and DRQ if 1

	port->ci = 1 << 0; // Issue command by setting bit 0 in command issued register

	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1 << 0)) == 0)
		{
			break;
		}
		if (port->is & HBA_PxIS_TFES)   // Task file error
		{
			print("Read disk error!   ", 1);
			break;
		}
	}

	int i = 0;
	while(i < 10000)
	{
		// print_hexdump(&port->is, 4, 9);
		i++;
	}

	//while ((port->is & HBA_PxIS_DPS) == 1); // The HBA shall set the PxIS.DPS bit after completing the data transfer, and if enabled, generate an interrupt.
}
