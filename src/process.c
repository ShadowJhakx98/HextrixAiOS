/// src/process.c
#include "process.h"
#include "kmalloc.h"
#include "string.h"
#include "stdio.h"
#include "terminal.h"
#include "scheduler.h"
#include "interrupts.h"  // Include this for timer_ticks

// Process table
static process_t process_table[MAX_PROCESSES];

// Next available PID
static uint32_t next_pid = 1;

// The currently running process
static process_t* current_process = NULL;

// Initialize the process management subsystem
void process_init(void) {
    // Clear the process table
    memset(process_table, 0, sizeof(process_table));
    
    // Initialize the idle process (PID 0)
    process_table[0].pid = 0;
    strcpy(process_table[0].name, "idle");
    process_table[0].state = PROCESS_STATE_READY;
    process_table[0].priority = PROCESS_PRIORITY_LOW;
    process_table[0].time_slice = 1;
    process_table[0].ticks_remaining = 1;
    process_table[0].total_runtime = 0;
    process_table[0].entry_point = NULL;  // Idle process just returns to scheduler
    process_table[0].cpu_usage_percent = 0;
    process_table[0].parent_pid = 0;
    
    // Allocate stack for idle process
    process_table[0].stack = kmalloc(PROCESS_STACK_SIZE);
    if (!process_table[0].stack) {
        terminal_writestring("Failed to allocate stack for idle process\n");
        return;
    }
    
    // Set up context for idle process
    process_table[0].context.esp = (uint32_t)process_table[0].stack + PROCESS_STACK_SIZE - 4;
    process_table[0].context.eip = (uint32_t)process_table[0].entry_point;
    process_table[0].context.eflags = 0x202;  // Interrupts enabled
    
    // Set current process to idle
    current_process = &process_table[0];
    
    terminal_writestring("Process management initialized\n");
}

// Save the context of the current process
static void process_save_context(process_context_t* context) {
    // Since this is just for demonstration and is not actually used in our system yet
    // (we'll use context_switch.asm instead), we'll just comment it out for now
    // to avoid compilation errors
    
    /*
    // This would be done in assembly in a real implementation
    asm volatile (
        "mov %%eax, %0\n"
        : "=m"(context->eax)
        :
        : "memory"
    );
    */
    
    // Actual context saving will be implemented in context_switch.asm
}

// Restore the context of a process
static void process_restore_context(process_context_t* context) {
    // Since this is just for demonstration and is not actually used in our system yet
    // (we'll use context_switch.asm instead), we'll just comment it out for now
    // to avoid compilation errors
    
    /*
    // This would be done in assembly in a real implementation
    asm volatile (
        "mov %0, %%eax\n"
        : : "m"(context->eax) : "eax"
    );
    */
    
    // Actual context restoration will be implemented in context_switch.asm
}

// Find a free slot in the process table
static int process_find_free_slot(void) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_TERMINATED || 
            process_table[i].pid == 0) {
            return i;
        }
    }
    return -1;  // No free slots
}

// Create a new process
int process_create(const char* name, void (*entry_point)(void), uint8_t priority) {
    int slot = process_find_free_slot();
    if (slot < 0) {
        terminal_writestring("Error: Maximum process limit reached\n");
        return -1;
    }
    
    process_t* proc = &process_table[slot];
    
    // Initialize the process
    proc->pid = next_pid++;
    strncpy(proc->name, name, sizeof(proc->name) - 1);
    proc->name[sizeof(proc->name) - 1] = '\0';
    proc->state = PROCESS_STATE_READY;
    proc->priority = priority;
    proc->parent_pid = 0;  // Default parent is system
    proc->cpu_usage_percent = 0;
    proc->exit_code = 0;
    
    // Set time slice based on priority
    switch (priority) {
        case PROCESS_PRIORITY_LOW:
            proc->time_slice = 1;
            break;
        case PROCESS_PRIORITY_NORMAL:
            proc->time_slice = 2;
            break;
        case PROCESS_PRIORITY_HIGH:
            proc->time_slice = 4;
            break;
        case PROCESS_PRIORITY_REALTIME:
            proc->time_slice = 8;
            break;
        default:
            proc->time_slice = 2;
            break;
    }
    
    proc->ticks_remaining = proc->time_slice;
    proc->total_runtime = 0;
    proc->entry_point = entry_point;
    
    // Allocate stack for the process
    proc->stack = kmalloc(PROCESS_STACK_SIZE);
    if (!proc->stack) {
        terminal_writestring("Error: Failed to allocate stack for process\n");
        return -1;
    }
    
    // Set up initial context
    memset(&proc->context, 0, sizeof(process_context_t));
    proc->context.esp = (uint32_t)proc->stack + PROCESS_STACK_SIZE - 4;
    proc->context.eip = (uint32_t)entry_point;
    proc->context.eflags = 0x202;  // Interrupts enabled
    
    // Add process to scheduler
    scheduler_add_process(proc);
    
    terminal_printf("Created process '%s' with PID %d\n", name, proc->pid);
    return proc->pid;
}

// Create a new process with a specified parent
int process_create_with_parent(const char* name, void (*entry_point)(void), uint8_t priority, uint32_t parent_pid) {
    int pid = process_create(name, entry_point, priority);
    if (pid > 0) {
        process_t* proc = process_get_by_pid(pid);
        if (proc) {
            proc->parent_pid = parent_pid;
        }
    }
    return pid;
}

