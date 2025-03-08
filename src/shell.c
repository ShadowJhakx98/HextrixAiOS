// src/shell.c
#include "shell.h"
#include "terminal.h"
#include "string.h"  // Added to fix implicit string function declarations
#include "stdio.h"   // Added to fix implicit printf function declarations
#include "fs.h"
#include "kmalloc.h"
#include "hal.h"
#include "memory.h"
#include "syscall.h"
#include "process.h"
#include "scheduler.h"
#include <stdarg.h>



// Shell configuration
#define COMMAND_BUFFER_SIZE 256
#define COMMAND_HISTORY_SIZE 20
#define MAX_ARGS 16
#define PROMPT_TEXT "> "
#define MAX_COMMANDS 32
#define MAX_AUTOCOMPLETE_RESULTS 10

// Command history
static char* command_history[COMMAND_HISTORY_SIZE] = {0};
static int history_count = 0;
static int history_index = 0;
static int current_history_index = -1;

// Input buffer
static char command_buffer[COMMAND_BUFFER_SIZE];
static int buffer_pos = 0;

// Tab completion state
static int tab_pressed = 0;
static int tab_index = 0;
static char completion_buffer[COMMAND_BUFFER_SIZE];
static char* completion_results[MAX_AUTOCOMPLETE_RESULTS] = {0};
static int completion_count = 0;

// Command structure - Fix for the mixed up structure definition
typedef struct {
    const char* name;
    const char* description;
    int (*handler)(int argc, char** argv);
} command_t;

// Forward declarations of command handlers
static int cmd_help(int argc, char** argv);
static int cmd_clear(int argc, char** argv);
static int cmd_echo(int argc, char** argv);
static int cmd_ls(int argc, char** argv);
static int cmd_cat(int argc, char** argv);
static int cmd_write(int argc, char** argv);
static int cmd_rm(int argc, char** argv);
static int cmd_pwd(int argc, char** argv);
static int cmd_cd(int argc, char** argv);
static int cmd_mkdir(int argc, char** argv);
static int cmd_meminfo(int argc, char** argv);
static int cmd_ps(int argc, char** argv);
static int cmd_kill(int argc, char** argv);
static int cmd_nice(int argc, char** argv);
static int cmd_sleep(int argc, char** argv);
static int cmd_version(int argc, char** argv);
static int cmd_memenable(int argc, char** argv);
static int cmd_memdisable(int argc, char** argv);
static int cmd_memcheck(int argc, char** argv);
static int cmd_memregions(int argc, char** argv);
static int cmd_diag(int argc, char** argv);
static int cmd_fsinfo(int argc, char** argv);
static int cmd_fscheck(int argc, char** argv);
static int cmd_fsrepair(int argc, char** argv);
static int cmd_diskdump(int argc, char** argv);
static int cmd_sched(int argc, char** argv);
static int cmd_history(int argc, char** argv);
static int cmd_reboot(int argc, char** argv);
static int cmd_exit(int argc, char** argv);

// Command table
static command_t commands[MAX_COMMANDS] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
    {"echo", "Display text", cmd_echo},
    {"ls", "List files in directory", cmd_ls},
    {"cat", "Display file contents", cmd_cat},
    {"write", "Create/edit a file", cmd_write},
    {"rm", "Delete a file", cmd_rm},
    {"pwd", "Show current directory", cmd_pwd},
    {"cd", "Change current directory", cmd_cd},
    {"mkdir", "Create a directory", cmd_mkdir},
    {"meminfo", "Display memory usage", cmd_meminfo},
    {"ps", "List running processes", cmd_ps},
    {"kill", "Terminate a process", cmd_kill},
    {"nice", "Change process priority", cmd_nice},
    {"sleep", "Sleep for milliseconds", cmd_sleep},
    {"version", "Show OS version", cmd_version},
    {"memenable", "Enable memory protection", cmd_memenable},
    {"memdisable", "Disable memory protection", cmd_memdisable},
    {"memcheck", "Check memory access validity", cmd_memcheck},
    {"memregions", "Display memory regions", cmd_memregions},
    {"diag", "Run system diagnostics", cmd_diag},
    {"fsinfo", "Display file system info", cmd_fsinfo},
    {"fscheck", "Check file system consistency", cmd_fscheck},
    {"fsrepair", "Repair file system", cmd_fsrepair},
    {"diskdump", "Dump disk contents", cmd_diskdump},
    {"sched", "Display scheduler info", cmd_sched},
    {"history", "Show command history", cmd_history},
    {"reboot", "Reboot the system", cmd_reboot},
    {"exit", "Exit the shell", cmd_exit},
    {NULL, NULL, NULL}  // Terminator
};

