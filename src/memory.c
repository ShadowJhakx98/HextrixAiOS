#include "memory.h"
#include "stdio.h"
#include "terminal.h"
#include "system_utils.h"
#include <stdint.h>

// For a 32-bit OS, simplify the paging structures
// Page Directory and Page Table Entries - for 32-bit paging
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_table_kernel[1024] __attribute__((aligned(4096)));
static uint32_t page_table_heap[1024] __attribute__((aligned(4096)));

// Constants for page table/directory entry flags
#define PAGE_PRESENT        0x001
#define PAGE_WRITE          0x002
#define PAGE_USER           0x004
#define PAGE_WRITE_THROUGH  0x008
#define PAGE_CACHE_DISABLE  0x010
#define PAGE_ACCESSED       0x020
#define PAGE_DIRTY          0x040
#define PAGE_SIZE_4MB       0x080
#define PAGE_GLOBAL         0x100

// Define memory region boundaries (in MB)
#define KERNEL_START 0
#define KERNEL_END   1  // Kernel takes the first 1MB
#define HEAP_START   1  // Heap starts at 1MB
#define HEAP_END     5  // Heap ends at 5MB (4MB heap size)

// Memory protection state
static int memory_protection_enabled = 0;

// Memory region table (for access control)
#define MAX_MEMORY_REGIONS 16

typedef struct {
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t region_type;
    uint32_t access_flags;
} memory_region_t;

static memory_region_t memory_regions[MAX_MEMORY_REGIONS];
static int num_memory_regions = 0;

// Initialize the memory region table
static void init_memory_regions(void) {
    // Clear the memory region table
    for (int i = 0; i < MAX_MEMORY_REGIONS; i++) {
        memory_regions[i].start_addr = 0;
        memory_regions[i].end_addr = 0;
        memory_regions[i].region_type = 0;
        memory_regions[i].access_flags = 0;
    }
    
    // Add kernel region (first 1MB)
    memory_regions[0].start_addr = 0;
    memory_regions[0].end_addr = KERNEL_END * 1024 * 1024;
    memory_regions[0].region_type = MEM_REGION_KERNEL;
    memory_regions[0].access_flags = MEM_PROT_READ | MEM_PROT_WRITE | MEM_PROT_EXEC;
    // Kernel memory is not accessible with user privileges
    
    // Add heap region (1MB to 5MB)
    memory_regions[1].start_addr = HEAP_START * 1024 * 1024;
    memory_regions[1].end_addr = HEAP_END * 1024 * 1024;
    memory_regions[1].region_type = MEM_REGION_HEAP;
    memory_regions[1].access_flags = MEM_PROT_READ | MEM_PROT_WRITE;
    
    num_memory_regions = 2;
}

// Convert protection flags to page table flags
static uint32_t prot_to_page_flags(uint32_t prot_flags, int region_type) {
    uint32_t page_flags = 0;
    
    // Set present flag if any access is allowed
    if (prot_flags & (MEM_PROT_READ | MEM_PROT_WRITE | MEM_PROT_EXEC)) {
        page_flags |= PAGE_PRESENT;
    }
    
    // Set write flag if write access is allowed
    if (prot_flags & MEM_PROT_WRITE) {
        page_flags |= PAGE_WRITE;
    }
    
    // Set user flag if user access is allowed or it's a user region
    if ((prot_flags & MEM_PROT_USER) || region_type == MEM_REGION_USER) {
        page_flags |= PAGE_USER;
    }
    
    return page_flags;
}

// Add a new memory region
int add_memory_region(uint32_t start_addr, uint32_t end_addr, uint32_t region_type, uint32_t access_flags) {
    if (num_memory_regions >= MAX_MEMORY_REGIONS) {
        return -1; // No space for more regions
    }
    
    // Add the new region
    memory_regions[num_memory_regions].start_addr = start_addr;
    memory_regions[num_memory_regions].end_addr = end_addr;
    memory_regions[num_memory_regions].region_type = region_type;
    memory_regions[num_memory_regions].access_flags = access_flags;
    
    num_memory_regions++;
    return 0;
}

// Find memory region for an address
static memory_region_t* find_memory_region(uint32_t addr) {
    for (int i = 0; i < num_memory_regions; i++) {
        if (addr >= memory_regions[i].start_addr && addr < memory_regions[i].end_addr) {
            return &memory_regions[i];
        }
    }
    
    return NULL; // Address not in any defined region
}

