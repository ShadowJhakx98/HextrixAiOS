// src/scheduler_enhanced.c
#include "scheduler.h"
#include "process.h"
#include "terminal.h"
#include "stdio.h"
#include "hal.h"

// Scheduler types
#define SCHEDULER_TYPE_ROUND_ROBIN   0
#define SCHEDULER_TYPE_PRIORITY      1
#define SCHEDULER_TYPE_MULTILEVEL    2

// Scheduler queues for multilevel feedback
#define MAX_PRIORITY_QUEUES  4
#define QUEUE_HIGH           0
#define QUEUE_NORMAL         1
#define QUEUE_LOW            2
#define QUEUE_BACKGROUND     3

// Process arrays for each priority queue
static process_t* process_queues[MAX_PRIORITY_QUEUES][MAX_PROCESSES] = {0};
static uint32_t queue_counts[MAX_PRIORITY_QUEUES] = {0};

// Current scheduler configuration
static struct {
    uint8_t scheduler_type;       // Type of scheduling algorithm
    uint32_t time_slice_base;     // Base time slice in ms
    uint32_t time_slice_factor;   // Factor to multiply by priority
    uint32_t boost_interval;      // Interval for priority boosting in ticks
    uint32_t boost_countdown;     // Countdown to next boost
    uint8_t preemption_enabled;   // Whether preemption is enabled
    uint8_t priority_aging;       // Whether priority aging is enabled
    uint32_t idle_task_pid;       // PID of idle task
} scheduler_config;

// Statistics
static struct {
    uint32_t total_tasks_created;
    uint32_t total_tasks_completed;
    uint32_t context_switches;
    uint32_t voluntary_yields;
    uint32_t involuntary_preemptions;
    uint32_t total_runtime;       // Total ticks since boot
    uint32_t idle_time;           // Time spent in idle task
    uint32_t kernel_time;         // Time spent in kernel
    uint32_t user_time;           // Time spent in user tasks
} scheduler_stats;

// Map process priority to queue
static int priority_to_queue(uint8_t priority) {
    switch (priority) {
        case PROCESS_PRIORITY_REALTIME:   return QUEUE_HIGH;
        case PROCESS_PRIORITY_HIGH:       return QUEUE_HIGH;
        case PROCESS_PRIORITY_NORMAL:     return QUEUE_NORMAL;
        case PROCESS_PRIORITY_LOW:        return QUEUE_LOW;
        default:                          return QUEUE_LOW;
    }
}

// Get time slice for a process based on priority
static uint32_t get_time_slice(process_t* process) {
    if (!process) return 1;
    
    // Base time slice depends on priority
    uint32_t slice = scheduler_config.time_slice_base;
    
    switch (process->priority) {
        case PROCESS_PRIORITY_REALTIME:
            slice *= 4;
            break;
        case PROCESS_PRIORITY_HIGH:
            slice *= 2;
            break;
        case PROCESS_PRIORITY_NORMAL:
            // Default slice
            break;
        case PROCESS_PRIORITY_LOW:
            slice /= 2;
            break;
    }
    
    // Minimum time slice
    if (slice < 1) slice = 1;
    
    return slice;
}

// Priority boosting to prevent starvation
static void boost_priorities(void) {
    terminal_writestring("Boosting process priorities\n");
    
    // Move all processes up one queue level
    for (int q = MAX_PRIORITY_QUEUES - 1; q > 0; q--) {
        for (uint32_t i = 0; i < queue_counts[q]; i++) {
            process_t* proc = process_queues[q][i];
            if (proc && proc->state == PROCESS_STATE_READY) {
                // Boost priority by one level
                uint8_t new_priority;
                switch (proc->priority) {
                    case PROCESS_PRIORITY_LOW:
                        new_priority = PROCESS_PRIORITY_NORMAL;
                        break;
                    case PROCESS_PRIORITY_NORMAL:
                        new_priority = PROCESS_PRIORITY_HIGH;
                        break;
                    default:
                        new_priority = proc->priority;
                        break;
                }
                
                // Update process priority
                process_set_priority(proc->pid, new_priority);
                
                // Move to higher queue
                int new_queue = priority_to_queue(new_priority);
                
                // Remove from current queue
                for (uint32_t j = i; j < queue_counts[q] - 1; j++) {
                    process_queues[q][j] = process_queues[q][j + 1];
                }
                queue_counts[q]--;
                
                // Add to new queue
                process_queues[new_queue][queue_counts[new_queue]++] = proc;
                
                // Adjust index since we removed an item
                i--;
            }
        }
    }
    
    // Reset boost countdown
    scheduler_config.boost_countdown = scheduler_config.boost_interval;
}