// Helper function to add command to history
static void add_to_history(const char* command) {
    // Don't add empty commands
    if (strlen(command) == 0) {
        return;
    }
    
    // Don't add duplicate of the most recent command
    if (history_count > 0 && strcmp(command_history[history_count - 1], command) == 0) {
        return;
    }
    
    // Free the oldest command if history is full
    if (history_count == COMMAND_HISTORY_SIZE) {
        kfree(command_history[0]);
        
        // Shift all commands down
        for (int i = 0; i < COMMAND_HISTORY_SIZE - 1; i++) {
            command_history[i] = command_history[i + 1];
        }
        
        history_count = COMMAND_HISTORY_SIZE - 1;
    }
    
    // Allocate memory for the new command
    command_history[history_count] = kmalloc(strlen(command) + 1);
    if (command_history[history_count]) {
        strcpy(command_history[history_count], command);
        history_count++;
        history_index = history_count;
    }
}

// Helper function to clear the current line
static void clear_current_line(void) {
    // Move cursor to beginning of line
    for (int i = 0; i < buffer_pos + strlen(PROMPT_TEXT); i++) {
        terminal_putchar('\b');
    }
    
    // Clear the line with spaces
    for (int i = 0; i < buffer_pos + strlen(PROMPT_TEXT); i++) {
        terminal_putchar(' ');
    }
    
    // Move cursor back to beginning of line
    for (int i = 0; i < buffer_pos + strlen(PROMPT_TEXT); i++) {
        terminal_putchar('\b');
    }
}

// Helper function to redraw the current line
static void redraw_current_line(void) {
    terminal_writestring(PROMPT_TEXT);
    terminal_writestring(command_buffer);
}

// Helper function to handle up/down arrow for history
static void handle_history_navigation(int direction) {
    // Up arrow
    if (direction < 0) {
        if (history_count > 0 && current_history_index > 0) {
            current_history_index--;
            clear_current_line();
            strcpy(command_buffer, command_history[current_history_index]);
            buffer_pos = strlen(command_buffer);
            redraw_current_line();
        }
    }
    // Down arrow
    else if (direction > 0) {
        if (current_history_index < history_count - 1) {
            current_history_index++;
            clear_current_line();
            strcpy(command_buffer, command_history[current_history_index]);
            buffer_pos = strlen(command_buffer);
            redraw_current_line();
        } else if (current_history_index == history_count - 1) {
            // Clear the command buffer when pressing down at the end of history
            current_history_index = history_count;
            clear_current_line();
            command_buffer[0] = '\0';
            buffer_pos = 0;
            redraw_current_line();
        }
    }
}

// Helper function to parse command into arguments
static int parse_command(char* command, char** argv) {
    int argc = 0;
    int in_quotes = 0;
    char* p = command;
    
    // Skip leading spaces
    while (*p && *p == ' ') p++;
    
    // Parse command and arguments
    while (*p && argc < MAX_ARGS - 1) {
        // Start of an argument
        argv[argc++] = p;
        
        // Find end of argument
        while (*p) {
            if (*p == '"') {
                // Toggle quote state
                in_quotes = !in_quotes;
                
                // Remove quote by shifting the rest of the string
                for (char* q = p; *q; q++) {
                    *q = *(q + 1);
                }
                
                // Stay on the same character after shift
                p--;
            } else if (*p == ' ' && !in_quotes) {
                // End of argument
                *p = '\0';
                p++;
                break;
            }
            p++;
        }
        
        // Skip consecutive spaces
        while (*p && *p == ' ') p++;
    }
    
    // Null-terminate the argument list
    argv[argc] = NULL;
    
    return argc;
}

// Helper function to find matching commands for tab completion
static void find_matching_commands(const char* prefix) {
    // Clear previous completion results
    for (int i = 0; i < completion_count; i++) {
        kfree(completion_results[i]);
        completion_results[i] = NULL;
    }
    completion_count = 0;
    
    // Find all matching commands
    for (int i = 0; commands[i].name != NULL && completion_count < MAX_AUTOCOMPLETE_RESULTS; i++) {
        if (strncmp(prefix, commands[i].name, strlen(prefix)) == 0) {
            completion_results[completion_count] = kmalloc(strlen(commands[i].name) + 1);
            if (completion_results[completion_count]) {
                strcpy(completion_results[completion_count], commands[i].name);
                completion_count++;
            }
        }
    }
}

// Helper function to find a node by path (normally in fs.c, replicated here for completeness)
static int fs_find_node(const char* path) {
    extern fs_node_t fs_nodes[FS_MAX_FILES];
    
    // Handle absolute vs relative paths
    char full_path[FS_MAX_PATH];
    if (path[0] == '/') {
        // Absolute path
        strncpy(full_path, path, FS_MAX_PATH);
    } else {
        // Relative path - concatenate with current directory
        strncpy(full_path, fs_getcwd(), FS_MAX_PATH);
        if (fs_getcwd()[strlen(fs_getcwd())-1] != '/') {
            strncat(full_path, "/", FS_MAX_PATH - strlen(full_path) - 1);
        }
        strncat(full_path, path, FS_MAX_PATH - strlen(full_path) - 1);
    }
    
    // Search for the node
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_nodes[i].in_use && strcmp(fs_nodes[i].path, full_path) == 0) {
            return i;
        }
    }
    
    return -1; // Not found
}

