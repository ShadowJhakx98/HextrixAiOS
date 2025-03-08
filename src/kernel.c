#include "terminal.h"
#include "memory.h"
#include "scheduler.h"
#include "kmalloc.h"
#include "interrupts.h"

void kernel_main(void) {
    terminal_initialize();
    terminal_writestring("Initializing Hextrix OS (x86_64)\n");

    init_paging();
    init_memory();
    init_interrupts(); // Enable interrupts

    void* ptr1 = kmalloc(256);
    if (ptr1) {
        terminal_writestring("kmalloc(256) succeeded\n");
    } else {
        terminal_writestring("kmalloc(256) failed\n");
    }
    kfree(ptr1);

    int current_task = 0;
    while (1) {
        run_next_task(&current_task);
    }
}