// Add process to appropriate queue
void scheduler_add_process(process_t* process) {
    if (!process) return;
    
    // Determine which queue to add to
    int queue = priority_to_queue(process->priority);
    
    // Add to queue if not full
    if (queue_counts[queue] < MAX_PROCESSES) {
        process_queues[queue][queue_counts[queue]++] = process;
        
        // Set initial time slice
        process->time_slice = get_time_slice(process);
        process->ticks_remaining = process->time_slice;
        
        // Update statistics
        scheduler_stats.total_tasks_created++;
    }
}

// Remove process from scheduler queues
void scheduler_remove_process(uint32_t pid) {
    // Find the process in all queues
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        for (uint32_t i = 0; i < queue_counts[q]; i++) {
            if (process_queues[q][i] && process_queues[q][i]->pid == pid) {
                // Found the process, remove it
                for (uint32_t j = i; j < queue_counts[q] - 1; j++) {
                    process_queues[q][j] = process_queues[q][j + 1];
                }
                process_queues[q][--queue_counts[q]] = NULL;
                
                // Update statistics
                scheduler_stats.total_tasks_completed++;
                
                return;
            }
        }
    }
}

// Helper functions for picking next process to run
static process_t* pick_next_round_robin(void) {
    process_t* current = process_get_current();
    
    // Simple round-robin: just pick the next ready process in same queue
    if (current) {
        int queue = priority_to_queue(current->priority);
        uint32_t start_idx = 0;
        
        // Find current process in queue
        for (uint32_t i = 0; i < queue_counts[queue]; i++) {
            if (process_queues[queue][i] == current) {
                start_idx = (i + 1) % queue_counts[queue];
                break;
            }
        }
        
        // Search from next position
        for (uint32_t i = 0; i < queue_counts[queue]; i++) {
            uint32_t idx = (start_idx + i) % queue_counts[queue];
            if (process_queues[queue][idx] && 
                process_queues[queue][idx]->state == PROCESS_STATE_READY) {
                return process_queues[queue][idx];
            }
        }
    }
    
    // No suitable process in same queue, search all queues
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        if (queue_counts[q] > 0) {
            for (uint32_t i = 0; i < queue_counts[q]; i++) {
                if (process_queues[q][i] && 
                    process_queues[q][i]->state == PROCESS_STATE_READY) {
                    return process_queues[q][i];
                }
            }
        }
    }
    
    // No ready process, return idle task
    return process_get_by_pid(scheduler_config.idle_task_pid);
}

static process_t* pick_next_priority(void) {
    // Priority scheduling: pick highest priority ready process
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        if (queue_counts[q] > 0) {
            for (uint32_t i = 0; i < queue_counts[q]; i++) {
                if (process_queues[q][i] && 
                    process_queues[q][i]->state == PROCESS_STATE_READY) {
                    return process_queues[q][i];
                }
            }
        }
    }
    
    // No ready process, return idle task
    return process_get_by_pid(scheduler_config.idle_task_pid);
}