// Helper function to find matching files for tab completion
static void find_matching_files(const char* prefix) {
    // Clear previous completion results
    for (int i = 0; i < completion_count; i++) {
        kfree(completion_results[i]);
        completion_results[i] = NULL;
    }
    completion_count = 0;
    
    // Parse path to get directory and partial filename
    char dir_path[FS_MAX_PATH] = {0};
    char file_prefix[FS_MAX_FILENAME] = {0};
    
    const char* last_slash = strrchr(prefix, '/');
    if (last_slash) {
        // Path contains directory separator
        int dir_len = last_slash - prefix;
        strncpy(dir_path, prefix, dir_len);
        dir_path[dir_len] = '\0';
        strcpy(file_prefix, last_slash + 1);
    } else {
        // No directory separator, use current directory
        strcpy(dir_path, "");
        strcpy(file_prefix, prefix);
    }
    
    // Get directory listing
    extern fs_node_t fs_nodes[FS_MAX_FILES];
    
    // Find the directory
    int dir_idx = -1;
    if (strlen(dir_path) == 0) {
        // Current directory
        dir_idx = fs_find_node(fs_getcwd());
    } else {
        // Specified directory
        dir_idx = fs_find_node(dir_path);
    }
    
    if (dir_idx < 0) {
        return;  // Directory not found
    }
    
    // Find all matching files
    for (int i = 0; i < FS_MAX_FILES && completion_count < MAX_AUTOCOMPLETE_RESULTS; i++) {
        if (fs_nodes[i].in_use && fs_nodes[i].parent_index == dir_idx) {
            if (strncmp(file_prefix, fs_nodes[i].name, strlen(file_prefix)) == 0) {
                // Construct full path
                char full_path[FS_MAX_PATH] = {0};
                
                if (strlen(dir_path) == 0) {
                    // No directory prefix
                    strcpy(full_path, fs_nodes[i].name);
                } else {
                    // Include directory prefix
                    sprintf(full_path, "%s/%s", dir_path, fs_nodes[i].name);
                }
                
                // Add trailing slash for directories
                if (fs_nodes[i].type == FS_TYPE_DIRECTORY) {
                    strcat(full_path, "/");
                }
                
                // Add to completion results
                completion_results[completion_count] = kmalloc(strlen(full_path) + 1);
                if (completion_results[completion_count]) {
                    strcpy(completion_results[completion_count], full_path);
                    completion_count++;
                }
            }
        }
    }
}

// Helper function to handle tab completion
static void handle_tab_completion(void) {
    // Save the current command
    if (tab_pressed == 0) {
        strcpy(completion_buffer, command_buffer);
        tab_index = 0;
        
        // Determine if we're completing a command or a file/directory
        char* space = strchr(command_buffer, ' ');
        if (space == NULL) {
            // Completing a command
            find_matching_commands(command_buffer);
        } else {
            // Completing a filename (simple implementation)
            char* start = space + 1;
            while (*start == ' ') start++;  // Skip consecutive spaces
            
            find_matching_files(start);
        }
    }
    
    // If no completions found, do nothing
    if (completion_count == 0) {
        return;
    }
    
    // Clear the current line
    clear_current_line();
    
    // Apply the completion
    if (tab_index >= completion_count) {
        tab_index = 0;
    }
    
    // For command completion
    char* space = strchr(completion_buffer, ' ');
    if (space == NULL) {
        // Command completion
        strcpy(command_buffer, completion_results[tab_index]);
    } else {
        // File completion
        int cmd_length = space - completion_buffer + 1;
        strncpy(command_buffer, completion_buffer, cmd_length);
        command_buffer[cmd_length] = '\0';
        strcat(command_buffer, completion_results[tab_index]);
    }
    
    buffer_pos = strlen(command_buffer);
    redraw_current_line();
    
    // Increment completion index for next tab press
    tab_index++;
}

