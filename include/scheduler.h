// include/scheduler.h
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <stdint.h>

// Initialize the scheduler
void scheduler_init(void);

// Schedule the next process to run
void scheduler_run_next(void);

// Called on each timer tick to update scheduler state
void scheduler_timer_tick(void);

// Yield the CPU to another process
void scheduler_yield(void);

// Add process to scheduler
void scheduler_add_process(process_t* process);

// Remove process from scheduler
void scheduler_remove_process(uint32_t pid);

// Get the number of active processes
uint32_t scheduler_process_count(void);

#endif // SCHEDULER_H