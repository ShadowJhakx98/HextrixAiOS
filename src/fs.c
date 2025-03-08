// src/fs.c
#include "fs.h"
#include "terminal.h"
#include "string.h"
#include "stdio.h"
#include "kmalloc.h"
#include <stdint.h>  // Add this line for uint32_t

// Our in-memory file system
static fs_node_t fs_nodes[FS_MAX_FILES];

// Current working directory
static char current_directory[FS_MAX_PATH] = "/";

// Timer ticks for timestamps
extern volatile uint32_t timer_ticks;
// Helper function to find a node by path
static int find_node(const char* path) {
    // Handle absolute vs relative paths
    char full_path[FS_MAX_PATH];
    if (path[0] == '/') {
        // Absolute path
        strncpy(full_path, path, FS_MAX_PATH);
    } else {
        // Relative path - concatenate with current directory
        strncpy(full_path, current_directory, FS_MAX_PATH);
        if (current_directory[strlen(current_directory)-1] != '/') {
            strncat(full_path, "/", FS_MAX_PATH - strlen(full_path) - 1);
        }
        strncat(full_path, path, FS_MAX_PATH - strlen(full_path) - 1);
    }
    
    // Normalize path (remove ./ and ../)
    // ... (would implement path normalization here) ...
    
    // Search for the node
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_nodes[i].in_use && strcmp(fs_nodes[i].path, full_path) == 0) {
            return i;
        }
    }
    
    return -1; // Not found
}

// Helper function to get parent directory index
static int get_parent_index(const char* path) {
    // If root, has no parent
    if (strcmp(path, "/") == 0) {
        return -1;
    }
    
    // Make a copy of the path we can modify
    char path_copy[FS_MAX_PATH];
    strncpy(path_copy, path, FS_MAX_PATH);
    
    // Find the last slash
    int len = strlen(path_copy);
    int last_slash = len - 1;
    while (last_slash >= 0 && path_copy[last_slash] != '/') {
        last_slash--;
    }
    
    // If no slash, parent is root
    if (last_slash < 0) {
        return find_node("/");
    }
    
    // If this is the root directory's child
    if (last_slash == 0) {
        path_copy[1] = '\0';  // Just "/"
    } else {
        path_copy[last_slash] = '\0';  // Terminate at last slash
    }
    
    return find_node(path_copy);
}

// Helper function to extract file name from path
static void extract_name(const char* path, char* name) {
    const char* last_slash = strrchr(path, '/');
    if (last_slash == NULL) {
        // No slash, use the whole path
        strncpy(name, path, FS_MAX_FILENAME);
    } else {
        // Copy after the last slash
        strncpy(name, last_slash + 1, FS_MAX_FILENAME);
    }
}

void fs_init(void) {
    // Initialize all nodes as unused
    for (int i = 0; i < FS_MAX_FILES; i++) {
        fs_nodes[i].in_use = 0;
        fs_nodes[i].size = 0;
    }
    
    // Create root directory
    int root_idx = 0;
    fs_nodes[root_idx].in_use = 1;
    fs_nodes[root_idx].type = FS_TYPE_DIRECTORY;
    strcpy(fs_nodes[root_idx].name, "/");
    strcpy(fs_nodes[root_idx].path, "/");
    fs_nodes[root_idx].parent_index = -1;
    fs_nodes[root_idx].permissions = 0777;  // rwxrwxrwx
    fs_nodes[root_idx].created_time = timer_ticks;
    fs_nodes[root_idx].modified_time = timer_ticks;
    
    // Create README.txt in root directory
    fs_create("/README.txt", FS_TYPE_FILE);
    fs_write("/README.txt", "Welcome to Hextrix OS v0.3.2!\nType 'help' for available commands.\n", 60);
    
    // Create some initial directories
    fs_mkdir("/home");
    fs_mkdir("/bin");
    fs_mkdir("/etc");
}

int fs_create(const char* path, int type) {
    // Check if file already exists
    if (find_node(path) >= 0) {
        return -1; // Already exists
    }
    
    // Find a free node
    int free_idx = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!fs_nodes[i].in_use) {
            free_idx = i;
            break;
        }
    }
    
    if (free_idx < 0) {
        return -2; // No free inodes
    }
    
    // Get parent directory
    int parent_idx = get_parent_index(path);
    if (parent_idx < 0 && strcmp(path, "/") != 0) {
        return -3; // Parent doesn't exist
    }
    
    // If parent exists, make sure it's a directory
    if (parent_idx >= 0 && fs_nodes[parent_idx].type != FS_TYPE_DIRECTORY) {
        return -4; // Parent is not a directory
    }
    
    // Initialize the new node
    fs_nodes[free_idx].in_use = 1;
    fs_nodes[free_idx].type = type;
    strncpy(fs_nodes[free_idx].path, path, FS_MAX_PATH);
    extract_name(path, fs_nodes[free_idx].name);
    fs_nodes[free_idx].parent_index = parent_idx;
    fs_nodes[free_idx].size = 0;
    fs_nodes[free_idx].permissions = 0755;  // rwxr-xr-x
    fs_nodes[free_idx].created_time = timer_ticks;
    fs_nodes[free_idx].modified_time = timer_ticks;
    
    return 0;
}

