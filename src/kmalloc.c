#include "kmalloc.h"
#include <stdint.h>
#include <stdatomic.h>

// Structure for a free memory block
typedef struct FreeBlock {
    size_t size;
    struct FreeBlock* next;
} FreeBlock;

// Global free list and synchronization
static FreeBlock* free_list = NULL;
static atomic_flag lock = ATOMIC_FLAG_INIT;

// Custom memset implementation for freestanding environment
static void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

// Spinlock functions
static void spin_lock(void) {
    while (atomic_flag_test_and_set(&lock)) {}
}

static void spin_unlock(void) {
    atomic_flag_clear(&lock);
}

// Initialize the memory allocator
void init_memory(void) {
    free_list = (FreeBlock*)KMALLOC_HEAP_START;
    free_list->size = KMALLOC_HEAP_SIZE;
    free_list->next = NULL;
}

void* kmalloc(size_t size) {
    return kmalloc_flags(size, KMALLOC_NORMAL);
}

void* kmalloc_flags(size_t size, int flags) {
    spin_lock();
    size_t total_size = (size + sizeof(FreeBlock) + 7) & ~7;
    FreeBlock* current = free_list;
    FreeBlock* previous = NULL;

    while (current != NULL) {
        if (current->size >= total_size) {
            if (current->size >= total_size + sizeof(FreeBlock)) {
                FreeBlock* new_block = (FreeBlock*)((uint8_t*)current + total_size);
                new_block->size = current->size - total_size;
                new_block->next = current->next;
                if (previous) {
                    previous->next = new_block;
                } else {
                    free_list = new_block;
                }
                current->size = total_size;
            } else {
                if (previous) {
                    previous->next = current->next;
                } else {
                    free_list = current->next;
                }
            }
            void* result = (void*)((uint8_t*)current + sizeof(FreeBlock));
            if (result && (flags & KMALLOC_ZERO)) {
                memset(result, 0, size);
            }
            spin_unlock();
            return result;
        }
        previous = current;
        current = current->next;
    }
    spin_unlock();
    return NULL;
}

void* kmalloc_aligned(size_t size, size_t align) {
    spin_lock();
    if (align < sizeof(FreeBlock)) align = sizeof(FreeBlock);
    size_t total_size = (size + sizeof(FreeBlock) + align - 1) & ~(align - 1);
    FreeBlock* current = free_list;
    FreeBlock* previous = NULL;

    while (current != NULL) {
        uintptr_t raw_addr = (uintptr_t)current + sizeof(FreeBlock);
        uintptr_t aligned_addr = (raw_addr + align - 1) & ~(align - 1);
        size_t padding = aligned_addr - raw_addr;
        size_t required_size = total_size + padding;

        if (current->size >= required_size) {
            if (current->size >= required_size + sizeof(FreeBlock)) {
                FreeBlock* new_block = (FreeBlock*)((uint8_t*)current + required_size);
                new_block->size = current->size - required_size;
                new_block->next = current->next;
                if (previous) {
                    previous->next = new_block;
                } else {
                    free_list = new_block;
                }
                current->size = required_size;
            } else {
                if (previous) {
                    previous->next = current->next;
                } else {
                    free_list = current->next;
                }
            }
            spin_unlock();
            return (void*)aligned_addr;
        }
        previous = current;
        current = current->next;
    }
    spin_unlock();
    return NULL;
}

void kfree(void* ptr) {
    if (ptr == NULL) return;

    spin_lock();
    FreeBlock* block = (FreeBlock*)((uint8_t*)ptr - sizeof(FreeBlock));
    if ((uint8_t*)block < (uint8_t*)KMALLOC_HEAP_START || 
        (uint8_t*)block >= (uint8_t*)KMALLOC_HEAP_START + KMALLOC_HEAP_SIZE) {
        spin_unlock();
        return;
    }

    FreeBlock* current = free_list;
    FreeBlock* previous = NULL;
    while (current != NULL && current < block) {
        previous = current;
        current = current->next;
    }

    if (previous) {
        previous->next = block;
    } else {
        free_list = block;
    }
    block->next = current;

    if (current && (uint8_t*)block + block->size == (uint8_t*)current) {
        block->size += current->size;
        block->next = current->next;
    }
    if (previous && (uint8_t*)previous + previous->size == (uint8_t*)block) {
        previous->size += block->size;
        previous->next = block->next;
    }
    spin_unlock();
}

void kmalloc_stats(KmallocStats* stats) {
    if (!stats) return;

    spin_lock();
    stats->total_size = KMALLOC_HEAP_SIZE;
    stats->free_size = 0;
    stats->allocated_size = 0;
    stats->block_count = 0;

    FreeBlock* current = free_list;
    while (current != NULL) {
        stats->free_size += current->size - sizeof(FreeBlock);
        stats->block_count++;
        current = current->next;
    }
    stats->allocated_size = KMALLOC_HEAP_SIZE - stats->free_size;
    spin_unlock();
}