static process_t* pick_next_multilevel(void) {
    // Multilevel queue: pick process from highest priority non-empty queue
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        if (queue_counts[q] > 0) {
            // Round-robin within this queue
            process_t* current = process_get_current();
            int current_queue = -1;
            uint32_t start_idx = 0;
            
            // Find current process queue
            if (current) {
                current_queue = priority_to_queue(current->priority);
                
                // If current process is in this queue, find its index
                if (current_queue == q) {
                    for (uint32_t i = 0; i < queue_counts[q]; i++) {
                        if (process_queues[q][i] == current) {
                            start_idx = (i + 1) % queue_counts[q];
                            break;
                        }
                    }
                }
            }
            
            // Search from next position or beginning
            for (uint32_t i = 0; i < queue_counts[q]; i++) {
                uint32_t idx = (start_idx + i) % queue_counts[q];
                if (process_queues[q][idx] && 
                    process_queues[q][idx]->state == PROCESS_STATE_READY) {
                    return process_queues[q][idx];
                }
            }
        }
    }
    
    // No ready process, return idle task
    return process_get_by_pid(scheduler_config.idle_task_pid);
}

// Updated context switch implementation
static void scheduler_context_switch(process_t* next) {
    if (!next) return;
    
    // Get current process
    process_t* current = process_get_current();
    
    // If switching to same process, just reset time slice
    if (current == next) {
        next->ticks_remaining = next->time_slice;
        return;
    }
    
    // Update states
    if (current) {
        if (current->state == PROCESS_STATE_RUNNING) {
            current->state = PROCESS_STATE_READY;
        }
        
        // Update statistics based on process type
        if (current->pid == scheduler_config.idle_task_pid) {
            scheduler_stats.idle_time++;
        } else if (current->pid < 10) { // Assuming PIDs < 10 are kernel tasks
            scheduler_stats.kernel_time++;
        } else {
            scheduler_stats.user_time++;
        }
    }
    
    next->state = PROCESS_STATE_RUNNING;
    next->ticks_remaining = next->time_slice;
    
    // Update current process pointer
    process_set_current(next);
    
    // Update statistics
    scheduler_stats.context_switches++;
}

// Pick next process based on scheduling algorithm
static process_t* scheduler_pick_next(void) {
    process_t* next = NULL;
    
    // Use appropriate scheduling algorithm
    switch (scheduler_config.scheduler_type) {
        case SCHEDULER_TYPE_ROUND_ROBIN:
            next = pick_next_round_robin();
            break;
            
        case SCHEDULER_TYPE_PRIORITY:
            next = pick_next_priority();
            break;
            
        case SCHEDULER_TYPE_MULTILEVEL:
            next = pick_next_multilevel();
            break;
            
        default:
            // Default to simple priority scheduling
            next = pick_next_priority();
            break;
    }
    
    // If no process found, use idle task
    if (!next) {
        next = process_get_by_pid(scheduler_config.idle_task_pid);
    }
    
    return next;
}

// Schedule the next process to run
void scheduler_run_next(void) {
    process_t* next = scheduler_pick_next();
    if (next) {
        scheduler_context_switch(next);
    }
}

// Initialize the enhanced scheduler
void scheduler_enhanced_init(void) {
    terminal_writestring("Initializing enhanced scheduler\n");
    
    // Clear process queues
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
            process_queues[q][i] = NULL;
        }
        queue_counts[q] = 0;
    }
    
    // Set initial configuration
    scheduler_config.scheduler_type = SCHEDULER_TYPE_MULTILEVEL;
    scheduler_config.time_slice_base = 10;       // 10 ms base
    scheduler_config.time_slice_factor = 2;      // Double for each priority level
    scheduler_config.boost_interval = 1000;      // Every 1000 ticks (about 10 seconds)
    scheduler_config.boost_countdown = 1000;
    scheduler_config.preemption_enabled = 1;     // Enable preemption
    scheduler_config.priority_aging = 1;         // Enable aging to prevent starvation
    
    // Reset statistics
    scheduler_stats.total_tasks_created = 0;
    scheduler_stats.total_tasks_completed = 0;
    scheduler_stats.context_switches = 0;
    scheduler_stats.voluntary_yields = 0;
    scheduler_stats.involuntary_preemptions = 0;
    scheduler_stats.total_runtime = 0;
    scheduler_stats.idle_time = 0;
    scheduler_stats.kernel_time = 0;
    scheduler_stats.user_time = 0;
    
    // Add the idle process
    process_t* idle = process_get_by_pid(0);
    if (idle) {
        scheduler_config.idle_task_pid = 0;
        scheduler_add_process(idle);
    }
    
    terminal_writestring("Enhanced scheduler initialized with multilevel feedback queues\n");
}