void init_paging(void) {
    // Initialize memory regions
    init_memory_regions();
    
    // Clear page tables and directory
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;  // Not present, but writable (for when we do map it)
        page_table_kernel[i] = 0;
        page_table_heap[i] = 0;
    }

    // Map kernel memory (0-1MB) with kernel privileges (no user access)
    // Also map in the kernel code and data that was loaded at 1MB
    for (int i = 0; i < 512; i++) {  // 512 pages = 2MB (covering potentially loaded kernel)
        uint32_t phys_addr = i * PAGE_SIZE;
        // Present + Writable + Kernel-only
        page_table_kernel[i] = phys_addr | PAGE_PRESENT | PAGE_WRITE;
    }

    // Map heap memory (1MB-5MB) with kernel privileges
    for (int i = 0; i < 1024; i++) {
        uint32_t phys_addr = (HEAP_START * 1024 * 1024) + (i * PAGE_SIZE);
        // Present + Writable + Kernel-only
        page_table_heap[i] = phys_addr | PAGE_PRESENT | PAGE_WRITE;
    }

    // Point the page directory to our page tables
    page_directory[0] = ((uint32_t)page_table_kernel) | PAGE_PRESENT | PAGE_WRITE;
    page_directory[1] = ((uint32_t)page_table_heap) | PAGE_PRESENT | PAGE_WRITE;

    // Ensure directory and tables are properly aligned
    if ((uint32_t)page_directory & 0xFFF) {
        terminal_writestring("WARNING: Page directory not aligned to 4KB boundary!\n");
        // Don't continue with paging initialization if structures aren't aligned
        return;
    }
    
    if ((uint32_t)page_table_kernel & 0xFFF) {
        terminal_writestring("WARNING: Kernel page table not aligned to 4KB boundary!\n");
        return;
    }
    
    if ((uint32_t)page_table_heap & 0xFFF) {
        terminal_writestring("WARNING: Heap page table not aligned to 4KB boundary!\n");
        return;
    }

    // Load the page directory
    asm volatile("mov %0, %%cr3" : : "r"((uint32_t)page_directory));

    // Enable paging
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // Set the paging bit
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
    
    // Start with memory protection disabled for safety
    memory_protection_enabled = 0;
    
    terminal_writestring("Paging initialized with basic memory protection (disabled by default)\n");
}

// Map a physical address to a virtual address with protection flags
int map_page(uint32_t physical_addr, uint32_t virtual_addr, uint32_t flags) {
    // Find the directory and table indices
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    // Get the memory region for this address
    memory_region_t* region = find_memory_region(virtual_addr);
    if (!region) {
        return -1; // Address not in any defined region
    }
    
    // Convert protection flags to page flags
    uint32_t page_flags = prot_to_page_flags(flags, region->region_type);
    
    // Check if we need to create a new page table
    if ((page_directory[pd_index] & PAGE_PRESENT) == 0) {
        // Allocate a new page table (in a real implementation, this would use kmalloc)
        uint32_t* new_page_table = (uint32_t*)0; // Replace with actual allocation
        if (!new_page_table) {
            return -2; // Out of memory
        }
        
        // Clear the new page table
        for (int i = 0; i < 1024; i++) {
            new_page_table[i] = 0;
        }
        
        // Add the page table to the directory
        page_directory[pd_index] = ((uint32_t)new_page_table) | PAGE_PRESENT | PAGE_WRITE;
    }
    
    // Get the page table
    uint32_t pt_addr = page_directory[pd_index] & 0xFFFFF000;
    uint32_t* page_table = (uint32_t*)pt_addr;
    
    // Map the page
    page_table[pt_index] = (physical_addr & 0xFFFFF000) | page_flags;
    
    // Invalidate TLB for this address
    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    
    return 0;
}

// Unmap a virtual address
int unmap_page(uint32_t virtual_addr) {
    // Find the directory and table indices
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    // Check if the page directory entry is present
    if ((page_directory[pd_index] & PAGE_PRESENT) == 0) {
        return -1; // Page directory entry not present
    }
    
    // Get the page table
    uint32_t pt_addr = page_directory[pd_index] & 0xFFFFF000;
    uint32_t* page_table = (uint32_t*)pt_addr;
    
    // Unmap the page
    page_table[pt_index] = 0;
    
    // Invalidate TLB for this address
    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    
    return 0;
}

// Set protection flags for a virtual address
int protect_page(uint32_t virtual_addr, uint32_t flags) {
    // Find the directory and table indices
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    // Get the memory region for this address
    memory_region_t* region = find_memory_region(virtual_addr);
    if (!region) {
        return -1; // Address not in any defined region
    }
    
    // Convert protection flags to page flags
    uint32_t page_flags = prot_to_page_flags(flags, region->region_type);
    
    // Check if the page directory entry is present
    if ((page_directory[pd_index] & PAGE_PRESENT) == 0) {
        return -2; // Page directory entry not present
    }
    
    // Get the page table
    uint32_t pt_addr = page_directory[pd_index] & 0xFFFFF000;
    uint32_t* page_table = (uint32_t*)pt_addr;
    
    // Update the protection flags
    uint32_t entry = page_table[pt_index];
    if ((entry & PAGE_PRESENT) == 0) {
        return -3; // Page not present
    }
    
    // Preserve the physical address, update the flags
    page_table[pt_index] = (entry & 0xFFFFF000) | page_flags;
    
    // Invalidate TLB for this address
    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    
    return 0;
}

