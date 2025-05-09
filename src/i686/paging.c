#include <stdint.h>

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024

// Page directory and page table entry structure
typedef uint32_t page_directory_entry_t;
typedef uint32_t page_table_entry_t;


// Page directory and a single page table for simplicity
__attribute__((aligned(PAGE_SIZE)))
page_directory_entry_t page_directory[PAGE_ENTRIES];

__attribute__((aligned(PAGE_SIZE)))
page_table_entry_t page_table[PAGE_ENTRIES * 4];

void initialize_page_table(page_table_entry_t* page_table, uint32_t vaddr_start)
{
    for (int i = 0; i < PAGE_ENTRIES; i++) 
    {
        page_table[i] = (vaddr_start + i * PAGE_SIZE) | 0x3; // Present (1) | Read/Write (1)
    }
}

void initialize_page_directory(page_directory_entry_t* page_directory)
{
    // Clear other entries for simplicity (optional)
    for (int i = 0; i < PAGE_ENTRIES; i++)
        page_directory[i] = 0;

    page_directory[0] = ((uint32_t)&page_table[0*PAGE_ENTRIES]) | 0x3; // Present (1) | Read/Write (1)
    page_directory[1] = ((uint32_t)&page_table[1*PAGE_ENTRIES]) | 0x3; // Present (1) | Read/Write (1)
    page_directory[1018] = ((uint32_t)&page_table[2*PAGE_ENTRIES]) | 0x3; // Present (1) | Read/Write (1)
    page_directory[1019] = ((uint32_t)&page_table[3*PAGE_ENTRIES]) | 0x3; // Present (1) | Read/Write (1)

}

void enable_paging(uint32_t* page_directory) 
{
    // Load the page directory base address into CR3
    __asm__ volatile ("mov %0, %%cr3" : : "r"(page_directory));

    // Enable paging by setting the PG bit (bit 31) in CR0
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Set the PG bit
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));
}

void init_paging() 
{
    initialize_page_table(&page_table[0*PAGE_ENTRIES], 0 * PAGE_SIZE * PAGE_ENTRIES);
    initialize_page_table(&page_table[1*PAGE_ENTRIES], 1 * PAGE_SIZE * PAGE_ENTRIES);
    initialize_page_table(&page_table[2*PAGE_ENTRIES], 1018 * PAGE_SIZE * PAGE_ENTRIES); // AHCI base
    initialize_page_table(&page_table[3*PAGE_ENTRIES], 1019 * PAGE_SIZE * PAGE_ENTRIES); // APIC base
    initialize_page_directory(page_directory);
    enable_paging(page_directory);
}