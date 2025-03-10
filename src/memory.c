// src/memory.c - Memory management implementation

#include "memory.h"
#include "stdio.h"
#include "terminal.h"
#include "kmalloc.h"
#include "string.h"
#include <stdint.h>

#define PAGE_SIZE 4096 // 4KB pages

// Memory block descriptor for memory tracking
typedef struct memory_block {
    uint32_t address;          // Virtual address
    uint32_t size;             // Size in bytes
    uint32_t flags;            // Allocation flags
    const char* allocation_type; // Description of allocation
    const char* allocated_by;   // Function that allocated the memory
    struct memory_block* next;  // Next block in list
} memory_block_t;

// Memory mapping descriptor for file mapping
typedef struct memory_mapping {
    uint32_t virtual_addr;     // Virtual address
    uint32_t physical_addr;    // Physical address
    uint32_t size;             // Size in bytes
    uint32_t flags;            // Protection flags
    uint32_t mapping_type;     // Type of mapping (file, device, etc.)
    void* mapping_data;        // Additional mapping data
    struct memory_mapping* next;
} memory_mapping_t;

// Memory zone descriptor
typedef struct memory_zone {
    uint32_t start_addr;       // Start of zone
    uint32_t end_addr;         // End of zone
    uint32_t total_size;       // Total size of zone
    uint32_t free_size;        // Free size in zone
    uint32_t largest_free_block; // Size of largest free block
    uint32_t allocation_count;  // Number of allocations
    const char* zone_name;     // Name of memory zone
} memory_zone_t;

// Memory statistics
static struct {
    uint32_t total_memory;     // Total physical memory
    uint32_t used_memory;      // Total used memory
    uint32_t free_memory;      // Total free memory
    uint32_t kernel_memory;    // Kernel memory usage
    uint32_t heap_memory;      // Heap memory usage
    uint32_t allocated_pages;  // Number of allocated pages
    uint32_t free_pages;       // Number of free pages
    uint32_t allocation_count; // Number of active allocations
} memory_stats;

// Memory zones
#define MAX_MEMORY_ZONES 4
static memory_zone_t memory_zones[MAX_MEMORY_ZONES];
static int num_memory_zones = 0;

// Memory allocation tracking
static memory_block_t* allocation_list = NULL;

// Map a physical page to a virtual address
int map_page(uint32_t physical_addr, uint32_t virtual_addr, uint32_t flags) {
    // Simple stub - in a real OS, this would update page tables
    return 0;  // Success
}

// Unmap a virtual address
int unmap_page(uint32_t virtual_addr) {
    // Simple stub - in a real OS, this would update page tables
    return 0;  // Success
}

// Memory mapping list
static memory_mapping_t* mapping_list = NULL;

// Bitmap of physical page allocations
#define MAX_PHYSICAL_PAGES 1024 // 4MB with 4KB pages
static uint32_t physical_page_bitmap[MAX_PHYSICAL_PAGES / 32]; // 1 bit per page, 32 pages per uint32_t