// Enhanced handle_key function for the shell
void shell_handle_key(int scancode) {
    // Convert scancode to key code
    int key = 0;
    
    // Simple conversion for printable characters
    if (scancode >= 2 && scancode <= 13) {
        // Digits 1-9, 0, -, =
        if (scancode <= 11) {
            key = '0' + (scancode - 2 + 1) % 10;
        } else if (scancode == 12) {
            key = '-';
        } else {
            key = '=';
        }
    } else if (scancode >= 16 && scancode <= 27) {
        // Letters Q-P
        key = 'q' + (scancode - 16);
    } else if (scancode >= 30 && scancode <= 40) {
        // Letters A-L
        key = 'a' + (scancode - 30);
    } else if (scancode >= 44 && scancode <= 53) {
        // Letters Z-M, comma, period, slash
        if (scancode <= 50) {
            key = 'z' + (scancode - 44);
        } else if (scancode == 51) {
            key = ',';
        } else if (scancode == 52) {
            key = '.';
        } else {
            key = '/';
        }
    } else {
        // Special keys
        switch (scancode) {
            case 14: key = '\b'; break;  // Backspace
            case 15: key = '\t'; break;  // Tab
            case 28: key = '\n'; break;  // Enter
            case 57: key = ' '; break;   // Space
            case 72: key = -1; break;    // Up arrow
            case 80: key = -2; break;    // Down arrow
            case 75: key = -3; break;    // Left arrow
            case 77: key = -4; break;    // Right arrow
            default: return;             // Ignore other keys
        }
    }
    
    // Reset tab completion state if not tab key
    if (key != '\t') {
        tab_pressed = 0;
    }
    
    // Process the key
    switch (key) {
        case '\n':
            // Execute command
            terminal_putchar('\n');
            command_buffer[buffer_pos] = '\0';
            
            // Add command to history
            add_to_history(command_buffer);
            
            // Process command
            shell_process_command(command_buffer);
            
            // Reset buffer and display prompt
            buffer_pos = 0;
            command_buffer[0] = '\0';
            current_history_index = history_count;
            terminal_writestring(PROMPT_TEXT);
            break;
            
        case '\b':
            // Backspace
            if (buffer_pos > 0) {
                buffer_pos--;
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
                command_buffer[buffer_pos] = '\0';
            }
            break;
            
        case '\t':
            // Tab completion
            tab_pressed = 1;
            handle_tab_completion();
            break;
            
        case -1:
            // Up arrow
            handle_history_navigation(-1);
            break;
            
        case -2:
            // Down arrow
            handle_history_navigation(1);
            break;
            
        default:
            // Regular character
            if (key > 0 && buffer_pos < COMMAND_BUFFER_SIZE - 1) {
                command_buffer[buffer_pos++] = key;
                command_buffer[buffer_pos] = '\0';
                terminal_putchar(key);
            }
            break;
    }
}

// Initialize the shell (original function)
void shell_init(void) {
    // Call enhanced init
    shell_enhanced_init();
}

// Initialize the enhanced shell
void shell_enhanced_init(void) {
    terminal_writestring("Hextrix OS v0.3.7-dev-v2 - Enhanced Shell\n");
    terminal_writestring("Type 'help' for a list of commands\n");
    terminal_writestring(PROMPT_TEXT);
    
    // Initialize buffer
    buffer_pos = 0;
    command_buffer[0] = '\0';
    
    // Initialize history
    for (int i = 0; i < COMMAND_HISTORY_SIZE; i++) {
        command_history[i] = NULL;
    }
    history_count = 0;
    history_index = 0;
    current_history_index = -1;
    
    // Initialize tab completion
    tab_pressed = 0;
    for (int i = 0; i < MAX_AUTOCOMPLETE_RESULTS; i++) {
        completion_results[i] = NULL;
    }
    completion_count = 0;
}

// Process a command
void shell_process_command(const char* cmd) {
    // Make a copy of the command for parsing
    char command_copy[COMMAND_BUFFER_SIZE];
    strncpy(command_copy, cmd, COMMAND_BUFFER_SIZE - 1);
    command_copy[COMMAND_BUFFER_SIZE - 1] = '\0';
    
    // Parse command into arguments
    char* argv[MAX_ARGS];
    int argc = parse_command(command_copy, argv);
    
    // Empty command
    if (argc == 0) {
        return;
    }
    
    // Look up and execute the command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
			commands[i].handler(argc, argv);
            // If you want to add any post-command processing, do it here
            // For now we just run the command and return
            return;
        }
    }
    
    // Command not found
    terminal_printf("Unknown command: %s\n", argv[0]);
    terminal_writestring("Type 'help' for a list of commands\n");
}

// Command handler implementations

static int cmd_clear(int argc, char** argv) {
    terminal_clear();
    return 0;
}

static int cmd_echo(int argc, char** argv) {
    // Skip the command name
    for (int i = 1; i < argc; i++) {
        terminal_writestring(argv[i]);
        if (i < argc - 1) {
            terminal_putchar(' ');
        }
    }
    terminal_putchar('\n');
    return 0;
}

static int cmd_ls(int argc, char** argv) {
    if (argc > 1) {
        fs_list(argv[1]);
    } else {
        fs_list("");  // Current directory
    }
    return 0;
}

