// src/shell.c
#include "shell.h"
#include "terminal.h"
#include "string.h"
#include "stdio.h"
#include "fs.h"
#include "kmalloc.h"
#include "interrupts.h" // For keyboard_poll() and timer_ticks
#include "process.h"
#include "scheduler.h"
#include "memory.h" // For memory protection functions
#include "io.h" // For inb function in diagnostics
// At the top of src/shell.c, add:
#include "hal.h"

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

// Simple diagnostic function to use in shell.c
// This doesn't require the full diagnostics library
void run_interrupt_diagnostics_simple(void) {
    // Display basic CPU/system information 
    terminal_writestring("Basic System Diagnostics:\n");
    terminal_writestring("----------------------------\n");
    
    // Get some basic system information using inline assembly
    uint32_t eflags;
    asm volatile("pushf\n\tpop %0" : "=r"(eflags));
    
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    
    // Print basic info
    terminal_printf("EFLAGS: 0x%x (", eflags);
    // Decode some EFLAGS bits
    if (eflags & 0x00000200) terminal_writestring("IF ");
    if (eflags & 0x00000100) terminal_writestring("TF ");
    if (eflags & 0x00000004) terminal_writestring("PF ");
    if (eflags & 0x00000001) terminal_writestring("CF ");
    terminal_writestring(")\n");
    
    terminal_printf("CR0: 0x%x (", cr0);
    // Decode some CR0 bits
    if (cr0 & 0x80000000) terminal_writestring("PG ");
    if (cr0 & 0x00010000) terminal_writestring("WP ");
    if (cr0 & 0x00000001) terminal_writestring("PE ");
    terminal_writestring(")\n");
    
    // Check PIC state
    uint8_t pic1_mask = inb(0x21);
    uint8_t pic2_mask = inb(0xA1);
    
    terminal_printf("PIC1 Mask: 0x%x\n", pic1_mask);
    terminal_printf("PIC2 Mask: 0x%x\n", pic2_mask);
    
    terminal_writestring("\nIndividual IRQ Status:\n");
    for (int i = 0; i < 8; i++) {
        terminal_printf("IRQ%d: %s\n", i, 
            (pic1_mask & (1 << i)) ? "Masked" : "Enabled");
    }
    
    for (int i = 0; i < 8; i++) {
        terminal_printf("IRQ%d: %s\n", i + 8, 
            (pic2_mask & (1 << i)) ? "Masked" : "Enabled");
    }
    
    terminal_writestring("\nSystem is running in polling mode.\n");
}

// Initialize the shell
void shell_init(void) {
    terminal_writestring("Hextrix OS v0.3.7-dev-v2 - HAL Edition\n");
    terminal_writestring("Type 'help' for a list of commands\n");
    terminal_writestring(PROMPT);
    buffer_pos = 0;
}

