// include/hal_timer.h
#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <stdint.h>

// Initialize the timer subsystem
int hal_timer_init(void);

// Get the current number of timer ticks
uint32_t hal_timer_get_ticks(void);

// Sleep for the specified number of milliseconds
void hal_timer_sleep(uint32_t ms);

// Delay for the specified number of milliseconds
// (Alias for hal_timer_sleep with a different name)
static inline void hal_timer_delay(uint32_t ms) {
    hal_timer_sleep(ms);
}

// Register a callback function to be called on each timer tick
void hal_timer_register_callback(void (*callback)(void));

// Poll the timer hardware (used in polling mode)
void hal_timer_poll(void);

#endif // HAL_TIMER_H