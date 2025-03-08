#include "memory.h"
#include <stdint.h>

// For a 32-bit OS, simplify the paging structures
// Page Directory and Page Table Entries - for 32-bit paging
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_table[1024] __attribute__((aligned(4096)));
static uint32_t page_table2[1024] __attribute__((aligned(4096)));

void init_paging(void) {
    // Clear page tables
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;  // Not present, but writable (for when we do map it)
        page_table[i] = 0;
        page_table2[i] = 0;
    }

    // Identity map the first 4MB of memory (0-4MB)
    // First 1MB for kernel
    for (int i = 0; i < 1024; i++) {
        // Present + Writable + User (0x7)
        page_table[i] = (i * 4096) | 0x3;  // Present + Writable
    }

    // Second 1MB for heap
    for (int i = 0; i < 1024; i++) {
        page_table2[i] = (0x00100000 + (i * 4096)) | 0x3;  // Present + Writable
    }

    // Point the page directory to our page tables
    page_directory[0] = ((uint32_t)page_table) | 0x3;     // Present + Writable
    page_directory[1] = ((uint32_t)page_table2) | 0x3;    // Present + Writable

    // Load the page directory
    asm volatile("mov %0, %%cr3" : : "r"((uint32_t)page_directory));

    // Enable paging
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // Set the paging bit
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}