// Updated timer tick handler
void scheduler_timer_tick(void) {
    // Update statistics
    scheduler_stats.total_runtime++;
    
    // Decrement priority boost countdown
    if (scheduler_config.priority_aging) {
        if (--scheduler_config.boost_countdown == 0) {
            boost_priorities();
        }
    }
    
    // Check for sleeping processes that need to wake up
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        for (uint32_t i = 0; i < queue_counts[q]; i++) {
            process_t* proc = process_queues[q][i];
            if (proc && proc->state == PROCESS_STATE_SLEEPING) {
                if (hal_timer_get_ticks() >= proc->sleep_until) {
                    proc->state = PROCESS_STATE_READY;
                }
            }
        }
    }
    
    // Get current process
    process_t* current = process_get_current();
    if (!current) return;
    
    // Update runtime statistics
    current->total_runtime++;
    
    // Skip time slice management for idle process
    if (current->pid == scheduler_config.idle_task_pid) {
        // Always try to find a non-idle process
        if (scheduler_config.preemption_enabled) {
            scheduler_run_next();
        }
        return;
    }
    
    // Decrement time slice
    if (current->ticks_remaining > 0) {
        current->ticks_remaining--;
    }
    
    // If time slice expired, schedule next process
    if (current->ticks_remaining == 0 && scheduler_config.preemption_enabled) {
        // If using multilevel feedback, demote process to lower priority
        if (scheduler_config.scheduler_type == SCHEDULER_TYPE_MULTILEVEL && 
            current->priority > PROCESS_PRIORITY_LOW) {
            
            // Adjust priority
            uint8_t new_priority;
            switch (current->priority) {
                case PROCESS_PRIORITY_REALTIME:
                    new_priority = PROCESS_PRIORITY_HIGH;
                    break;
                case PROCESS_PRIORITY_HIGH:
                    new_priority = PROCESS_PRIORITY_NORMAL;
                    break;
                case PROCESS_PRIORITY_NORMAL:
                    new_priority = PROCESS_PRIORITY_LOW;
                    break;
                default:
                    new_priority = PROCESS_PRIORITY_LOW;
                    break;
            }
            
            process_set_priority(current->pid, new_priority);
        }
        
        scheduler_stats.involuntary_preemptions++;
        scheduler_run_next();
    }
}

// Yield the CPU to another process
void scheduler_yield(void) {
    process_t* current = process_get_current();
    if (current) {
        // Mark as voluntary yield
        scheduler_stats.voluntary_yields++;
        
        // Reset time slice to prevent priority demotion
        current->ticks_remaining = 0;
        
        // Run next process
        scheduler_run_next();
    }
}

// Get number of active processes
uint32_t scheduler_process_count(void) {
    uint32_t count = 0;
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        count += queue_counts[q];
    }
    return count;
}

// Set scheduler type
void scheduler_set_type(uint8_t type) {
    if (type <= SCHEDULER_TYPE_MULTILEVEL) {
        scheduler_config.scheduler_type = type;
    }
}

// Enable/disable preemption
void scheduler_set_preemption(uint8_t enabled) {
    scheduler_config.preemption_enabled = enabled ? 1 : 0;
}

// Enable/disable priority aging
void scheduler_set_priority_aging(uint8_t enabled) {
    scheduler_config.priority_aging = enabled ? 1 : 0;
}

// Get scheduler statistics
void scheduler_get_stats(uint32_t* switches, uint32_t* yields, 
                         uint32_t* preemptions, uint32_t* runtime, 
                         uint32_t* idle, uint32_t* kernel, uint32_t* user) {
    if (switches)    *switches = scheduler_stats.context_switches;
    if (yields)      *yields = scheduler_stats.voluntary_yields;
    if (preemptions) *preemptions = scheduler_stats.involuntary_preemptions;
    if (runtime)     *runtime = scheduler_stats.total_runtime;
    if (idle)        *idle = scheduler_stats.idle_time;
    if (kernel)      *kernel = scheduler_stats.kernel_time;
    if (user)        *user = scheduler_stats.user_time;
}

