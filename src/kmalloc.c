#include "kmalloc.h"
#include "memory.h"

#define HEAP_START 0x100000  // 1MB (assuming kernel code is below this)
#define HEAP_SIZE 0x400000   // 4MB heap
// Forward declarations for debugging functions
// void SERIAL_DEBUG(const char* message);
// void fb_print_hex(uint32_t value);
typedef struct block_header {
    size_t size;              // Size of this block
    int is_free;              // 1 if block is free, 0 if used
    struct block_header* next; // Next block in list
} block_header_t;

static block_header_t* heap_start = NULL;
static void* heap_end = (void*)(HEAP_START + HEAP_SIZE);

/* Initialize the heap */
int kmalloc_init(void) {
    heap_start = (block_header_t*)HEAP_START;
    heap_start->size = HEAP_SIZE - sizeof(block_header_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
    return 0;  // Return 0 to indicate successful initialization
}

/* Memory allocation function with debugging */
void* kmalloc(size_t size) {
    // SERIAL_DEBUG("kmalloc: Requesting ");
    // fb_print_hex(size);
    // SERIAL_DEBUG(" bytes\n");

    // Align size to 4 bytes
    size = (size + 3) & ~3;
    
    block_header_t* current = heap_start;
    
    while (current) {
        // Found a free block of sufficient size
        if (current->is_free && current->size >= size) {
            // Check if we should split the block
            if (current->size > size + sizeof(block_header_t) + 4) {
                block_header_t* new_block = (block_header_t*)((char*)current + sizeof(block_header_t) + size);
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            void* result = (void*)((char*)current + sizeof(block_header_t));
            // SERIAL_DEBUG("kmalloc: Returning ");
            // fb_print_hex((unsigned int)result);
            // SERIAL_DEBUG("\n");
            return result;
        }
        
        current = current->next;
    }
    
    // // Out of memory
    // SERIAL_DEBUG("kmalloc: Out of memory\n");
    return NULL;
}

/* Free allocated memory */
void kfree(void* ptr) {
    if (!ptr)
        return;
    
    // Find the block header
    block_header_t* header = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    header->is_free = 1;
    
    // Coalesce with next block if free
    if (header->next && header->next->is_free) {
        header->size += sizeof(block_header_t) + header->next->size;
        header->next = header->next->next;
    }
    
    // Coalesce with previous block if free
    block_header_t* current = heap_start;
    while (current && current->next != header) {
        current = current->next;
    }
    
    if (current && current->is_free) {
        current->size += sizeof(block_header_t) + header->size;
        current->next = header->next;
    }
}

/* Get heap statistics */
void kmalloc_stats(size_t* total, size_t* used, size_t* free) {
    *total = HEAP_SIZE;
    *used = 0;
    *free = 0;
    
    block_header_t* current = heap_start;
    while (current) {
        if (current->is_free) {
            *free += current->size;
        } else {
            *used += current->size;
        }
        current = current->next;
    }
}