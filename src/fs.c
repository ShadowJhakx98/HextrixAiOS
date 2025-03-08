// src/fs_enhanced.c
#include "fs.h"
#include "hal.h"
#include "kmalloc.h"
#include "terminal.h"
#include "stdio.h"
#include "string.h"

// File system buffer cache
#define FS_CACHE_SIZE 32      // Number of cache blocks
#define FS_CACHE_BLOCK_SIZE 512 // Size of each cache block (same as sector size)

// Cache block states
#define CACHE_STATE_EMPTY     0
#define CACHE_STATE_CLEAN     1
#define CACHE_STATE_DIRTY     2

// Cache block
typedef struct {
    uint32_t sector;          // Disk sector this block represents
    uint8_t state;            // Block state (empty, clean, dirty)
    uint32_t last_access;     // Timestamp of last access
    uint32_t access_count;    // Number of times this block was accessed
    uint8_t data[FS_CACHE_BLOCK_SIZE]; // Data buffer
} cache_block_t;

// File system statistics
static struct {
    uint32_t cache_hits;      // Number of cache hits
    uint32_t cache_misses;    // Number of cache misses
    uint32_t cache_flushes;   // Number of cache flushes
    uint32_t file_opens;      // Number of file opens
    uint32_t file_closes;     // Number of file closes
    uint32_t file_reads;      // Number of file reads
    uint32_t file_writes;     // Number of file writes
    uint32_t dir_operations;  // Number of directory operations
} fs_stats;

// File system cache
static cache_block_t fs_cache[FS_CACHE_SIZE];
static uint32_t fs_cache_timer = 0;  // Simple timer for LRU

// Forward declarations
static int fs_flush_cache_block(int block_index);
static int fs_flush_all_cache(void);

// Initialize the enhanced file system
void fs_enhanced_init(void) {
    terminal_writestring("Initializing enhanced file system\n");
    
    // Initialize in-memory file system (existing code)
    fs_init();
    
    // Initialize cache
    for (int i = 0; i < FS_CACHE_SIZE; i++) {
        fs_cache[i].sector = 0;
        fs_cache[i].state = CACHE_STATE_EMPTY;
        fs_cache[i].last_access = 0;
        fs_cache[i].access_count = 0;
    }
    
    // Initialize statistics
    fs_stats.cache_hits = 0;
    fs_stats.cache_misses = 0;
    fs_stats.cache_flushes = 0;
    fs_stats.file_opens = 0;
    fs_stats.file_closes = 0;
    fs_stats.file_reads = 0;
    fs_stats.file_writes = 0;
    fs_stats.dir_operations = 0;
    
    terminal_writestring("Enhanced file system initialized with buffer cache\n");
}

// Find a cache block for the given sector
static int fs_find_cache_block(uint32_t sector) {
    for (int i = 0; i < FS_CACHE_SIZE; i++) {
        if (fs_cache[i].state != CACHE_STATE_EMPTY && fs_cache[i].sector == sector) {
            return i;
        }
    }
    return -1;  // Not found
}

// Find the least recently used cache block
static int fs_find_lru_cache_block(void) {
    int lru_index = 0;
    uint32_t lru_time = fs_cache[0].last_access;
    
    for (int i = 1; i < FS_CACHE_SIZE; i++) {
        if (fs_cache[i].state == CACHE_STATE_EMPTY) {
            return i;  // Empty block has highest priority
        }
        
        if (fs_cache[i].last_access < lru_time) {
            lru_index = i;
            lru_time = fs_cache[i].last_access;
        }
    }
    
    return lru_index;
}

// Flush a cache block to disk
static int fs_flush_cache_block(int block_index) {
    if (block_index < 0 || block_index >= FS_CACHE_SIZE) {
        return -1;
    }
    
    cache_block_t* block = &fs_cache[block_index];
    
    // Only flush dirty blocks
    if (block->state == CACHE_STATE_DIRTY) {
        // Write to disk
        if (hal_storage_write_sector(block->sector, block->data) != 0) {
            return -1;
        }
        
        // Mark as clean
        block->state = CACHE_STATE_CLEAN;
        fs_stats.cache_flushes++;
    }
    
    return 0;
}