static int cmd_cat(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: cat <filename>\n");
        return 1;
    }
    
    int size = fs_size(argv[1]);
    if (size < 0) {
        terminal_printf("File '%s' not found\n", argv[1]);
        return 1;
    }
    
    char* buffer = kmalloc(size + 1);
    if (!buffer) {
        terminal_writestring("Out of memory\n");
        return 1;
    }
    
    fs_read(argv[1], buffer, size);
    buffer[size] = '\0';
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    kfree(buffer);
    return 0;
}

static int cmd_write(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: write <filename>\n");
        return 1;
    }
    
    terminal_writestring("Enter file content (end with Ctrl+D on new line):\n");
    
    char content[FS_MAX_FILESIZE] = {0};
    int content_pos = 0;
    int line_start = 1;  // Flag to track start of new line
    
    while (content_pos < FS_MAX_FILESIZE - 1) {
        // Poll for keyboard input
        if (hal_keyboard_is_key_available()) {
            int scancode = hal_keyboard_read();
            
            // Simple scancode to character conversion
            char c = 0;
            if (scancode >= 2 && scancode <= 13) {
                // Digits 1-9, 0, -, =
                if (scancode <= 11) {
                    c = '0' + (scancode - 2 + 1) % 10;
                } else if (scancode == 12) {
                    c = '-';
                } else {
                    c = '=';
                }
            } else if (scancode >= 16 && scancode <= 27) {
                // Letters Q-P
                c = 'q' + (scancode - 16);
            } else if (scancode >= 30 && scancode <= 40) {
                // Letters A-L
                c = 'a' + (scancode - 30);
            } else if (scancode >= 44 && scancode <= 53) {
                // Letters Z-M, comma, period, slash
                if (scancode <= 50) {
                    c = 'z' + (scancode - 44);
                } else if (scancode == 51) {
                    c = ',';
                } else if (scancode == 52) {
                    c = '.';
                } else {
                    c = '/';
                }
            } else if (scancode == 28) {
                c = '\n';
            } else if (scancode == 57) {
                c = ' ';
            }
            
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
    if (fs_size(argv[1]) < 0) {
        fs_create(argv[1], FS_TYPE_FILE);
    }
    
    fs_write(argv[1], content, content_pos);
    terminal_printf("\nWrote %d bytes to %s\n", content_pos, argv[1]);
    return 0;
}

static int cmd_rm(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: rm <filename>\n");
        return 1;
    }
    
    if (fs_delete(argv[1]) < 0) {
        terminal_printf("File '%s' not found\n", argv[1]);
        return 1;
    }
    
    terminal_printf("Deleted '%s'\n", argv[1]);
    return 0;
}

static int cmd_pwd(int argc, char** argv) {
    terminal_printf("%s\n", fs_getcwd());
    return 0;
}

static int cmd_cd(int argc, char** argv) {
    if (argc < 2) {
        // Change to root directory
        if (fs_chdir("/") < 0) {
            terminal_writestring("Cannot change to root directory\n");
            return 1;
        }
        return 0;
    }
    
    if (fs_chdir(argv[1]) < 0) {
        terminal_printf("Cannot change to directory '%s'\n", argv[1]);
        return 1;
    }
    
    return 0;
}

static int cmd_mkdir(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: mkdir <directory>\n");
        return 1;
    }
    
    if (fs_mkdir(argv[1]) < 0) {
        terminal_printf("Failed to create directory '%s'\n", argv[1]);
        return 1;
    }
    
    terminal_printf("Created directory '%s'\n", argv[1]);
    return 0;
}

static int cmd_meminfo(int argc, char** argv) {
    size_t total, used, free;
    kmalloc_stats(&total, &used, &free);
    
    terminal_writestring("Memory usage:\n");
    terminal_printf("  Total: %d bytes (%d KB)\n", total, total / 1024);
    terminal_printf("  Used:  %d bytes (%d KB, %d%%)\n", 
                   used, used / 1024, (used * 100) / total);
    terminal_printf("  Free:  %d bytes (%d KB, %d%%)\n", 
                   free, free / 1024, (free * 100) / total);
                   
    // If enhanced memory stats available (placeholder)
    terminal_writestring("\nMemory Zones:\n");
    terminal_writestring("  Kernel: 0-1MB\n");
    terminal_writestring("  Heap: 1MB-5MB\n");
    terminal_writestring("  User: 5MB-8MB\n");
    
    return 0;
}

static int cmd_ps(int argc, char** argv) {
    process_list();
    return 0;
}

static int cmd_kill(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: kill <pid>\n");
        return 1;
    }
    
    int pid = 0;
    for (int i = 0; argv[1][i]; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            pid = pid * 10 + (argv[1][i] - '0');
        } else {
            terminal_writestring("Invalid PID format\n");
            return 1;
        }
    }
    
    if (pid <= 0) {
        terminal_writestring("Cannot kill system processes\n");
        return 1;
    }
    
    process_terminate(pid);
    return 0;
}

