// src/fs.c
#include "fs.h"
#include "terminal.h"
#include "string.h"
#include "stdio.h"

// Our simple in-memory file system
static file_t files[FS_MAX_FILES];

void fs_init(void) {
    // Initialize all files as unused
    for (int i = 0; i < FS_MAX_FILES; i++) {
        files[i].in_use = 0;
        files[i].size = 0;
    }
    
    // Create a README file
    fs_create("README.txt");
    fs_write("README.txt", "Welcome to Hextrix OS v0.3.1!\nType 'help' for available commands.\n", 60);
}

int fs_create(const char* filename) {
    // Find free file slot
    int i;
    for (i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].in_use) {
            break;
        }
    }
    
    if (i == FS_MAX_FILES) {
        return -1; // No free file slots
    }
    
    // Check if file already exists
    for (int j = 0; j < FS_MAX_FILES; j++) {
        if (files[j].in_use && strcmp(files[j].filename, filename) == 0) {
            return -2; // File exists
        }
    }
    
    // Create the file
    strncpy(files[i].filename, filename, FS_MAX_FILENAME - 1);
    files[i].filename[FS_MAX_FILENAME - 1] = '\0';
    files[i].size = 0;
    files[i].in_use = 1;
    
    return 0;
}

int fs_delete(const char* filename) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].in_use && strcmp(files[i].filename, filename) == 0) {
            files[i].in_use = 0;
            return 0;
        }
    }
    
    return -1; // File not found
}

int fs_write(const char* filename, const char* data, size_t size) {
    // Find the file
    int i;
    for (i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].in_use && strcmp(files[i].filename, filename) == 0) {
            break;
        }
    }
    
    if (i == FS_MAX_FILES) {
        return -1; // File not found
    }
    
    // Truncate if too large
    if (size > FS_MAX_FILESIZE) {
        size = FS_MAX_FILESIZE;
    }
    
    // Copy data
    memcpy(files[i].data, data, size);
    files[i].size = size;
    
    return size;
}

int fs_read(const char* filename, char* buffer, size_t size) {
    // Find the file
    int i;
    for (i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].in_use && strcmp(files[i].filename, filename) == 0) {
            break;
        }
    }
    
    if (i == FS_MAX_FILES) {
        return -1; // File not found
    }
    
    // Determine how much to read
    if (size > files[i].size) {
        size = files[i].size;
    }
    
    // Copy data
    memcpy(buffer, files[i].data, size);
    
    return size;
}

void fs_list(void) {
    terminal_writestring("Files:\n");
    
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].in_use) {
            terminal_printf("%s (%d bytes)\n", files[i].filename, files[i].size);
            count++;
        }
    }
    
    if (count == 0) {
        terminal_writestring("No files found.\n");
    }
}

int fs_size(const char* filename) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].in_use && strcmp(files[i].filename, filename) == 0) {
            return files[i].size;
        }
    }
    
    return -1; // File not found
}