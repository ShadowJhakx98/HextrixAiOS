// src/shell.c
#include "shell.h"
#include "terminal.h"
#include "keyboard.h"
#include "fs.h"
#include "kmalloc.h"
#include <string.h>

#define COMMAND_BUFFER_SIZE 256

static char command_buffer[COMMAND_BUFFER_SIZE];
static int buffer_pos = 0;

static void shell_execute_command(const char* cmd);

void shell_init(void) {
    terminal_writestring("Hextrix OS Shell\n");
    terminal_writestring("Type 'help' for a list of commands\n");
    terminal_writestring("> ");
    buffer_pos = 0;
}

void shell_run(void) {
    while (1) {
        char c = keyboard_read();
        
        if (c == '\n') {
            terminal_putchar('\n');
            command_buffer[buffer_pos] = '\0';
            
            if (buffer_pos > 0) {
                shell_execute_command(command_buffer);
            }
            
            terminal_writestring("> ");
            buffer_pos = 0;
        } 
        else if (c == '\b') {
            if (buffer_pos > 0) {
                buffer_pos--;
                terminal_writestring("\b \b");  // Erase the character
            }
        }
        else if (buffer_pos < COMMAND_BUFFER_SIZE - 1) {
            command_buffer[buffer_pos++] = c;
        }
    }
}

static void shell_execute_command(const char* cmd) {
    // Split command and arguments
    char command[32] = {0};
    char arg1[32] = {0};
    char arg2[32] = {0};
    
    int args = sscanf(cmd, "%31s %31s %31s", command, arg1, arg2);
    
    // Handle commands
    if (strcmp(command, "help") == 0) {
        terminal_writestring("Available commands:\n");
        terminal_writestring("  help         - Show this help\n");
        terminal_writestring("  clear        - Clear the screen\n");
        terminal_writestring("  ls           - List files\n");
        terminal_writestring("  cat [file]   - Display file contents\n");
        terminal_writestring("  write [file] - Create/write to a file\n");
        terminal_writestring("  rm [file]    - Delete a file\n");
        terminal_writestring("  meminfo      - Display memory usage\n");
    }
    else if (strcmp(command, "clear") == 0) {
        terminal_clear();
    }
    else if (strcmp(command, "ls") == 0) {
        fs_list();
    }
    else if (strcmp(command, "cat") == 0) {
        if (args < 2) {
            terminal_writestring("Usage: cat [filename]\n");
            return;
        }
        
        int size = fs_size(arg1);
        if (size < 0) {
            terminal_printf("File '%s' not found\n", arg1);
            return;
        }
        
        char* buffer = kmalloc(size + 1);
        if (!buffer) {
            terminal_writestring("Out of memory\n");
            return;
        }
        
        fs_read(arg1, buffer, size);
        buffer[size] = '\0';
        terminal_writestring(buffer);
        terminal_putchar('\n');
        
        kfree(buffer);
    }
    else if (strcmp(command, "write") == 0) {
        if (args < 2) {
            terminal_writestring("Usage: write [filename]\n");
            return;
        }
        
        terminal_writestring("Enter file content, end with Ctrl+D:\n");
        
        char content[FS_MAX_FILESIZE] = {0};
        int content_pos = 0;
        
        while (1) {
            char c = keyboard_read();
            if (c == 4) {  // Ctrl+D
                break;
            }
            
            terminal_putchar(c);
            
            if (content_pos < FS_MAX_FILESIZE - 1) {
                content[content_pos++] = c;
            }
        }
        
        content[content_pos] = '\0';
        
        // Create file if it doesn't exist
        if (fs_size(arg1) < 0) {
            fs_create(arg1);
        }
        
        fs_write(arg1, content, content_pos);
        terminal_printf("\nWrote %d bytes to %s\n", content_pos, arg1);
    }
    else if (strcmp(command, "rm") == 0) {
        if (args < 2) {
            terminal_writestring("Usage: rm [filename]\n");
            return;
        }
        
        if (fs_delete(arg1) < 0) {
            terminal_printf("File '%s' not found\n", arg1);
        } else {
            terminal_printf("Deleted '%s'\n", arg1);
        }
    }
    else if (strcmp(command, "meminfo") == 0) {
        size_t total, used, free;
        kmalloc_stats(&total, &used, &free);
        
        terminal_writestring("Memory usage:\n");
        terminal_printf("  Total: %d bytes\n", total);
        terminal_printf("  Used:  %d bytes\n", used);
        terminal_printf("  Free:  %d bytes\n", free);
    }
    else {
        terminal_printf("Unknown command: %s\n", command);
    }
}