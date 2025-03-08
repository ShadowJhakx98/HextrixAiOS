// src/shell.c
#include "shell.h"
#include "terminal.h"
#include "string.h"
#include "stdio.h"
#include "fs.h"
#include "kmalloc.h"
#include "interrupts.h" // For keyboard_poll()

#define COMMAND_BUFFER_SIZE 256
#define PROMPT "> "

static char command_buffer[COMMAND_BUFFER_SIZE];
static int buffer_pos = 0;

// Simple ASCII conversion table for scancodes
// Only includes basic keys, can be expanded
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// Initialize the shell
void shell_init(void) {
    terminal_writestring("Hextrix OS v0.3.2 - Polling Shell\n");
    terminal_writestring("Type 'help' for a list of commands\n");
    terminal_writestring(PROMPT);
    buffer_pos = 0;
}

// Process a command
void shell_process_command(const char* command) {
    // Empty command
    if (strlen(command) == 0) {
        return;
    }
    
    // Parse the command and arguments
    char cmd[32];
    char arg1[64];
    char arg2[64];
    int args = 0;
    
    // Simple command parsing
    int i = 0, j = 0;
    
    // Skip leading whitespace
    while (command[i] == ' ') i++;
    
    // Get command
    while (command[i] && command[i] != ' ' && j < 31) {
        cmd[j++] = command[i++];
    }
    cmd[j] = '\0';
    
    // Skip whitespace
    while (command[i] == ' ') i++;
    
    // Get first argument if exists
    j = 0;
    if (command[i]) {
        args++;
        while (command[i] && command[i] != ' ' && j < 63) {
            arg1[j++] = command[i++];
        }
    }
    arg1[j] = '\0';
    
    // Skip whitespace
    while (command[i] == ' ') i++;
    
    // Get second argument if exists
    j = 0;
    if (command[i]) {
        args++;
        while (command[i] && command[i] != ' ' && j < 63) {
            arg2[j++] = command[i++];
        }
    }
    arg2[j] = '\0';
    
    // Process commands
    if (strcmp(cmd, "help") == 0) {
        terminal_writestring("Available commands:\n");
        terminal_writestring("  help         - Show this help\n");
        terminal_writestring("  clear        - Clear the screen\n");
        terminal_writestring("  echo [text]  - Display text\n");
        terminal_writestring("  ls [dir]     - List files in directory\n");
        terminal_writestring("  cd [dir]     - Change current directory\n");
        terminal_writestring("  pwd          - Show current directory\n");
        terminal_writestring("  mkdir [dir]  - Create a directory\n");
        terminal_writestring("  cat [file]   - Display file contents\n");
        terminal_writestring("  write [file] - Create/edit a file\n");
        terminal_writestring("  rm [file]    - Delete a file\n");
        terminal_writestring("  meminfo      - Display memory usage\n");
        terminal_writestring("  version      - Show OS version\n");
    }
    else if (strcmp(cmd, "clear") == 0) {
        terminal_clear();
    }
    else if (strcmp(cmd, "echo") == 0) {
        if (args >= 1) {
            terminal_writestring(arg1);
            if (args >= 2) {
                terminal_writestring(" ");
                terminal_writestring(arg2);
            }
            terminal_writestring("\n");
        } else {
            terminal_writestring("\n");
        }
    }
    else if (strcmp(cmd, "meminfo") == 0) {
        size_t total, used, free;
        kmalloc_stats(&total, &used, &free);
        
        terminal_writestring("Memory usage:\n");
        terminal_printf("  Total: %d bytes\n", total);
        terminal_printf("  Used:  %d bytes\n", used);
        terminal_printf("  Free:  %d bytes\n", free);
    }
    else if (strcmp(cmd, "version") == 0) {
        terminal_writestring("Hextrix OS v0.3.2 - Polling-based system\n");
    }
    // File system commands
    else if (strcmp(cmd, "ls") == 0) {
        if (args < 1) {
            fs_list("");  // List current directory
        } else {
            fs_list(arg1);
        }
    }
    else if (strcmp(cmd, "cat") == 0) {
        if (args < 1) {
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
        terminal_writestring("\n");
        
        kfree(buffer);
    }
    else if (strcmp(cmd, "write") == 0) {
        if (args < 1) {
            terminal_writestring("Usage: write [filename]\n");
            return;
        }
        
        terminal_writestring("Enter file content (end with Ctrl+D on new line):\n");
        
        char content[FS_MAX_FILESIZE] = {0};
        int content_pos = 0;
        int line_start = 1;  // Flag to track start of new line
        
        while (content_pos < FS_MAX_FILESIZE - 1) {
            // Poll for keyboard input
            int scancode = keyboard_poll();
            
            if (scancode > 0 && scancode < sizeof(scancode_to_ascii)) {
                char c = scancode_to_ascii[scancode];
                
                if (c) {
                    if (c == '\n') {
                        // Check for Ctrl+D at beginning of line
                        if (line_start && content_pos > 0) {
                            break;
                        }
                        
                        line_start = 1;  // Next char will be at start of line
                        content[content_pos++] = c;
                        terminal_putchar(c);
                    } else {
                        line_start = 0;  // No longer at start of line
                        content[content_pos++] = c;
                        terminal_putchar(c);
                    }
                }
            }
            
            // Don't hog the CPU
            for (volatile int i = 0; i < 100000; i++) {}
        }
        
        content[content_pos] = '\0';
        
        // Create file if it doesn't exist
        if (fs_size(arg1) < 0) {
            fs_create(arg1, FS_TYPE_FILE);
        }
        
        fs_write(arg1, content, content_pos);
        terminal_printf("\nWrote %d bytes to %s\n", content_pos, arg1);
    }
    else if (strcmp(cmd, "rm") == 0) {
        if (args < 1) {
            terminal_writestring("Usage: rm [filename]\n");
            return;
        }
        
        if (fs_delete(arg1) < 0) {
            terminal_printf("File '%s' not found\n", arg1);
        } else {
            terminal_printf("Deleted '%s'\n", arg1);
        }
    }
    else if (strcmp(cmd, "pwd") == 0) {
        terminal_printf("%s\n", fs_getcwd());
    }
    else if (strcmp(cmd, "cd") == 0) {
        if (args < 1) {
            // CD to root if no arguments
            fs_chdir("/");
            return;
        }
        
        if (fs_chdir(arg1) < 0) {
            terminal_printf("Cannot change to directory '%s'\n", arg1);
        }
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        if (args < 1) {
            terminal_writestring("Usage: mkdir [directory]\n");
            return;
        }
        
        if (fs_mkdir(arg1) < 0) {
            terminal_printf("Failed to create directory '%s'\n", arg1);
        } else {
            terminal_printf("Created directory '%s'\n", arg1);
        }
    }
    else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(cmd);
        terminal_writestring("\n");
    }
}

// Run the shell
void shell_run(void) {
    int scancode;
    char c;
    
    while (1) {
        // Poll the timer to keep system responsive
        timer_poll();
        
        // Poll for keyboard input
        scancode = keyboard_poll();
        
        // Process keyboard input if available
        if (scancode > 0 && scancode < sizeof(scancode_to_ascii)) {
            // Convert scancode to ASCII
            c = scancode_to_ascii[scancode];
            
            // Process the character if valid
            if (c) {
                if (c == '\n') {
                    // End of command, process it
                    terminal_putchar('\n');
                    command_buffer[buffer_pos] = '\0';
                    
                    shell_process_command(command_buffer);
                    
                    // Reset buffer and show prompt
                    buffer_pos = 0;
                    terminal_writestring(PROMPT);
                } 
                else if (c == '\b') {
                    // Backspace
                    if (buffer_pos > 0) {
                        buffer_pos--;
                        terminal_putchar('\b');
                        terminal_putchar(' ');
                        terminal_putchar('\b');
                    }
                }
                else if (buffer_pos < COMMAND_BUFFER_SIZE - 1) {
                    // Regular character
                    command_buffer[buffer_pos++] = c;
                    terminal_putchar(c);
                }
            }
        }
    }
}