// Display scheduler statistics
void scheduler_display_stats(void) {
    terminal_writestring("Scheduler Statistics:\n");
    terminal_writestring("------------------------\n");
    
    // Display scheduler type
    switch (scheduler_config.scheduler_type) {
        case SCHEDULER_TYPE_ROUND_ROBIN:
            terminal_writestring("Type: Round-Robin\n");
            break;
        case SCHEDULER_TYPE_PRIORITY:
            terminal_writestring("Type: Priority\n");
            break;
        case SCHEDULER_TYPE_MULTILEVEL:
            terminal_writestring("Type: Multilevel Feedback\n");
            break;
        default:
            terminal_writestring("Type: Unknown\n");
            break;
    }
    
    // Display configuration
    terminal_printf("Preemption: %s\n", 
                   scheduler_config.preemption_enabled ? "Enabled" : "Disabled");
    terminal_printf("Priority Aging: %s\n",
                   scheduler_config.priority_aging ? "Enabled" : "Disabled");
    terminal_printf("Time Slice Base: %d ms\n", scheduler_config.time_slice_base);
    terminal_printf("Priority Boost: Every %d ticks\n", scheduler_config.boost_interval);
    
    // Display statistics
    terminal_printf("Context Switches: %d\n", scheduler_stats.context_switches);
    terminal_printf("Voluntary Yields: %d\n", scheduler_stats.voluntary_yields);
    terminal_printf("Involuntary Preemptions: %d\n", scheduler_stats.involuntary_preemptions);
    terminal_printf("Total Runtime: %d ticks\n", scheduler_stats.total_runtime);
    
    // Display CPU utilization
    if (scheduler_stats.total_runtime > 0) {
        uint32_t idle_pct = (scheduler_stats.idle_time * 100) / scheduler_stats.total_runtime;
        uint32_t kernel_pct = (scheduler_stats.kernel_time * 100) / scheduler_stats.total_runtime;
        uint32_t user_pct = (scheduler_stats.user_time * 100) / scheduler_stats.total_runtime;
        
        terminal_printf("CPU Utilization: %d%% (Kernel: %d%%, User: %d%%, Idle: %d%%)\n",
                      100 - idle_pct, kernel_pct, user_pct, idle_pct);
    }
    
    // Display queue statistics
    terminal_writestring("\nQueue Statistics:\n");
    for (int q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        const char* queue_name;
        switch (q) {
            case QUEUE_HIGH:       queue_name = "High"; break;
            case QUEUE_NORMAL:     queue_name = "Normal"; break;
            case QUEUE_LOW:        queue_name = "Low"; break;
            case QUEUE_BACKGROUND: queue_name = "Background"; break;
            default:               queue_name = "Unknown"; break;
        }
        
        terminal_printf("%s Queue: %d processes\n", queue_name, queue_counts[q]);
        
        // Show first few processes in each queue
        if (queue_counts[q] > 0) {
            for (uint32_t i = 0; i < queue_counts[q] && i < 3; i++) {
                process_t* proc = process_queues[q][i];
                if (proc) {
                    const char* state_str;
                    switch (proc->state) {
                        case PROCESS_STATE_READY:    state_str = "Ready"; break;
                        case PROCESS_STATE_RUNNING:  state_str = "Running"; break;
                        case PROCESS_STATE_BLOCKED:  state_str = "Blocked"; break;
                        case PROCESS_STATE_SLEEPING: state_str = "Sleeping"; break;
                        default:                     state_str = "Unknown"; break;
                    }
                    
                    terminal_printf("  PID %d (%s): %s, Slice: %d/%d\n",
                                  proc->pid, proc->name, state_str,
                                  proc->ticks_remaining, proc->time_slice);
                }
            }
            
            if (queue_counts[q] > 3) {
                terminal_printf("  ... and %d more\n", queue_counts[q] - 3);
            }
        }
    }
}