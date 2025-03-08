// include/fs.h
#ifndef FS_H
#define FS_H

#include <stddef.h>

#define FS_MAX_FILES 32
#define FS_MAX_FILENAME 32
#define FS_MAX_FILESIZE 4096

typedef struct {
    char filename[FS_MAX_FILENAME];
    char data[FS_MAX_FILESIZE];
    size_t size;
    int in_use;
} file_t;

// Initialize file system
void fs_init(void);

// Create a new file
int fs_create(const char* filename);

// Delete a file
int fs_delete(const char* filename);

// Write data to a file
int fs_write(const char* filename, const char* data, size_t size);

// Read from a file
int fs_read(const char* filename, char* buffer, size_t size);

// List all files
void fs_list(void);

// Get file size
int fs_size(const char* filename);

#endif