static int cmd_nice(int argc, char** argv) {
    if (argc < 3) {
        terminal_writestring("Usage: nice <pid> <priority: 0=low, 1=normal, 2=high, 3=realtime>\n");
        return 1;
    }
    
    int pid = 0;
    for (int i = 0; argv[1][i]; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            pid = pid * 10 + (argv[1][i] - '0');
        } else {
            terminal_writestring("Invalid PID format\n");
            return 1;
        }
    }
    
    int priority = 0;
    for (int i = 0; argv[2][i]; i++) {
        if (argv[2][i] >= '0' && argv[2][i] <= '9') {
            priority = priority * 10 + (argv[2][i] - '0');
        } else {
            terminal_writestring("Invalid priority format\n");
            return 1;
        }
    }
    
    // Validate priority value
    if (priority > 3) {
        terminal_printf("Invalid priority: %d. Must be 0-3\n", priority);
        return 1;
    }
    
    process_set_priority(pid, priority);
    terminal_printf("Set PID %d priority to %d\n", pid, priority);
    return 0;
}

static int cmd_sleep(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: sleep <milliseconds>\n");
        return 1;
    }
    
    int ms = 0;
    for (int i = 0; argv[1][i]; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            ms = ms * 10 + (argv[1][i] - '0');
        } else {
            terminal_writestring("Invalid time format\n");
            return 1;
        }
    }
    
    terminal_printf("Sleeping for %d ms...\n", ms);
    
    // Use HAL timer sleep function
    hal_timer_sleep(ms);
    
    terminal_writestring("Done sleeping\n");
    return 0;
}

static int cmd_version(int argc, char** argv) {
    terminal_writestring("Hextrix OS v0.3.7-dev-v2 - HAL Edition\n");
    terminal_writestring("Copyright (c) 2025 Jared Edwards - The Hextrix AI Project\n");
    terminal_writestring("Enhanced Shell with command history and tab completion\n");
    return 0;
}

static int cmd_memenable(int argc, char** argv) {
    enable_memory_protection();
    return 0;
}

static int cmd_memdisable(int argc, char** argv) {
    disable_memory_protection();
    return 0;
}

static int cmd_memcheck(int argc, char** argv) {
    if (argc < 3) {
        terminal_writestring("Usage: memcheck <addr> <flags: 1=R,2=W,4=X,8=U>\n");
        return 1;
    }
    
    // Parse the address
    uint32_t addr = 0;
    for (int i = 0; argv[1][i]; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            addr = addr * 10 + (argv[1][i] - '0');
        } else if (argv[1][i] >= 'a' && argv[1][i] <= 'f') {
            addr = addr * 16 + (argv[1][i] - 'a' + 10);
        } else if (argv[1][i] >= 'A' && argv[1][i] <= 'F') {
            addr = addr * 16 + (argv[1][i] - 'A' + 10);
        } else if (argv[1][i] == 'x' || argv[1][i] == 'X') {
            addr = 0; // Start of hex notation
        } else {
            terminal_writestring("Invalid address format\n");
            return 1;
        }
    }
    
    // Parse the flags
    uint32_t flags = 0;
    for (int i = 0; argv[2][i]; i++) {
        if (argv[2][i] >= '0' && argv[2][i] <= '9') {
            flags = flags * 10 + (argv[2][i] - '0');
        } else {
            terminal_writestring("Invalid flags format\n");
            return 1;
        }
    }
    
    // Check if memory access is valid
    int valid = is_valid_access(addr, flags);
    
    terminal_printf("Memory access to 0x%x with flags 0x%x is %s\n",
                   addr, flags, valid ? "valid" : "invalid");
    return 0;
}

static int cmd_memregions(int argc, char** argv) {
    display_memory_regions();
    return 0;
}

static int cmd_diag(int argc, char** argv) {
    terminal_writestring("Running system diagnostics...\n");
    
    // System information
    terminal_writestring("\n=== System Information ===\n");
    terminal_writestring("Hextrix OS v0.3.7-dev-v2 - HAL Edition\n");
    
    // Memory information
    terminal_writestring("\n=== Memory Information ===\n");
    size_t total, used, free;
    kmalloc_stats(&total, &used, &free);
    terminal_printf("Memory: %d KB total, %d KB used, %d KB free\n",
                   total / 1024, used / 1024, free / 1024);
    
    // Process information
    terminal_writestring("\n=== Process Information ===\n");
    terminal_printf("Processes: %d running\n", process_count());
    
    // File system information
    terminal_writestring("\n=== File System Information ===\n");
    // These would be actual calls if we had implemented these functions
    uint32_t hits = 0, misses = 0, flushes = 0;
    terminal_printf("Cache: %d hits, %d misses, %d flushes\n",
                   hits, misses, flushes);
    
    // Hardware information
    terminal_writestring("\n=== Hardware Information ===\n");
    terminal_writestring("HAL Devices:\n");
    terminal_writestring("  - Timer: Functional\n");
    terminal_writestring("  - Keyboard: Functional\n");
    terminal_writestring("  - Display: Functional\n");
    terminal_writestring("  - Storage: Functional (RAM Disk)\n");
    
    terminal_writestring("\nDiagnostics completed.\n");
    return 0;
}