// Flush all dirty cache blocks
static int fs_flush_all_cache(void) {
    int result = 0;
    
    for (int i = 0; i < FS_CACHE_SIZE; i++) {
        if (fs_cache[i].state == CACHE_STATE_DIRTY) {
            if (fs_flush_cache_block(i) != 0) {
                result = -1;  // But continue flushing other blocks
            }
        }
    }
    
    return result;
}

// Read a sector through cache
static int fs_cached_read_sector(uint32_t sector, void* buffer) {
    // Increment cache timer
    fs_cache_timer++;
    
    // Check if sector is in cache
    int block_index = fs_find_cache_block(sector);
    
    if (block_index >= 0) {
        // Cache hit
        fs_stats.cache_hits++;
        
        // Update access time and count
        fs_cache[block_index].last_access = fs_cache_timer;
        fs_cache[block_index].access_count++;
        
        // Copy data to buffer
        memcpy(buffer, fs_cache[block_index].data, FS_CACHE_BLOCK_SIZE);
        
        return 0;
    }
    
    // Cache miss
    fs_stats.cache_misses++;
    
    // Find a cache block to use
    block_index = fs_find_lru_cache_block();
    
    // Flush block if dirty
    if (fs_cache[block_index].state == CACHE_STATE_DIRTY) {
        fs_flush_cache_block(block_index);
    }
    
    // Read sector from disk
    if (hal_storage_read_sector(sector, fs_cache[block_index].data) != 0) {
        return -1;
    }
    
    // Update cache block
    fs_cache[block_index].sector = sector;
    fs_cache[block_index].state = CACHE_STATE_CLEAN;
    fs_cache[block_index].last_access = fs_cache_timer;
    fs_cache[block_index].access_count = 1;
    
    // Copy data to buffer
    memcpy(buffer, fs_cache[block_index].data, FS_CACHE_BLOCK_SIZE);
    
    return 0;
}

// Write a sector through cache
static int fs_cached_write_sector(uint32_t sector, const void* buffer) {
    // Increment cache timer
    fs_cache_timer++;
    
    // Check if sector is in cache
    int block_index = fs_find_cache_block(sector);
    
    if (block_index >= 0) {
        // Cache hit
        fs_stats.cache_hits++;
    } else {
        // Cache miss
        fs_stats.cache_misses++;
        
        // Find a cache block to use
        block_index = fs_find_lru_cache_block();
        
        // Flush block if dirty
        if (fs_cache[block_index].state == CACHE_STATE_DIRTY) {
            fs_flush_cache_block(block_index);
        }
        
        // If replacing a valid block, update it
        fs_cache[block_index].sector = sector;
    }
    
    // Update cache block
    memcpy(fs_cache[block_index].data, buffer, FS_CACHE_BLOCK_SIZE);
    fs_cache[block_index].state = CACHE_STATE_DIRTY;
    fs_cache[block_index].last_access = fs_cache_timer;
    fs_cache[block_index].access_count++;
    
    return 0;
}

// Synchronize cache with disk (flush all dirty blocks)
int fs_sync(void) {
    return fs_flush_all_cache();
}

// Enhanced file system operations that use the cache

// Read from a file with caching
int fs_enhanced_read(const char* path, void* buffer, size_t size) {
    // Use existing fs_read for now, but increment stats
    fs_stats.file_reads++;
    return fs_read(path, buffer, size);
}

// Write to a file with caching
int fs_enhanced_write(const char* path, const void* data, size_t size) {
    // Use existing fs_write for now, but increment stats
    fs_stats.file_writes++;
    return fs_write(path, data, size);
}

// Create a file with caching
int fs_enhanced_create(const char* path, int type) {
    // Use existing fs_create for now, but increment stats
    fs_stats.file_opens++;
    return fs_create(path, type);
}

// Delete a file with caching
int fs_enhanced_delete(const char* path) {
    // Use existing fs_delete for now, but increment stats
    fs_stats.file_closes++;
    return fs_delete(path);
}

// Create a directory with caching
int fs_enhanced_mkdir(const char* path) {
    // Use existing fs_mkdir for now, but increment stats
    fs_stats.dir_operations++;
    return fs_mkdir(path);
}