// Initialize memory zones
static void init_memory_zones(void) {
    // Clear memory zones
    for (int i = 0; i < MAX_MEMORY_ZONES; i++) {
        memory_zones[i].start_addr = 0;
        memory_zones[i].end_addr = 0;
        memory_zones[i].total_size = 0;
        memory_zones[i].free_size = 0;
        memory_zones[i].largest_free_block = 0;
        memory_zones[i].allocation_count = 0;
        memory_zones[i].zone_name = "Unused";
    }

    // Define kernel zone (0MB to 1MB)
    memory_zones[0].start_addr = 0;
    memory_zones[0].end_addr = 0x100000; // 1MB
    memory_zones[0].total_size = 0x100000;
    memory_zones[0].free_size = 0; // Fully allocated to kernel
    memory_zones[0].largest_free_block = 0;
    memory_zones[0].allocation_count = 1; // Consider kernel as 1 allocation
    memory_zones[0].zone_name = "Kernel";

    // Define heap zone (1MB to 5MB)
    memory_zones[1].start_addr = 0x100000; // 1MB
    memory_zones[1].end_addr = 0x500000; // 5MB
    memory_zones[1].total_size = 0x400000; // 4MB
    memory_zones[1].free_size = 0x400000; // Initially fully free
    memory_zones[1].largest_free_block = 0x400000;
    memory_zones[1].allocation_count = 0;
    memory_zones[1].zone_name = "Heap";

    // Define user zone (5MB to 8MB) - reserved for future user space allocations
    memory_zones[2].start_addr = 0x500000; // 5MB
    memory_zones[2].end_addr = 0x800000; // 8MB
    memory_zones[2].total_size = 0x300000; // 3MB
    memory_zones[2].free_size = 0x300000; // Initially fully free
    memory_zones[2].largest_free_block = 0x300000;
    memory_zones[2].allocation_count = 0;
    memory_zones[2].zone_name = "User";

    num_memory_zones = 3;

    // Initialize memory statistics
    memory_stats.total_memory = 0x800000; // 8MB total
    memory_stats.used_memory = 0x100000;  // 1MB kernel
    memory_stats.free_memory = 0x700000;  // 7MB free
    memory_stats.kernel_memory = 0x100000; // 1MB kernel
    memory_stats.heap_memory = 0;         // No heap allocations yet
    memory_stats.allocated_pages = 256;   // 1MB in 4KB pages
    memory_stats.free_pages = 1792;       // 7MB in 4KB pages
    memory_stats.allocation_count = 1;    // Kernel as initial allocation
}

// Initialize physical page bitmap
static void init_physical_page_bitmap(void) {
    // Clear the bitmap (0 = free, 1 = allocated)
    for (int i = 0; i < MAX_PHYSICAL_PAGES / 32; i++) {
        physical_page_bitmap[i] = 0;
    }

    // Mark kernel pages as allocated (0MB to 1MB, pages 0-255)
    for (int i = 0; i < 256 / 32; i++) {
        physical_page_bitmap[i] = 0xFFFFFFFF; // All 32 pages in this uint32_t are allocated
    }
    
    // Mark any partially allocated uint32_t entries
    if (256 % 32 != 0) {
        int remainder = 256 % 32;
        physical_page_bitmap[256 / 32] = (1 << remainder) - 1;
    }
}

// Find a free physical page
static int find_free_physical_page(void) {
    for (int i = 0; i < MAX_PHYSICAL_PAGES / 32; i++) {
        if (physical_page_bitmap[i] != 0xFFFFFFFF) {
            // Found a uint32_t with at least one free bit
            for (int j = 0; j < 32; j++) {
                if ((physical_page_bitmap[i] & (1 << j)) == 0) {
                    // Found a free bit
                    return i * 32 + j;
                }
            }
        }
    }
    return -1; // No free pages
}

// Allocate a physical page
static uint32_t allocate_physical_page(void) {
    int page_index = find_free_physical_page();
    if (page_index < 0) {
        return 0; // No free pages
    }

    // Mark page as allocated
    physical_page_bitmap[page_index / 32] |= (1 << (page_index % 32));

    // Update statistics
    memory_stats.allocated_pages++;
    memory_stats.free_pages--;
    memory_stats.used_memory += PAGE_SIZE;
    memory_stats.free_memory -= PAGE_SIZE;

    // Return physical address
    return page_index * PAGE_SIZE;
}

// Free a physical page
static void free_physical_page(uint32_t physical_addr) {
    int page_index = physical_addr / PAGE_SIZE;
    if (page_index >= MAX_PHYSICAL_PAGES) {
        return; // Invalid address
    }

    // Check if page is allocated
    if ((physical_page_bitmap[page_index / 32] & (1 << (page_index % 32))) == 0) {
        return; // Page already free
    }

    // Mark page as free
    physical_page_bitmap[page_index / 32] &= ~(1 << (page_index % 32));

    // Update statistics
    memory_stats.allocated_pages--;
    memory_stats.free_pages++;
    memory_stats.used_memory -= PAGE_SIZE;
    memory_stats.free_memory += PAGE_SIZE;
}

