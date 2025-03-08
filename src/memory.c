#include "memory.h"
#include <stdint.h>

static uint64_t pml4[512] __attribute__((aligned(4096)));
static uint64_t pdp[512] __attribute__((aligned(4096)));
static uint64_t pd[512] __attribute__((aligned(4096)));
static uint64_t pt[512] __attribute__((aligned(4096)));
static uint64_t pt2[512] __attribute__((aligned(4096))); // Maps 0x200000–0x3FFFFF

void init_paging(void) {
    // Clear tables
    for (int i = 0; i < 512; i++) {
        pml4[i] = 0;
        pdp[i] = 0;
        pd[i] = 0;
        pt[i] = 0;
        pt2[i] = 0;
    }

    // Map 0x000000–0x1FFFFF (first 2MB)
    for (int i = 0; i < 512; i++) {
        pt[i] = (i * 4096) | 3; // Present + Writable
    }

    // Map 0x200000–0x3FFFFF (next 2MB for heap)
    for (int i = 0; i < 512; i++) {
        pt2[i] = (0x200000 + i * 4096) | 3; // Present + Writable
    }

    // Link page tables to directory
    pd[0] = (uint64_t)pt | 3;
    pd[1] = (uint64_t)pt2 | 3;
    pdp[0] = (uint64_t)pd | 3;
    pml4[0] = (uint64_t)pdp | 3;

    // Load page table and enable paging
    asm volatile("mov %0, %%cr3" : : "r"((uint64_t)pml4));
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}