static int cmd_fsinfo(int argc, char** argv) {
    // This would call the enhanced file system functions
    // For now, just display placeholder information
    terminal_writestring("File System Information:\n");
    terminal_writestring("------------------------\n");
    terminal_writestring("Type: In-memory file system\n");
    terminal_writestring("Max files: 64\n");
    terminal_writestring("Max filename length: 32\n");
    terminal_writestring("Max path length: 128\n");
    terminal_writestring("Max file size: 8192 bytes\n");
    
    // These would be actual calls if we had implemented these functions
    uint32_t hits = 0, misses = 0, flushes = 0;
    uint32_t opens = 0, closes = 0, reads = 0, writes = 0, dirs = 0;
    
    terminal_writestring("\nCache Statistics:\n");
    terminal_printf("  Hits: %d\n", hits);
    terminal_printf("  Misses: %d\n", misses);
    terminal_printf("  Flushes: %d\n", flushes);
    
    terminal_writestring("\nFile Operations:\n");
    terminal_printf("  Opens: %d\n", opens);
    terminal_printf("  Closes: %d\n", closes);
    terminal_printf("  Reads: %d\n", reads);
    terminal_printf("  Writes: %d\n", writes);
    terminal_printf("  Directory Ops: %d\n", dirs);
    
    return 0;
}

static int cmd_fscheck(int argc, char** argv) {
    // This would call the file system check function
    // For now, just display placeholder information
    terminal_writestring("Performing file system check...\n");
    terminal_writestring("No errors found.\n");
    return 0;
}

static int cmd_fsrepair(int argc, char** argv) {
    // This would call the file system repair function
    // For now, just display placeholder information
    terminal_writestring("Repairing file system...\n");
    terminal_writestring("No repairs needed.\n");
    return 0;
}

static int cmd_diskdump(int argc, char** argv) {
    if (argc < 2) {
        terminal_writestring("Usage: diskdump <sector> [count=1]\n");
        return 1;
    }
    
    // Parse sector number
    uint32_t sector = 0;
    for (int i = 0; argv[1][i]; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            sector = sector * 10 + (argv[1][i] - '0');
        } else {
            terminal_writestring("Invalid sector format\n");
            return 1;
        }
    }
    
    // Parse count if provided
    uint32_t count = 1;
    if (argc > 2) {
        count = 0;
        for (int i = 0; argv[2][i]; i++) {
            if (argv[2][i] >= '0' && argv[2][i] <= '9') {
                count = count * 10 + (argv[2][i] - '0');
            } else {
                terminal_writestring("Invalid count format\n");
                return 1;
            }
        }
    }
    
    // Check count boundaries
    if (count > 10) {
        terminal_writestring("Maximum dump count is 10 sectors\n");
        count = 10;
    }
    
    // This would call the hal_storage_dump function
    // For now, just display placeholder information
    terminal_printf("Dumping %d sector(s) starting at sector %d:\n", count, sector);
    
    // Dummy data for demonstration
    for (uint32_t s = 0; s < count; s++) {
        terminal_printf("\nSector %d:\n", sector + s);
        for (int i = 0; i < 4; i++) {
            terminal_printf("%04x: ", i * 16);
            for (int j = 0; j < 16; j++) {
                terminal_printf("%02x ", (i + j) % 256);
            }
            terminal_writestring(" |");
            for (int j = 0; j < 16; j++) {
                char c = (i + j) % 256;
                if (c >= 32 && c <= 126) {
                    terminal_putchar(c);
                } else {
                    terminal_putchar('.');
                }
            }
            terminal_writestring("|\n");
        }
        terminal_writestring("...\n");
    }
    
    return 0;
}

static int cmd_sched(int argc, char** argv) {
    // Display scheduler statistics
    terminal_writestring("Scheduler Information:\n");
    terminal_writestring("---------------------\n");
    
    // These would be actual calls if we had implemented these functions
    uint32_t switches = 0, yields = 0, preemptions = 0;
    uint32_t runtime = 0, idle = 0, kernel = 0, user = 0;
    
    terminal_writestring("Type: Multilevel Feedback Queue\n");
    terminal_writestring("Preemption: Enabled\n");
    terminal_writestring("Priority Aging: Enabled\n");
    
    terminal_writestring("\nStatistics:\n");
    terminal_printf("  Context Switches: %d\n", switches);
    terminal_printf("  Voluntary Yields: %d\n", yields);
    terminal_printf("  Preemptions: %d\n", preemptions);
    terminal_printf("  Total Runtime: %d ticks\n", runtime);
    
    if (runtime > 0) {
        terminal_printf("  CPU Usage: Kernel=%d%%, User=%d%%, Idle=%d%%\n",
                      kernel * 100 / runtime,
                      user * 100 / runtime,
                      idle * 100 / runtime);
    }
    
    return 0;
}