// Terminate the specified process
void process_terminate(uint32_t pid) {
    // Can't terminate idle process
    if (pid == 0) {
        return;
    }
    
    // Find the process
    process_t* proc = process_get_by_pid(pid);
    if (!proc) {
        terminal_printf("Error: Process with PID %d not found\n", pid);
        return;
    }
    
    // Update state
    proc->state = PROCESS_STATE_TERMINATED;
    
    // Free resources
    if (proc->stack) {
        kfree(proc->stack);
        proc->stack = NULL;
    }
    
    // Remove from scheduler
    scheduler_remove_process(pid);
    
    terminal_printf("Terminated process '%s' with PID %d\n", proc->name, proc->pid);
    
    // If we terminated the current process, yield to scheduler
    if (proc == current_process) {
        scheduler_yield();
    }
}

// Block the specified process
void process_block(uint32_t pid) {
    // Can't block idle process
    if (pid == 0) {
        return;
    }
    
    process_t* proc = process_get_by_pid(pid);
    if (!proc) {
        return;
    }
    
    proc->state = PROCESS_STATE_BLOCKED;
    
    // If we blocked the current process, yield to scheduler
    if (proc == current_process) {
        scheduler_yield();
    }
}

// Unblock the specified process
void process_unblock(uint32_t pid) {
    process_t* proc = process_get_by_pid(pid);
    if (!proc || proc->state != PROCESS_STATE_BLOCKED) {
        return;
    }
    
    proc->state = PROCESS_STATE_READY;
}

// Put a process to sleep for a specified number of milliseconds
void process_sleep(uint32_t pid, uint32_t ms) {
    process_t* proc = process_get_by_pid(pid);
    if (!proc) {
        return;
    }
    
    // Convert ms to timer ticks (assuming 100Hz timer)
    uint32_t ticks = (ms / 10) + 1;
    
    proc->state = PROCESS_STATE_SLEEPING;
    proc->sleep_until = timer_ticks + ticks;
    
    // If we're sleeping the current process, yield
    if (proc == current_process) {
        scheduler_yield();
    }
}

// Update process priority
void process_set_priority(uint32_t pid, uint8_t priority) {
    process_t* proc = process_get_by_pid(pid);
    if (!proc) {
        return;
    }
    
    if (priority > PROCESS_PRIORITY_REALTIME) {
        priority = PROCESS_PRIORITY_REALTIME;
    }
    
    proc->priority = priority;
    
    // Update time slice based on new priority
    switch (priority) {
        case PROCESS_PRIORITY_LOW:
            proc->time_slice = 1;
            break;
        case PROCESS_PRIORITY_NORMAL:
            proc->time_slice = 2;
            break;
        case PROCESS_PRIORITY_HIGH:
            proc->time_slice = 4;
            break;
        case PROCESS_PRIORITY_REALTIME:
            proc->time_slice = 8;
            break;
        default:
            proc->time_slice = 2;
            break;
    }
}

// Get the current running process
process_t* process_get_current(void) {
    return current_process;
}

// Set the current running process
void process_set_current(process_t* proc) {
    current_process = proc;
}

// Get a process by PID
process_t* process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_STATE_TERMINATED) {
            return &process_table[i];
        }
    }
    return NULL;
}

// List all processes
void process_list(void) {
    terminal_writestring("PID  Name                 State    Pri  CPU%  Runtime Parent\n");
    terminal_writestring("---- -------------------- -------- --- ----- ------- ------\n");
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != 0 || i == 0) {  // Always show idle process
            const char* state_str = "Unknown";
            
            switch (process_table[i].state) {
                case PROCESS_STATE_READY:
                    state_str = "Ready   ";
                    break;
                case PROCESS_STATE_RUNNING:
                    state_str = "Running ";
                    break;
                case PROCESS_STATE_BLOCKED:
                    state_str = "Blocked ";
                    break;
                case PROCESS_STATE_TERMINATED:
                    state_str = "Terminat";
                    break;
                case PROCESS_STATE_SLEEPING:
                    state_str = "Sleeping";
                    break;
            }
            
            char priority_str[2] = {'-', '\0'};
            switch (process_table[i].priority) {
                case PROCESS_PRIORITY_LOW:
                    priority_str[0] = 'L';
                    break;
                case PROCESS_PRIORITY_NORMAL:
                    priority_str[0] = 'N';
                    break;
                case PROCESS_PRIORITY_HIGH:
                    priority_str[0] = 'H';
                    break;
                case PROCESS_PRIORITY_REALTIME:
                    priority_str[0] = 'R';
                    break;
            }
            
            // Current process indicator
            char is_current = ' ';
            if (&process_table[i] == current_process) {
                is_current = '*';
            }
            
            terminal_printf("%-4d%c%-20s %-8s %-3s %3d%%  %7d %6d\n",
                process_table[i].pid,
                is_current,
                process_table[i].name,
                state_str,
                priority_str,
                process_table[i].cpu_usage_percent,
                process_table[i].total_runtime,
                process_table[i].parent_pid);
        }
    }
}

// Update CPU usage statistics for all processes
void process_update_cpu_stats(void) {
    // Simple implementation: just the percentage of total ticks
    uint32_t total_ticks = 0;
    
    // Calculate total ticks
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != 0 || i == 0) {
            total_ticks += process_table[i].total_runtime;
        }
    }
    
    // Avoid division by zero
    if (total_ticks == 0) {
        total_ticks = 1;
    }
    
    // Update percentage for each process
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != 0 || i == 0) {
            process_table[i].cpu_usage_percent = 
                (process_table[i].total_runtime * 100) / total_ticks;
        }
    }
}

// Get count of active processes
uint32_t process_count(void) {
    uint32_t count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != 0 && 
            process_table[i].state != PROCESS_STATE_TERMINATED) {
            count++;
        }
    }
    return count;
}