// Get file system statistics
void fs_get_stats(uint32_t* hits, uint32_t* misses, uint32_t* flushes,
                 uint32_t* opens, uint32_t* closes, uint32_t* reads,
                 uint32_t* writes, uint32_t* dirs) {
    if (hits) *hits = fs_stats.cache_hits;
    if (misses) *misses = fs_stats.cache_misses;
    if (flushes) *flushes = fs_stats.cache_flushes;
    if (opens) *opens = fs_stats.file_opens;
    if (closes) *closes = fs_stats.file_closes;
    if (reads) *reads = fs_stats.file_reads;
    if (writes) *writes = fs_stats.file_writes;
    if (dirs) *dirs = fs_stats.dir_operations;
}

// Display file system cache information
void fs_display_cache_info(void) {
    terminal_writestring("File System Cache Information:\n");
    terminal_writestring("----------------------------\n");
    
    // Display cache statistics
    terminal_printf("Cache size: %d blocks of %d bytes\n", 
                  FS_CACHE_SIZE, FS_CACHE_BLOCK_SIZE);
    terminal_printf("Cache hits: %d, misses: %d (%.1f%% hit rate)\n", 
                  fs_stats.cache_hits, fs_stats.cache_misses,
                  (fs_stats.cache_hits + fs_stats.cache_misses > 0) ?
                  (float)fs_stats.cache_hits * 100.0f / (fs_stats.cache_hits + fs_stats.cache_misses) : 0.0f);
    terminal_printf("Cache flushes: %d\n", fs_stats.cache_flushes);
    
    // Count cache blocks by state
    int empty_blocks = 0;
    int clean_blocks = 0;
    int dirty_blocks = 0;
    
    for (int i = 0; i < FS_CACHE_SIZE; i++) {
        switch (fs_cache[i].state) {
            case CACHE_STATE_EMPTY: empty_blocks++; break;
            case CACHE_STATE_CLEAN: clean_blocks++; break;
            case CACHE_STATE_DIRTY: dirty_blocks++; break;
        }
    }
    
    terminal_printf("Cache blocks: %d empty, %d clean, %d dirty\n",
                  empty_blocks, clean_blocks, dirty_blocks);
    
    // Display file operation statistics
    terminal_printf("File operations: %d opens, %d closes, %d reads, %d writes\n",
                  fs_stats.file_opens, fs_stats.file_closes,
                  fs_stats.file_reads, fs_stats.file_writes);
    terminal_printf("Directory operations: %d\n", fs_stats.dir_operations);
    
    // Display most active cache blocks
    terminal_writestring("\nMost Active Cache Blocks:\n");
    terminal_writestring("Block  Sector   State   Accesses\n");
    terminal_writestring("----- -------- -------- --------\n");
    
    // Find top 5 most accessed blocks
    for (int top = 0; top < 5; top++) {
        int max_index = -1;
        uint32_t max_count = 0;
        
        for (int i = 0; i < FS_CACHE_SIZE; i++) {
            if (fs_cache[i].state != CACHE_STATE_EMPTY && 
                fs_cache[i].access_count > max_count) {
                max_index = i;
                max_count = fs_cache[i].access_count;
            }
        }
        
        if (max_index >= 0) {
            const char* state_str;
            switch (fs_cache[max_index].state) {
                case CACHE_STATE_CLEAN: state_str = "Clean"; break;
                case CACHE_STATE_DIRTY: state_str = "Dirty"; break;
                default: state_str = "Unknown"; break;
            }
            
            terminal_printf("%5d %8d %8s %8d\n",
                          max_index,
                          fs_cache[max_index].sector,
                          state_str,
                          fs_cache[max_index].access_count);
            
            // Mark as processed by setting count to 0
            fs_cache[max_index].access_count = 0;
        }
    }
}

