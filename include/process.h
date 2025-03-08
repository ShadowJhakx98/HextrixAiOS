// include/process.h
#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

// Process states
#define PROCESS_STATE_READY      0
#define PROCESS_STATE_RUNNING    1
#define PROCESS_STATE_BLOCKED    2
#define PROCESS_STATE_TERMINATED 3
#define PROCESS_STATE_SLEEPING   4  // New: process is sleeping for a specific time

// Process priorities
#define PROCESS_PRIORITY_LOW     0
#define PROCESS_PRIORITY_NORMAL  1
#define PROCESS_PRIORITY_HIGH    2
#define PROCESS_PRIORITY_REALTIME 3  // New: highest priority

// Maximum number of processes in the system
#define MAX_PROCESSES 32

// Process stack size (16 KB)
#define PROCESS_STACK_SIZE 16384

// Process context structure
typedef struct {
    uint32_t eax, ebx, ecx, edx;     // General purpose registers
    uint32_t esi, edi, ebp;          // Additional registers
    uint32_t esp;                    // Stack pointer
    uint32_t eip;                    // Instruction pointer
    uint32_t eflags;                 // CPU flags
} process_context_t;

// Process Control Block (PCB)
typedef struct {
    uint32_t pid;                    // Process ID
    char name[32];                   // Process name
    uint8_t state;                   // Process state
    uint8_t priority;                // Process priority
    uint32_t time_slice;             // Time slice allocation in ticks
    uint32_t ticks_remaining;        // Remaining ticks in current time slice
    uint32_t total_runtime;          // Total runtime in ticks
    process_context_t context;       // CPU context
    uint8_t* stack;                  // Stack memory
    void (*entry_point)(void);       // Process entry point
    uint32_t sleep_until;            // Wake time for sleeping processes
    uint32_t cpu_usage_percent;      // CPU usage percentage
    uint32_t parent_pid;             // Parent process PID
    uint32_t exit_code;              // Process exit code
} process_t;

// Initialize the process management subsystem
void process_init(void);

// Create a new process
int process_create(const char* name, void (*entry_point)(void), uint8_t priority);

// Create a new process with a specified parent
int process_create_with_parent(const char* name, void (*entry_point)(void), uint8_t priority, uint32_t parent_pid);

// Terminate the specified process
void process_terminate(uint32_t pid);

// Block the specified process
void process_block(uint32_t pid);

// Unblock the specified process
void process_unblock(uint32_t pid);

// Put a process to sleep for a specified number of milliseconds
void process_sleep(uint32_t pid, uint32_t ms);

// Update process priorities
void process_set_priority(uint32_t pid, uint8_t priority);

// Get the current running process
process_t* process_get_current(void);

// Set the current running process
void process_set_current(process_t* proc);

// Get a process by PID
process_t* process_get_by_pid(uint32_t pid);

// List all processes
void process_list(void);

// Update process CPU usage statistics
void process_update_cpu_stats(void);

// Get the number of active processes
uint32_t process_count(void);

#endif // PROCESS_H