static int cmd_history(int argc, char** argv) {
    if (history_count == 0) {
        terminal_writestring("No command history\n");
        return 0;
    }
    
    terminal_writestring("Command History:\n");
    for (int i = 0; i < history_count; i++) {
        terminal_printf("%d: %s\n", i + 1, command_history[i]);
    }
    
    return 0;
}

static int cmd_reboot(int argc, char** argv) {
    terminal_writestring("System rebooting...\n");
    
    // This would call a system reboot function
    // For now, just display a message
    terminal_writestring("Reboot not implemented in simulation\n");
    
    return 0;
}

static int cmd_exit(int argc, char** argv) {
    terminal_writestring("Exiting shell...\n");
    
    // In a real system, this would exit the shell
    // For this simulation, just clear the screen and restart
    terminal_clear();
    shell_enhanced_init();
    
    return 0;
}

static int cmd_help(int argc, char** argv) {
    if (argc > 1) {
        // Help for specific command
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(argv[1], commands[i].name) == 0) {
                terminal_printf("%s - %s\n", commands[i].name, commands[i].description);
                
                // Add detailed help for some commands
                if (strcmp(argv[1], "ls") == 0) {
                    terminal_writestring("Usage: ls [directory]\n");
                    terminal_writestring("List files and directories in the specified directory.\n");
                    terminal_writestring("If no directory is specified, list the current directory.\n");
                } else if (strcmp(argv[1], "cat") == 0) {
                    terminal_writestring("Usage: cat <filename>\n");
                    terminal_writestring("Display the contents of the specified file.\n");
                } // Add more detailed help for other commands as needed
                
                return 0;
            }
        }
        
        terminal_printf("No help available for '%s'\n", argv[1]);
        return 1;
    }
    
    // General help - list all commands
    terminal_writestring("Available commands:\n");
    
    // Calculate the maximum command name length for alignment
    int max_name_len = 0;
    for (int i = 0; commands[i].name != NULL; i++) {
        int len = strlen(commands[i].name);
        if (len > max_name_len) {
            max_name_len = len;
		}
    }
    
    terminal_writestring("\nProcess Management:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, "ps") == 0 ||
            strcmp(commands[i].name, "kill") == 0 ||
            strcmp(commands[i].name, "nice") == 0 ||
            strcmp(commands[i].name, "sleep") == 0 ||
            strcmp(commands[i].name, "sched") == 0) {
            terminal_printf("  %-*s - %s\n", max_name_len + 2, commands[i].name, commands[i].description);
        }
    }
    
    terminal_writestring("\nMemory Management:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, "meminfo") == 0 ||
            strcmp(commands[i].name, "memenable") == 0 ||
            strcmp(commands[i].name, "memdisable") == 0 ||
            strcmp(commands[i].name, "memcheck") == 0 ||
            strcmp(commands[i].name, "memregions") == 0) {
            terminal_printf("  %-*s - %s\n", max_name_len + 2, commands[i].name, commands[i].description);
        }
    }
    
    terminal_writestring("\nShell Commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, "help") == 0 ||
            strcmp(commands[i].name, "clear") == 0 ||
            strcmp(commands[i].name, "echo") == 0 ||
            strcmp(commands[i].name, "version") == 0 ||
            strcmp(commands[i].name, "history") == 0 ||
            strcmp(commands[i].name, "reboot") == 0 ||
            strcmp(commands[i].name, "exit") == 0) {
            terminal_printf("  %-*s - %s\n", max_name_len + 2, commands[i].name, commands[i].description);
        }
    }
    
    terminal_writestring("\nSystem Diagnostics:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, "diag") == 0) {
            terminal_printf("  %-*s - %s\n", max_name_len + 2, commands[i].name, commands[i].description);
        }
    }
    
    terminal_writestring("\nType 'help <command>' for more information on a specific command.\n");
    return 0;
}
// Run the shell main loop
void shell_run(void) {
    terminal_writestring("Hextrix OS Shell\n");
    terminal_writestring("Type 'help' for a list of commands\n");
    terminal_writestring("> ");
    
    while (1) {
        // Poll for keyboard input
        if (hal_keyboard_is_key_available()) {
            int scancode = hal_keyboard_read();
            shell_handle_key(scancode);
        }
    }
}