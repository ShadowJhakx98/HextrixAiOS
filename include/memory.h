// include/memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

// Constants
#define PAGE_SIZE 4096

// Memory protection flags
#define MEM_PROT_READ   0x01
#define MEM_PROT_WRITE  0x02
#define MEM_PROT_EXEC   0x04
#define MEM_PROT_USER   0x08

// Memory region types
#define MEM_REGION_KERNEL  0
#define MEM_REGION_HEAP    1
#define MEM_REGION_USER    2

// Function declarations
void init_paging(void);
int map_page(uint32_t physical_addr, uint32_t virtual_addr, uint32_t flags);
int unmap_page(uint32_t virtual_addr);
int protect_page(uint32_t virtual_addr, uint32_t flags);
uint32_t get_physical_address(uint32_t virtual_addr);
void enable_memory_protection(void);
void disable_memory_protection(void);
int is_valid_access(uint32_t virtual_addr, uint32_t access_flags);
void memory_fault_handler(uint32_t fault_addr, uint32_t error_code);
void display_memory_regions(void);

// Enhanced memory management
void memory_enhanced_init(void);
void* memory_alloc(uint32_t size, uint32_t flags, const char* type, const char* by);
void memory_free(void* ptr);
void display_memory_statistics(void);
int enhanced_memory_check(uint32_t address, uint32_t size, uint32_t flags);
void display_memory_map(void);

#endif // MEMORY_H