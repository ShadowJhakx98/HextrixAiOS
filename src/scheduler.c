// src/scheduler.c
#include "scheduler.h"
#include "process.h"
#include "terminal.h"
#include "stdio.h"
#include "interrupts.h"  // Include this for timer_ticks

// Array of process pointers for scheduling
static process_t* process_queue[MAX_PROCESSES];
static uint32_t scheduler_process_count_var = 0;

// Keeps track of whether a context switch is needed
static int context_switch_needed = 0;

// Static functions forward declarations
static void scheduler_context_switch(process_t* next);

// Initialize the scheduler
void scheduler_init(void) {
    // Clear the process queue
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_queue[i] = NULL;
    }
    
    scheduler_process_count_var = 0;
    
    // Add the idle process
    process_t* idle = process_get_by_pid(0);
    if (idle) {
        scheduler_add_process(idle);
    }
    
    terminal_writestring("Scheduler initialized\n");
}

// Add process to scheduler
void scheduler_add_process(process_t* process) {
    if (!process || scheduler_process_count_var >= MAX_PROCESSES) {
        return;
    }
    
    // Add to the queue
    process_queue[scheduler_process_count_var++] = process;
}

// Remove process from scheduler
void scheduler_remove_process(uint32_t pid) {
    // Find the process in the queue
    int index = -1;
    for (int i = 0; i < scheduler_process_count_var; i++) {
        if (process_queue[i]->pid == pid) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        return;  // Process not found
    }
    
    // Remove the process and shift others down
    for (int i = index; i < scheduler_process_count_var - 1; i++) {
        process_queue[i] = process_queue[i + 1];
    }
    
    process_queue[--scheduler_process_count_var] = NULL;
}

// Get the number of active processes in the scheduler
uint32_t scheduler_process_count(void) {
    return scheduler_process_count_var;
}

// Forward declaration of assembly function
extern void context_switch(process_t* next);

// Perform a context switch to the next process
static void scheduler_context_switch(process_t* next) {
    // Get the current process
    process_t* current = process_get_current();
    
    // If same process, nothing to do
    if (current == next) {
        return;
    }
    
    // Update states
    if (current) {
        if (current->state == PROCESS_STATE_RUNNING) {
            current->state = PROCESS_STATE_READY;
        }
    }
    
    next->state = PROCESS_STATE_RUNNING;
    
    // Reset the next process's time slice
    next->ticks_remaining = next->time_slice;
    
    // For this implementation, we'll just update the current process
    // without actually doing a context switch since we're still developing
    // the context switching mechanism
    process_set_current(next);
    
    // In a fully implemented system, we would call our assembly routine:
    // context_switch(next);
}

// Simple priority-based scheduling algorithm
static process_t* scheduler_pick_next(void) {
    // If no processes other than idle, return idle
    if (scheduler_process_count_var <= 1) {
        return process_queue[0];  // Idle process
    }
    
    // Start with highest priority (REALTIME) down to lowest (LOW)
    for (int priority = PROCESS_PRIORITY_REALTIME; priority >= PROCESS_PRIORITY_LOW; priority--) {
        // Find any READY process with this priority
        for (int i = 0; i < scheduler_process_count_var; i++) {
            if (process_queue[i]->state == PROCESS_STATE_READY && 
                process_queue[i]->priority == priority) {
                return process_queue[i];
            }
        }
    }
    
    // If no READY process found at any priority, return idle
    return process_queue[0];
}

// Schedule the next process to run
void scheduler_run_next(void) {
    process_t* next = scheduler_pick_next();
    if (!next) {
        return;
    }
    
    scheduler_context_switch(next);
}

// Called on each timer tick to update scheduler state
void scheduler_timer_tick(void) {
    process_t* current = process_get_current();
    if (!current) {
        return;
    }
    
    // Increment total runtime
    current->total_runtime++;
    
    // Update process CPU statistics periodically (every 100 ticks)
    static uint32_t stat_counter = 0;
    if (++stat_counter >= 100) {
        process_update_cpu_stats();
        stat_counter = 0;
    }
    
    // Check for sleeping processes that need to wake up
    for (int i = 0; i < scheduler_process_count_var; i++) {
        if (process_queue[i] && process_queue[i]->state == PROCESS_STATE_SLEEPING) {
            if (timer_ticks >= process_queue[i]->sleep_until) {
                process_queue[i]->state = PROCESS_STATE_READY;
            }
        }
    }
    
    // Don't decrement for idle process
    if (current->pid == 0) {
        // Always try to schedule a non-idle process if available
        scheduler_run_next();
        return;
    }
    
    // Decrement remaining time slice
    if (current->ticks_remaining > 0) {
        current->ticks_remaining--;
    }
    
    // If time slice expired, schedule next process
    if (current->ticks_remaining == 0) {
        context_switch_needed = 1;
    }
    
    // If a context switch is needed, do it now
    if (context_switch_needed) {
        context_switch_needed = 0;
        scheduler_run_next();
    }
}

// Yield the CPU to another process
void scheduler_yield(void) {
    context_switch_needed = 1;
    scheduler_timer_tick();  // Force a context switch
}