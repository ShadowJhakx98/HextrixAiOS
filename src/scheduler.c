#include "scheduler.h"
#include "terminal.h"
#include "interrupts.h"

static void task1(void) {
    static int state = 0;
    if (state < 5) {
        terminal_writestring("Task 1: ");
        terminal_putchar('0' + state);
        terminal_putchar('\n');
        state++;
    } else {
        state = 0;
    }
}

static void task2(void) {
    static int state = 0;
    if (state < 5) {
        terminal_writestring("Task 2: ");
        terminal_putchar('0' + state);
        terminal_putchar('\n');
        state++;
    } else {
        state = 0;
    }
}

static void task3(void) {
    if (keyboard_index > 0) {
        for (int i = 0; i < keyboard_index; i++) {
            terminal_putchar(keyboard_buffer[i]);
        }
        keyboard_index = 0; // Reset buffer
    }
}

static void (*tasks[3])(void) = {task1, task2, task3};

void run_next_task(int* current_task) {
    tasks[*current_task]();
    *current_task = (*current_task + 1) % 3;
}