#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

// Page size is 4KB
#define PAGE_SIZE 4096

// Memory protection flags
#define MEM_PROT_NONE  0x00  // No access
#define MEM_PROT_READ  0x01  // Read access
#define MEM_PROT_WRITE 0x02  // Write access
#define MEM_PROT_EXEC  0x04  // Execute access
#define MEM_PROT_USER  0x08  // User-mode access (vs. kernel-only)

// Memory region types
#define MEM_REGION_KERNEL    0x01  // Kernel code and data
#define MEM_REGION_HEAP      0x02  // Kernel heap
#define MEM_REGION_USER      0x03  // User space
#define MEM_REGION_HARDWARE  0x04  // Memory-mapped hardware

// Initialize paging system
void init_paging(void);

// Map a physical address to a virtual address with protection flags
int map_page(uint32_t physical_addr, uint32_t virtual_addr, uint32_t flags);

// Unmap a virtual address
int unmap_page(uint32_t virtual_addr);

// Set protection flags for a virtual address
int protect_page(uint32_t virtual_addr, uint32_t flags);

// Get the physical address for a virtual address
uint32_t get_physical_address(uint32_t virtual_addr);

// Enable memory protection
void enable_memory_protection(void);

// Disable memory protection
void disable_memory_protection(void);

// Check if an address is valid for access with given protection flags
int is_valid_access(uint32_t virtual_addr, uint32_t access_flags);

// Memory protection fault handler
void memory_fault_handler(uint32_t fault_addr, uint32_t error_code);

// Display memory region information
void display_memory_regions(void);

#endif