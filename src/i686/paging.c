#include <stdint.h>
#include "paging.h"
#include "x86.h"

void print_hexdump(volatile void* loc, int size, unsigned int line);

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 512

#define PAGE_TABLE(pt, n) (pt[PAGE_ENTRIES*n])

__attribute__((aligned(PAGE_SIZE)))
page_directory_entry_t pml4[PAGE_ENTRIES];

__attribute__((aligned(PAGE_SIZE)))
page_directory_entry_t pdpt[PAGE_ENTRIES];

__attribute__((aligned(PAGE_SIZE)))
page_directory_entry_t page_directory[PAGE_ENTRIES];

__attribute__((aligned(PAGE_SIZE)))
page_table_entry_t page_table[PAGE_ENTRIES * 8];

uint8_t iPageTable = 0;

void init_paging()
{
    init_kernel_page_directory();

    // Set temp pml4 with pdpt
    pdpt[0] = ((uintptr_t)page_directory) | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
    pml4[0] = ((uintptr_t)pdpt) | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;

    set_pml4(pml4);
}

void init_kernel_page_directory()
{
    // Identity map the first pages
    init_page_table(&PAGE_TABLE(page_table, 0), 0);
    init_page_table(&PAGE_TABLE(page_table, 1), 0x200000);
    init_page_table(&PAGE_TABLE(page_table, 2), 0x400000);
    init_page_table(&PAGE_TABLE(page_table, 3), 0x600000);

    page_directory[0] = ((uintptr_t)&PAGE_TABLE(page_table, 0)) | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
    page_directory[1] = ((uintptr_t)&PAGE_TABLE(page_table, 1)) | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
    page_directory[2] = ((uintptr_t)&PAGE_TABLE(page_table, 2)) | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
    page_directory[3] = ((uintptr_t)&PAGE_TABLE(page_table, 3)) | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;

    iPageTable = 4;
}

void init_page_table(page_table_entry_t* page_table, uint64_t paddr_start)
{
    for (int i = 0; i < PAGE_ENTRIES; i++)
    {
        page_table[i] = (paddr_start + i * PAGE_SIZE) | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;
    }
}

void set_pml4(uint64_t* pml4)
{
    // Load the page directory base address into CR3
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pml4));

    // Enable paging by setting the PG bit (bit 31) in CR0
    //uint64_t cr0;
    //__asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    //cr0 |= 0x80000000; // Set the PG bit
    //__asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
}

void* map_page_table(uint64_t paddr_start)
{
    // Map page table to physical address
    init_page_table(&PAGE_TABLE(page_table, iPageTable), paddr_start);

    page_directory[iPageTable] = ((uintptr_t)&PAGE_TABLE(page_table, iPageTable)) | PAGE_USER | PAGE_WRITE | PAGE_PRESENT;

    return (void*)(iPageTable++ * 0x200000UL);
}