#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>  // For size_t

// Heap configuration
#define KMALLOC_HEAP_START 0x200000  // Starting address of the heap
#define KMALLOC_HEAP_SIZE  0x100000  // 1MB heap size

// Allocation flags
#define KMALLOC_NORMAL 0x0  // Default allocation
#define KMALLOC_ZERO   0x1  // Zero-initialize the allocated memory

// Error codes
#define KMALLOC_ENOMEM 1  // Out of memory error

/**
 * @brief Structure to hold memory allocator statistics.
 */
typedef struct {
    size_t total_size;    // Total heap size
    size_t free_size;     // Total free memory
    size_t allocated_size;// Total allocated memory
    size_t block_count;   // Number of free blocks
} KmallocStats;

/**
 * @brief Initialize the kernel memory allocator.
 * 
 * This function sets up the memory allocator by initializing the free list
 * with the initial heap memory region.
 */
void init_memory(void);

/**
 * @brief Allocate a block of memory of the specified size.
 * 
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if allocation fails.
 */
void* kmalloc(size_t size);

/**
 * @brief Allocate a block of memory with specified flags.
 * 
 * @param size The size of the memory block to allocate.
 * @param flags Allocation options (e.g., KMALLOC_ZERO).
 * @return A pointer to the allocated memory block, or NULL if allocation fails.
 */
void* kmalloc_flags(size_t size, int flags);

/**
 * @brief Allocate an aligned block of memory.
 * 
 * @param size The size of the memory block to allocate.
 * @param align The alignment boundary (e.g., 16, 4096).
 * @return A pointer to the allocated memory block, or NULL if allocation fails.
 */
void* kmalloc_aligned(size_t size, size_t align);

/**
 * @brief Free a previously allocated memory block.
 * 
 * @param ptr Pointer to the memory block to free.
 */
void kfree(void* ptr);

/**
 * @brief Retrieve memory allocator statistics.
 * 
 * @param stats Pointer to a KmallocStats structure to fill.
 */
void kmalloc_stats(KmallocStats* stats);

#endif // KMALLOC_H