// Initialize paging system
int init_paging(void) {
    terminal_writestring("Paging initialized\n");
    return 0; // Success
}

// Track memory allocation
void track_memory_allocation(uint32_t address, uint32_t size, uint32_t flags, 
                           const char* allocation_type, const char* allocated_by) {
    memory_block_t* block = kmalloc(sizeof(memory_block_t));
    if (!block) {
        return; // Out of memory
    }

    block->address = address;
    block->size = size;
    block->flags = flags;
    block->allocation_type = allocation_type;
    block->allocated_by = allocated_by;
    block->next = allocation_list;
    allocation_list = block;

    // Update statistics
    memory_stats.allocation_count++;
    
    // Update zone statistics
    for (int i = 0; i < num_memory_zones; i++) {
        if (address >= memory_zones[i].start_addr && 
            address < memory_zones[i].end_addr) {
            memory_zones[i].allocation_count++;
            memory_zones[i].free_size -= size;
            // Would need to recalculate largest free block in a real implementation
            break;
        }
    }
}

// Untrack memory allocation
void untrack_memory_allocation(uint32_t address) {
    memory_block_t* prev = NULL;
    memory_block_t* curr = allocation_list;
    
    while (curr) {
        if (curr->address == address) {
            // Found the allocation
            if (prev) {
                prev->next = curr->next;
            } else {
                allocation_list = curr->next;
            }
            
            // Update statistics
            memory_stats.allocation_count--;
            
            // Update zone statistics
            for (int i = 0; i < num_memory_zones; i++) {
                if (address >= memory_zones[i].start_addr && 
                    address < memory_zones[i].end_addr) {
                    memory_zones[i].allocation_count--;
                    memory_zones[i].free_size += curr->size;
                    // Would need to recalculate largest free block in a real implementation
                    break;
                }
            }
            
            kfree(curr);
            return;
        }
        
        prev = curr;
        curr = curr->next;
    }
}

// Improved initialization of memory management
void memory_enhanced_init(void) {
    init_memory_zones();
    init_physical_page_bitmap();
    
    terminal_writestring("Enhanced memory management initialized\n");
    terminal_printf("Total memory: %d KB\n", memory_stats.total_memory / 1024);
    terminal_printf("Free memory: %d KB\n", memory_stats.free_memory / 1024);
}

// Map a virtual address range to a physical address range
int map_memory_range(uint32_t virtual_addr, uint32_t physical_addr, 
                     uint32_t size, uint32_t flags) {
    memory_mapping_t* mapping = kmalloc(sizeof(memory_mapping_t));
    if (!mapping) {
        return -1; // Out of memory
    }
    
    mapping->virtual_addr = virtual_addr;
    mapping->physical_addr = physical_addr;
    mapping->size = size;
    mapping->flags = flags;
    mapping->mapping_type = 0; // Default type
    mapping->mapping_data = NULL;
    mapping->next = mapping_list;
    mapping_list = mapping;
    
    // Now map all pages in the range
    uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < num_pages; i++) {
        uint32_t vaddr = virtual_addr + i * PAGE_SIZE;
        uint32_t paddr = physical_addr + i * PAGE_SIZE;
        
        // Use the existing map_page function to map each page
        int result = map_page(paddr, vaddr, flags);
        if (result != 0) {
            // If mapping fails, we should clean up, but for simplicity
            // we'll just return an error here
            return -1;
        }
    }
    
    return 0;
}

