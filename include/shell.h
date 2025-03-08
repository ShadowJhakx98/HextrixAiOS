// include/shell.h
#ifndef SHELL_H
#define SHELL_H

// Initialize shell
void shell_init(void);

// Run the shell (polling-based)
void shell_run(void);

// Process a single command
void shell_process_command(const char* command);

#endif