// File system consistency check
int fs_check(void) {
    terminal_writestring("Performing file system consistency check...\n");
    
    // Flush all cache to ensure disk is consistent
    fs_flush_all_cache();
    
    // For now, we'll just perform a simple check of the in-memory file system
    int errors = 0;
    
    // Check that every file/directory with a parent has a valid parent
    for (int i = 0; i < FS_MAX_FILES; i++) {
        fs_node_t node;
        if (fs_stat_by_index(i, &node) == 0 && node.in_use) {
            if (node.parent_index >= 0) {
                fs_node_t parent;
                if (fs_stat_by_index(node.parent_index, &parent) != 0 || 
                    !parent.in_use || 
                    parent.type != FS_TYPE_DIRECTORY) {
                    terminal_printf("Error: %s has invalid parent\n", node.path);
                    errors++;
                }
            }
        }
    }
    
    // Check that every directory contains . and .. entries
    for (int i = 0; i < FS_MAX_FILES; i++) {
        fs_node_t node;
        if (fs_stat_by_index(i, &node) == 0 && 
            node.in_use && 
            node.type == FS_TYPE_DIRECTORY) {
            
            int has_dot = 0;
            int has_dotdot = 0;
            
            // Check all files to see if they have this directory as parent
            for (int j = 0; j < FS_MAX_FILES; j++) {
                fs_node_t child;
                if (fs_stat_by_index(j, &child) == 0 && 
                    child.in_use && 
                    child.parent_index == i) {
                    
                    if (strcmp(child.name, ".") == 0) {
                        has_dot = 1;
                    } else if (strcmp(child.name, "..") == 0) {
                        has_dotdot = 1;
                    }
                }
            }
            
            if (!has_dot) {
                terminal_printf("Error: Directory %s missing . entry\n", node.path);
                errors++;
            }
            
            if (!has_dotdot) {
                terminal_printf("Error: Directory %s missing .. entry\n", node.path);
                errors++;
            }
        }
    }
    
    if (errors == 0) {
        terminal_writestring("File system check completed: No errors found\n");
    } else {
        terminal_printf("File system check completed: %d errors found\n", errors);
    }
    
    return errors;
}

// Enhanced file system repair
int fs_repair(void) {
    terminal_writestring("Repairing file system...\n");
    
    // Flush all cache
    fs_flush_all_cache();
    
    // For this simplified version, we'll just implement basic repairs
    int repairs = 0;
    
    // 1. Fix missing . and .. entries in directories
    for (int i = 0; i < FS_MAX_FILES; i++) {
        fs_node_t node;
        if (fs_stat_by_index(i, &node) == 0 && 
            node.in_use && 
            node.type == FS_TYPE_DIRECTORY) {
            
            int has_dot = 0;
            int has_dotdot = 0;
            int dot_index = -1;
            int dotdot_index = -1;
            
            // Check all files to see if they have this directory as parent
            for (int j = 0; j < FS_MAX_FILES; j++) {
                fs_node_t child;
                if (fs_stat_by_index(j, &child) == 0 && 
                    child.in_use && 
                    child.parent_index == i) {
                    
                    if (strcmp(child.name, ".") == 0) {
                        has_dot = 1;
                        dot_index = j;
                    } else if (strcmp(child.name, "..") == 0) {
                        has_dotdot = 1;
                        dotdot_index = j;
                    }
                }
            }
            
            // Fix missing . entry
            if (!has_dot) {
                // Find a free node
                int free_idx = -1;
                for (int j = 0; j < FS_MAX_FILES; j++) {
                    fs_node_t temp;
                    if (fs_stat_by_index(j, &temp) == 0 && !temp.in_use) {
                        free_idx = j;
                        break;
                    }
                }
                
                if (free_idx >= 0) {
                    // Create . entry
                    fs_node_t dot;
                    memset(&dot, 0, sizeof(dot));
                    strcpy(dot.name, ".");
                    sprintf(dot.path, "%s/.", node.path);
                    dot.type = FS_TYPE_DIRECTORY;
                    dot.parent_index = i;
                    dot.in_use = 1;
                    dot.permissions = node.permissions;
                    dot.created_time = node.created_time;
                    dot.modified_time = node.modified_time;
                    
                    // Write node
                    fs_update_node(free_idx, &dot);
                    repairs++;
                    
                    terminal_printf("Repaired: Created missing . entry for %s\n", node.path);
                }
            }
            
            // Fix missing .. entry
            if (!has_dotdot) {
                // Find a free node
                int free_idx = -1;
                for (int j = 0; j < FS_MAX_FILES; j++) {
                    fs_node_t temp;
                    if (fs_stat_by_index(j, &temp) == 0 && !temp.in_use) {
                        free_idx = j;
                        break;
                    }
                }
                
                if (free_idx >= 0) {
                    // Create .. entry
                    fs_node_t dotdot;
                    memset(&dotdot, 0, sizeof(dotdot));
                    strcpy(dotdot.name, "..");
                    
                    // Parent of .. is parent of directory (or self for root)
                    int parent_idx = (node.parent_index >= 0) ? node.parent_index : i;
                    
                    // Get parent path
                    fs_node_t parent;
                    if (fs_stat_by_index(parent_idx, &parent) == 0) {
                        sprintf(dotdot.path, "%s/..", node.path);
                        dotdot.type = FS_TYPE_DIRECTORY;
                        dotdot.parent_index = i;  // Parent of .. is the directory itself
                        dotdot.in_use = 1;
                        dotdot.permissions = node.permissions;
                        dotdot.created_time = node.created_time;
                        dotdot.modified_time = node.modified_time;
                        
                        // Write node
                        fs_update_node(free_idx, &dotdot);
                        repairs++;
                        
                        terminal_printf("Repaired: Created missing .. entry for %s\n", node.path);
                    }
                }
            }
        }
    }
    
    if (repairs == 0) {
        terminal_writestring("No repairs needed\n");
    } else {
        terminal_printf("File system repair completed: Made %d repairs\n", repairs);
    }
    
    return repairs;
}

