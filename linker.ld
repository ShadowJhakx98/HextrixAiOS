OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

/* Define stack size explicitly */
STACK_SIZE = 0x10000;  /* 64KB stack */

MEMORY
{
    rom : ORIGIN = 0x00000000, LENGTH = 128K
    ram : ORIGIN = 0x00100000, LENGTH = 8M
}

SECTIONS
{
    . = 0x100000;  /* Start at 1MB memory location */

    .multiboot : 
    {
        *(.multiboot)
    } > ram

    .text ALIGN(4K) : 
    {
        *(.text)
        *(.text.*)
    } > ram

    .rodata ALIGN(4K) : 
    {
        *(.rodata)
        *(.rodata.*)
    } > ram

    .data ALIGN(4K) : 
    {
        *(.data)
        *(.data.*)
    } > ram

    .bss ALIGN(4K) : 
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    } > ram

    .stack ALIGN(4K) : 
    {
        __stack_canary = .;  /* Stack canary variable - at start of stack */
        *(.stack)
        . += STACK_SIZE; /* Reserve STACK_SIZE bytes for stack */
    } > ram

    /DISCARD/ : 
    {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}