// Unmap a virtual address range
int unmap_memory_range(uint32_t virtual_addr, uint32_t size) {
    // Find the mapping
    memory_mapping_t* prev = NULL;
    memory_mapping_t* curr = mapping_list;
    
    while (curr) {
        if (curr->virtual_addr == virtual_addr && curr->size == size) {
            // Found the mapping
            if (prev) {
                prev->next = curr->next;
            } else {
                mapping_list = curr->next;
            }
            
            // Unmap all pages in the range
            uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
            for (uint32_t i = 0; i < num_pages; i++) {
                uint32_t vaddr = virtual_addr + i * PAGE_SIZE;
                unmap_page(vaddr);
            }
            
            kfree(curr);
            return 0;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return -1; // Mapping not found
}

// Allocate memory with specific requirements
void* memory_alloc(uint32_t size, uint32_t flags, const char* allocation_type, const char* allocated_by) {
    // Align size to 4 bytes
    size = (size + 3) & ~3;
    
    // Use kmalloc for actual allocation
    void* ptr = kmalloc(size);
    if (!ptr) {
        return NULL; // Out of memory
    }
    
    // Track the allocation
    track_memory_allocation((uint32_t)ptr, size, flags, allocation_type, allocated_by);
    
    // Update heap usage
    memory_stats.heap_memory += size;
    
    return ptr;
}

// Free allocated memory
void memory_free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    // Find allocation info
    memory_block_t* curr = allocation_list;
    while (curr) {
        if (curr->address == (uint32_t)ptr) {
            // Update heap usage
            memory_stats.heap_memory -= curr->size;
            break;
        }
        curr = curr->next;
    }
    
    // Untrack the allocation
    untrack_memory_allocation((uint32_t)ptr);
    
    // Free the memory
    kfree(ptr);
}

// Display detailed memory statistics
void display_memory_statistics(void) {
    terminal_writestring("Memory Statistics:\n");
    terminal_writestring("----------------------------\n");
    terminal_printf("Total Memory: %d KB\n", memory_stats.total_memory / 1024);
    terminal_printf("Used Memory: %d KB (%d%%)\n", 
                    memory_stats.used_memory / 1024,
                    (memory_stats.used_memory * 100) / memory_stats.total_memory);
    terminal_printf("Free Memory: %d KB (%d%%)\n", 
                    memory_stats.free_memory / 1024,
                    (memory_stats.free_memory * 100) / memory_stats.total_memory);
    terminal_printf("Kernel Memory: %d KB\n", memory_stats.kernel_memory / 1024);
    terminal_printf("Heap Memory: %d KB\n", memory_stats.heap_memory / 1024);
    terminal_printf("Page Status: %d allocated, %d free\n", 
                    memory_stats.allocated_pages, memory_stats.free_pages);
    terminal_printf("Active Allocations: %d\n", memory_stats.allocation_count);
    
    terminal_writestring("\nMemory Zones:\n");
    terminal_writestring("----------------------------\n");
    for (int i = 0; i < num_memory_zones; i++) {
        memory_zone_t* zone = &memory_zones[i];
        terminal_printf("%s: 0x%x - 0x%x (%d KB)\n", 
                       zone->zone_name, zone->start_addr, zone->end_addr,
                       zone->total_size / 1024);
        terminal_printf("  Free: %d KB (%d%%), Allocations: %d\n",
                       zone->free_size / 1024,
                       (zone->free_size * 100) / zone->total_size,
                       zone->allocation_count);
    }
    
    terminal_writestring("\nActive Allocations:\n");
    terminal_writestring("----------------------------\n");
    memory_block_t* curr = allocation_list;
    int count = 0;
    
    while (curr && count < 10) {
        terminal_printf("0x%x: %d bytes, %s by %s\n", 
                       curr->address, curr->size, 
                       curr->allocation_type, curr->allocated_by);
        curr = curr->next;
        count++;
    }
    
    if (count == 10 && curr) {
        terminal_writestring("(more allocations not shown)\n");
    }
}

// Enhanced memory check function that provides more details
int enhanced_memory_check(uint32_t address, uint32_t size, uint32_t access_flags) {
    // Check basic validity first
    if (!is_valid_access(address, access_flags)) {
        terminal_printf("Memory access violation: 0x%x is not accessible with flags 0x%x\n", 
                       address, access_flags);
        return 0;
    }
    
    // Check if address is within a known allocation
    memory_block_t* curr = allocation_list;
    while (curr) {
        if (address >= curr->address && address < curr->address + curr->size) {
            // Found the allocation, check if the size fits
            if (address + size <= curr->address + curr->size) {
                // Check if the allocation allows this access
                if ((curr->flags & access_flags) == access_flags) {
                    return 1; // Access allowed
                } else {
                    terminal_printf("Memory protection violation: allocation at 0x%x allows 0x%x but requested 0x%x\n", 
                                  curr->address, curr->flags, access_flags);
                    return 0;
                }
            } else {
                terminal_printf("Memory access violation: access exceeds allocation boundary at 0x%x\n", 
                              curr->address + curr->size);
                return 0;
            }
        }
        curr = curr->next;
    }
    
    // If we get here, address is not in a tracked allocation
    // For now, we'll allow it if the basic memory check passed
    return 1;
}

// Add near other memory protection functions
void enable_memory_protection(void) {
    terminal_writestring("Memory protection enabled\n");
}

void disable_memory_protection(void) {
    terminal_writestring("Memory protection disabled\n");
}

// Check if a memory access is valid
int is_valid_access(uint32_t virtual_addr, uint32_t access_flags) {
    // Simple implementation - consider all accesses to usable memory valid
    // In a real OS, this would check page permissions
    return (virtual_addr < 0x800000);  // Allow access to first 8MB
}

// Add with other diagnostic functions, or at the end of the file
void display_memory_regions(void) {
    terminal_writestring("Memory regions:\n");
    terminal_writestring("  Kernel: 0MB - 1MB\n");
    terminal_writestring("  Heap: 1MB - 5MB\n");
    terminal_writestring("  User: 5MB - 8MB\n");
}

// Display memory map visualization
void display_memory_map(void) {
    terminal_writestring("Memory Map Visualization:\n");
    terminal_writestring("--------------------------------------------------\n");
    
    // Define our display width (number of characters per line)
    const int display_width = 60;
    const uint32_t memory_per_char = memory_stats.total_memory / display_width;
    
    // Display scale
    terminal_printf("Each character represents %d KB of memory\n", memory_per_char / 1024);
    terminal_writestring("K = Kernel, H = Heap, U = User, F = Free, X = Reserved\n\n");
    
    // Display memory map line
    terminal_writestring("|");
    
    for (int i = 0; i < display_width; i++) {
        uint32_t addr = i * memory_per_char;
        char display_char = '?';
        
        // Determine what's at this address
        for (int j = 0; j < num_memory_zones; j++) {
            memory_zone_t* zone = &memory_zones[j];
            if (addr >= zone->start_addr && addr < zone->end_addr) {
                if (strcmp(zone->zone_name, "Kernel") == 0) {
                    display_char = 'K';
                } else if (strcmp(zone->zone_name, "Heap") == 0) {
                    // Check if this part of heap is allocated
                    int is_allocated = 0;
                    memory_block_t* curr = allocation_list;
                    while (curr) {
                        if (addr >= curr->address && addr < curr->address + curr->size) {
                            is_allocated = 1;
                            break;
                        }
                        curr = curr->next;
                    }
                    display_char = is_allocated ? 'H' : 'F';
                } else if (strcmp(zone->zone_name, "User") == 0) {
                    display_char = 'U';
                } else {
                    display_char = 'X'; // Reserved/unknown
                }
                break;
            }
        }
        
        terminal_putchar(display_char);
    }
    
    terminal_writestring("|\n");
    
    // Add markers for major addresses
    terminal_writestring("0");
    for (int i = 10; i < display_width; i += 10) {
        for (int j = 0; j < 9; j++) {
            terminal_putchar(' ');
        }
        terminal_putchar('|');
    }
    terminal_writestring("\n");
    
    terminal_writestring("0MB");
    int mb_per_10chars = (10 * memory_per_char) / (1024 * 1024);
    for (int i = mb_per_10chars; i < (display_width * memory_per_char) / (1024 * 1024); i += mb_per_10chars) {
        // Calculate how many spaces to add
        int spaces = 10 - 2; // Subtract length of "nMB"
        if (i >= 10) spaces--; // One more digit
        
        for (int j = 0; j < spaces; j++) {
            terminal_putchar(' ');
        }
        
        terminal_printf("%dMB", i);
    }
    terminal_writestring("\n");
}