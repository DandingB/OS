#pragma once 

#define PAGE_PRESENT	(1 << 0)
#define PAGE_WRITE		(1 << 1)
#define PAGE_USER		(1 << 2)

typedef uint64_t page_directory_entry_t;
typedef uint64_t page_table_entry_t;

void  init_paging();
void  init_kernel_page_directory();
void  init_page_table(page_table_entry_t* page_table, uint64_t paddr_start);
void  set_pml4(uint64_t* pml4);
void* map_page_table(uint64_t paddr_start);