// Get the physical address for a virtual address
uint32_t get_physical_address(uint32_t virtual_addr) {
    // Find the directory and table indices
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    // Check if the page directory entry is present
    if ((page_directory[pd_index] & PAGE_PRESENT) == 0) {
        return 0; // Page directory entry not present
    }
    
    // Get the page table
    uint32_t pt_addr = page_directory[pd_index] & 0xFFFFF000;
    uint32_t* page_table = (uint32_t*)pt_addr;
    
    // Check if the page is present
    if ((page_table[pt_index] & PAGE_PRESENT) == 0) {
        return 0; // Page not present
    }
    
    // Get the physical address (high 20 bits of the PTE)
    uint32_t physical_addr = page_table[pt_index] & 0xFFFFF000;
    
    // Add the offset within the page (low 12 bits of the virtual address)
    physical_addr |= (virtual_addr & 0xFFF);
    
    return physical_addr;
}

// Enable memory protection
void enable_memory_protection(void) {
    // We're already using paging, but here we'd add any additional protection
    // For now, just set our flag without changing CPU state
    memory_protection_enabled = 1;
    terminal_writestring("Memory protection enabled (software validation only)\n");
    terminal_writestring("Note: No hardware protection is active\n");
}

// Disable memory protection
void disable_memory_protection(void) {
    // Just clear our flag
    memory_protection_enabled = 0;
    terminal_writestring("Memory protection disabled\n");
}

// Check if an address is valid for access with given protection flags
int is_valid_access(uint32_t virtual_addr, uint32_t access_flags) {
    if (!memory_protection_enabled) {
        return 1; // Memory protection is disabled, allow all access
    }
    
    // Special cases: allow kernel to access everything
    // This check detects if execution is in kernel code (first 1MB)
    uint32_t caller_address;
    asm volatile("call 1f; 1: pop %0" : "=r"(caller_address));
    
    if (caller_address < 0x100000) {
        // This is kernel code, allow full access
        return 1;
    }
    
    // Standard check: find the memory region for this address
    memory_region_t* region = find_memory_region(virtual_addr);
    if (!region) {
        // If address not in defined region, default to denying access
        return 0;
    }
    
    // Check if the requested access is allowed
    return (region->access_flags & access_flags) == access_flags;
}

// Memory protection fault handler - this is now only used for diagnostic purposes
// In polling mode, it will never be automatically called by an interrupt
void memory_fault_handler(uint32_t fault_addr, uint32_t error_code) {
    terminal_writestring("Memory Protection Fault!\n");
    terminal_printf("Fault address: 0x%08x\n", fault_addr);
    terminal_printf("Error code: 0x%08x\n", error_code);
    
    // Decode the error code
    terminal_writestring("Error details:\n");
    if (error_code & 0x1) {
        terminal_writestring("- Page protection violation\n");
    } else {
        terminal_writestring("- Page not present\n");
    }
    
    if (error_code & 0x2) {
        terminal_writestring("- Write operation\n");
    } else {
        terminal_writestring("- Read operation\n");
    }
    
    if (error_code & 0x4) {
        terminal_writestring("- User-mode access\n");
    } else {
        terminal_writestring("- Kernel-mode access\n");
    }
    
    // Find the memory region for this address
    memory_region_t* region = find_memory_region(fault_addr);
    if (region) {
        terminal_printf("Memory region: 0x%08x - 0x%08x (Type: %d)\n", 
            region->start_addr, region->end_addr, region->region_type);
        terminal_printf("Allowed access: 0x%08x\n", region->access_flags);
    } else {
        terminal_writestring("Address not in any defined memory region\n");
    }
    
    // In polling mode with no interrupts, we can't actually recover from faults
    // This function is just for diagnostic purposes when explicitly called
    terminal_writestring("Since we're in polling mode, this is for diagnostic only\n");
}

// Display memory region information
void display_memory_regions(void) {
    terminal_writestring("Memory Regions:\n");
    terminal_writestring("Start      End        Type Access Flags\n");
    terminal_writestring("---------- ---------- ---- -----------\n");
    
    for (int i = 0; i < num_memory_regions; i++) {
        memory_region_t* region = &memory_regions[i];
        
        // Skip unused regions
        if (region->end_addr == 0) {
            continue;
        }
        
        // Print region info
        terminal_printf("0x%08x 0x%08x %4d ", 
            region->start_addr,
            region->end_addr,
            region->region_type);
        
        // Print access flags
        terminal_writestring("[");
        if (region->access_flags & MEM_PROT_READ) {
            terminal_writestring("R");
        } else {
            terminal_writestring("-");
        }
        
        if (region->access_flags & MEM_PROT_WRITE) {
            terminal_writestring("W");
        } else {
            terminal_writestring("-");
        }
        
        if (region->access_flags & MEM_PROT_EXEC) {
            terminal_writestring("X");
        } else {
            terminal_writestring("-");
        }
        
        if (region->access_flags & MEM_PROT_USER) {
            terminal_writestring("U");
        } else {
            terminal_writestring("K");  // Kernel-only
        }
        
        terminal_writestring("]\n");
    }
}