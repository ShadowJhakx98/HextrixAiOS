// include/kmalloc.h
#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>

// Initialize memory allocator
int kmalloc_init(void);

// Allocate memory
void* kmalloc(size_t size);

// Free allocated memory
void kfree(void* ptr);

// Get total memory stats
void kmalloc_stats(size_t* total, size_t* used, size_t* free);

#endif