// Process a single key from keyboard input
void shell_handle_key(int scancode) {
    if (scancode < 0 || scancode >= sizeof(scancode_to_ascii)) {
        return;
    }
    
    // Convert scancode to ASCII
    char c = scancode_to_ascii[scancode];
    
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
        terminal_writestring("  ps           - List running processes\n");
        terminal_writestring("  kill [pid]   - Terminate a process\n");
        terminal_writestring("  nice [pid] [priority] - Change process priority\n");
        terminal_writestring("  sleep [ms]   - Sleep current shell for milliseconds\n");
        terminal_writestring("  version      - Show OS version\n");
        terminal_writestring("  memenable    - Enable memory protection\n");
        terminal_writestring("  memdisable   - Disable memory protection\n");
        terminal_writestring("  memcheck [addr] [flags] - Check if memory access is valid\n");
        terminal_writestring("  memregions   - Display memory region information\n");
        terminal_writestring("  diag         - Run system diagnostics\n");
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
		terminal_writestring("Hextrix OS v0.3.7-dev-v2 - HAL Edition\n");
    }
    // Process management commands
    else if (strcmp(cmd, "ps") == 0) {
        process_list();
    }
    else if (strcmp(cmd, "kill") == 0) {
        if (args < 1) {
            terminal_writestring("Usage: kill [pid]\n");
            return;
        }
        
        int pid = 0;
        for (i = 0; arg1[i]; i++) {
            if (arg1[i] >= '0' && arg1[i] <= '9') {
                pid = pid * 10 + (arg1[i] - '0');
            } else {
                terminal_writestring("Invalid PID format\n");
                return;
            }
        }
        
        if (pid <= 0) {
            terminal_writestring("Cannot kill system processes\n");
            return;
        }
        
        process_terminate(pid);
    }
    else if (strcmp(cmd, "nice") == 0) {
        if (args < 2) {
            terminal_writestring("Usage: nice [pid] [priority: 0=low, 1=normal, 2=high, 3=realtime]\n");
            return;
        }
        
        int pid = 0;
        for (i = 0; arg1[i]; i++) {
            if (arg1[i] >= '0' && arg1[i] <= '9') {
                pid = pid * 10 + (arg1[i] - '0');
            } else {
                terminal_writestring("Invalid PID format\n");
                return;
            }
        }
        
        int priority = 0;
        for (i = 0; arg2[i]; i++) {
            if (arg2[i] >= '0' && arg2[i] <= '9') {
                priority = priority * 10 + (arg2[i] - '0');
            } else {
                terminal_writestring("Invalid priority format\n");
                return;
            }
        }
        
        if (priority > PROCESS_PRIORITY_REALTIME) {
            terminal_printf("Invalid priority: %d. Must be 0-3\n", priority);
            return;
        }
        
        process_set_priority(pid, priority);
        terminal_printf("Set PID %d priority to %d\n", pid, priority);
    }
    else if (strcmp(cmd, "sleep") == 0) {
        if (args < 1) {
            terminal_writestring("Usage: sleep [milliseconds]\n");
            return;
        }
        
        int ms = 0;
        for (i = 0; arg1[i]; i++) {
            if (arg1[i] >= '0' && arg1[i] <= '9') {
                ms = ms * 10 + (arg1[i] - '0');
            } else {
                terminal_writestring("Invalid time format\n");
                return;
            }
        }
        
        terminal_printf("Sleeping for %d ms...\n", ms);
        
        // For testing, we'll use a busy-wait approach
        // In a real implementation with process scheduling,
        // we would use process_sleep() here
        uint32_t start_time = timer_ticks;
        uint32_t ticks_to_wait = (ms / 10) + 1;  // Convert ms to ticks
        
        while (timer_ticks - start_time < ticks_to_wait) {
            // Just keep polling the timer
            timer_poll();
        }
        
        terminal_writestring("Done sleeping\n");
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
    else if (strcmp(cmd, "diag") == 0 || strcmp(cmd, "intdiag") == 0) {
        terminal_writestring("Running system diagnostics...\n");
        run_interrupt_diagnostics_simple();
    }
    else if (strcmp(cmd, "memenable") == 0) {
        enable_memory_protection();
    }
    else if (strcmp(cmd, "memdisable") == 0) {
        disable_memory_protection();
    }
    else if (strcmp(cmd, "memcheck") == 0) {
        if (args < 2) {
            terminal_writestring("Usage: memcheck [addr] [flags: 1=R,2=W,4=X,8=U]\n");
            return;
        }
        
        // Parse the address
        uint32_t addr = 0;
        for (i = 0; arg1[i]; i++) {
            if (arg1[i] >= '0' && arg1[i] <= '9') {
                addr = addr * 10 + (arg1[i] - '0');
            } else if (arg1[i] >= 'a' && arg1[i] <= 'f') {
                addr = addr * 16 + (arg1[i] - 'a' + 10);
            } else if (arg1[i] >= 'A' && arg1[i] <= 'F') {
                addr = addr * 16 + (arg1[i] - 'A' + 10);
            } else if (arg1[i] == 'x' || arg1[i] == 'X') {
                addr = 0; // Start of hex notation
            } else {
                terminal_writestring("Invalid address format\n");
                return;
            }
        }
        
        // Parse the flags
        uint32_t flags = 0;
        for (i = 0; arg2[i]; i++) {
            if (arg2[i] >= '0' && arg2[i] <= '9') {
                flags = flags * 10 + (arg2[i] - '0');
            } else {
                terminal_writestring("Invalid flags format\n");
                return;
            }
        }
        
        // Check if memory access is valid
        int valid = is_valid_access(addr, flags);
        
        terminal_printf("Memory access to 0x%x with flags 0x%x is %s\n",
            addr, flags, valid ? "valid" : "invalid");
    }
    else if (strcmp(cmd, "memregions") == 0) {
        display_memory_regions();
    }
    else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(cmd);
        terminal_writestring("\n");
    }
}

// Modify shell_run in src/shell.c
void shell_run(void) {
    while (1) {
        // Poll for keyboard input using HAL
        if (hal_keyboard_is_key_available()) {
            int scancode = hal_keyboard_read();
            shell_handle_key(scancode);
        }
    }
}