int fs_delete(const char* path) {
    int node_idx = find_node(path);
    if (node_idx < 0) {
        return -1; // Not found
    }
    
    // Can't delete root
    if (strcmp(fs_nodes[node_idx].path, "/") == 0) {
        return -2;
    }
    
    // If directory, make sure it's empty
    if (fs_nodes[node_idx].type == FS_TYPE_DIRECTORY) {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (fs_nodes[i].in_use && fs_nodes[i].parent_index == node_idx) {
                return -3; // Directory not empty
            }
        }
    }
    
    // Delete the node
    fs_nodes[node_idx].in_use = 0;
    return 0;
}

int fs_write(const char* path, const char* data, size_t size) {
    int node_idx = find_node(path);
    if (node_idx < 0) {
        return -1; // Not found
    }
    
    // Must be a file, not a directory
    if (fs_nodes[node_idx].type != FS_TYPE_FILE) {
        return -2;
    }
    
    // Check size limit
    if (size > FS_MAX_FILESIZE) {
        size = FS_MAX_FILESIZE;  // Truncate
    }
    
    // Copy data
    memcpy(fs_nodes[node_idx].data, data, size);
    fs_nodes[node_idx].size = size;
    fs_nodes[node_idx].modified_time = timer_ticks;
    
    return size;
}

int fs_read(const char* path, char* buffer, size_t size) {
    int node_idx = find_node(path);
    if (node_idx < 0) {
        return -1; // Not found
    }
    
    // Must be a file, not a directory
    if (fs_nodes[node_idx].type != FS_TYPE_FILE) {
        return -2;
    }
    
    // Determine how much to read
    if (size > fs_nodes[node_idx].size) {
        size = fs_nodes[node_idx].size;
    }
    
    // Copy data
    memcpy(buffer, fs_nodes[node_idx].data, size);
    
    return size;
}

void fs_list(const char* path) {
    // Find the directory
    int dir_idx;
    if (strlen(path) == 0) {
        // Use current directory if empty path
        dir_idx = find_node(current_directory);
    } else {
        dir_idx = find_node(path);
    }
    
    if (dir_idx < 0) {
        terminal_printf("Directory '%s' not found\n", path);
        return;
    }
    
    // Check if it's a directory
    if (fs_nodes[dir_idx].type != FS_TYPE_DIRECTORY) {
        terminal_printf("'%s' is not a directory\n", path);
        return;
    }
    
    // Print directory contents
    terminal_printf("Contents of %s:\n", fs_nodes[dir_idx].path);
    
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_nodes[i].in_use && fs_nodes[i].parent_index == dir_idx) {
            if (fs_nodes[i].type == FS_TYPE_DIRECTORY) {
                terminal_printf("[DIR]  %s/\n", fs_nodes[i].name);
            } else {
                terminal_printf("[FILE] %s (%d bytes)\n", fs_nodes[i].name, fs_nodes[i].size);
            }
            count++;
        }
    }
    
    if (count == 0) {
        terminal_writestring("Directory is empty.\n");
    }
}

int fs_size(const char* path) {
    int node_idx = find_node(path);
    if (node_idx < 0) {
        return -1; // Not found
    }
    
    // Must be a file, not a directory
    if (fs_nodes[node_idx].type != FS_TYPE_FILE) {
        return -2;
    }
    
    return fs_nodes[node_idx].size;
}

int fs_mkdir(const char* path) {
    return fs_create(path, FS_TYPE_DIRECTORY);
}

int fs_chdir(const char* path) {
    int node_idx = find_node(path);
    if (node_idx < 0) {
        return -1; // Not found
    }
    
    // Must be a directory
    if (fs_nodes[node_idx].type != FS_TYPE_DIRECTORY) {
        return -2;
    }
    
    // Update current directory
    strncpy(current_directory, fs_nodes[node_idx].path, FS_MAX_PATH);
    return 0;
}

const char* fs_getcwd(void) {
    return current_directory;
}

int fs_stat(const char* path, fs_node_t* info) {
    int node_idx = find_node(path);
    if (node_idx < 0) {
        return -1; // Not found
    }
    
    // Copy node info
    memcpy(info, &fs_nodes[node_idx], sizeof(fs_node_t));
    return 0;
}