// include/fs.h
#ifndef FS_H
#define FS_H

#include <stddef.h>

#define FS_MAX_FILES 64         // Increased max files
#define FS_MAX_FILENAME 32
#define FS_MAX_PATH 128         // Added max path length
#define FS_MAX_FILESIZE 8192    // Increased max file size

// File types
#define FS_TYPE_FILE 1
#define FS_TYPE_DIRECTORY 2

typedef struct {
    char name[FS_MAX_FILENAME];  // Name (not full path)
    char path[FS_MAX_PATH];      // Full path
    int type;                    // File or directory
    char data[FS_MAX_FILESIZE];  // File data (only for files)
    size_t size;                 // Size (for files)
    int parent_index;            // Index of parent directory (-1 for root)
    int in_use;                  // Whether this entry is in use
    unsigned int permissions;    // Basic permissions (owner, group, world)
    unsigned int created_time;   // Creation timestamp
    unsigned int modified_time;  // Last modification timestamp
} fs_node_t;

// Initialize file system
int fs_init(void);

// Create a new file
int fs_create(const char* path, int type);

// Delete a file or directory
int fs_delete(const char* path);

// Write data to a file
int fs_write(const char* path, const char* data, size_t size);

// Read from a file
int fs_read(const char* path, char* buffer, size_t size);

// List all files in a directory
void fs_list(const char* path);

// Get file size
int fs_size(const char* path);

// Create a directory
int fs_mkdir(const char* path);

// Change current directory
int fs_chdir(const char* path);

// Get current directory
const char* fs_getcwd(void);

// Get info about a file or directory
int fs_stat(const char* path, fs_node_t* info);

#endif