// Function to get a file node by index (adding to fs.h for use here)
int fs_stat_by_index(int index, fs_node_t* info) {
    // Simple implementation assuming the in-memory FS has an array of nodes
    extern fs_node_t fs_nodes[FS_MAX_FILES];
    
    if (index < 0 || index >= FS_MAX_FILES) {
        return -1;
    }
    
    // Copy node info
    memcpy(info, &fs_nodes[index], sizeof(fs_node_t));
    return 0;
}

// Function to update a file node by index (adding to fs.h for use here)
int fs_update_node(int index, fs_node_t* info) {
    // Simple implementation assuming the in-memory FS has an array of nodes
    extern fs_node_t fs_nodes[FS_MAX_FILES];
    
    if (index < 0 || index >= FS_MAX_FILES) {
        return -1;
    }
    
    // Update node info
    memcpy(&fs_nodes[index], info, sizeof(fs_node_t));
    return 0;
}
// File system node array
fs_node_t fs_nodes[FS_MAX_FILES];

// Current working directory
static char current_directory[FS_MAX_PATH] = "/";

// Initialize file system
void fs_init(void) {
    // Clear all nodes
    for (int i = 0; i < FS_MAX_FILES; i++) {
        fs_nodes[i].in_use = 0;
    }
    
    // Create root directory
    fs_nodes[0].in_use = 1;
    fs_nodes[0].type = FS_TYPE_DIRECTORY;
    strcpy(fs_nodes[0].name, "/");
    strcpy(fs_nodes[0].path, "/");
    fs_nodes[0].parent_index = -1;
    
    terminal_writestring("File system initialized\n");
}

// Get current working directory
const char* fs_getcwd(void) {
    return current_directory;
}

// List files in a directory
void fs_list(const char* path) {
    terminal_writestring("Directory listing (stub):\n");
    terminal_writestring("  .  (directory)\n");
    terminal_writestring("  .. (directory)\n");
}

// Create a file
int fs_create(const char* path, int type) {
    // Simple stub - in a real FS, this would create a file
    return 0;  // Success
}

// Read from a file
int fs_read(const char* path, char* buffer, size_t size) {
    // Simple stub - in a real FS, this would read file data
    strcpy(buffer, "File contents (stub)");
    return strlen(buffer);
}

// Write to a file
int fs_write(const char* path, const char* data, size_t size) {
    // Simple stub - in a real FS, this would write data to file
    return size;  // Success, wrote all bytes
}

// Delete a file
int fs_delete(const char* path) {
    // Simple stub - in a real FS, this would delete a file
    return 0;  // Success
}

// Get file size
int fs_size(const char* path) {
    // Simple stub - in a real FS, this would return actual size
    return 100;  // Return dummy size
}

// Create a directory
int fs_mkdir(const char* path) {
    // Simple stub - in a real FS, this would create a directory
    return 0;  // Success
}

// Change current directory
int fs_chdir(const char* path) {
    // Simple stub - in a real FS, this would change directory
    strcpy(current_directory, path);
    return 0;  // Success
}

// Get file information by index
int fs_stat_by_index(int index, fs_node_t* info) {
    if (index < 0 || index >= FS_MAX_FILES) {
        return -1;
    }
    
    *info = fs_nodes[index];
    return 0;
}

// Update file node
int fs_update_node(int index, fs_node_t* info) {
    if (index < 0 || index >= FS_MAX_FILES) {
        return -1;
    }
    
    fs_nodes[index] = *info;
    return 0;
}