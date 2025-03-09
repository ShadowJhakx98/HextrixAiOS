ASM = nasm
ASMFLAGS = -f elf32
CC = gcc
CFLAGS = -m32 -ffreestanding -nostdlib -Wall -Iinclude
LD = ld
LDFLAGS = -m elf_i386 -T linker.ld
SRC_DIR = src
OBJ_DIR = build

# Regular build sources
ASM_SOURCES = $(SRC_DIR)/boot.asm $(SRC_DIR)/test_stubs.asm $(SRC_DIR)/context_switch.asm
# Add these to the C_SOURCES variable in the makefile
C_SOURCES = $(SRC_DIR)/kernel.c \
    $(SRC_DIR)/terminal.c \
    $(SRC_DIR)/string.c \
    $(SRC_DIR)/memory.c \
    $(SRC_DIR)/kmalloc.c \
    $(SRC_DIR)/interrupts.c \
    $(SRC_DIR)/shell.c \
    $(SRC_DIR)/stdio.c \
    $(SRC_DIR)/stdlib.c \
    $(SRC_DIR)/fs.c \
    $(SRC_DIR)/fs_extended.c \
    $(SRC_DIR)/hal_ata.c \
    $(SRC_DIR)/process.c \
    $(SRC_DIR)/scheduler.c \
    $(SRC_DIR)/system_utils.c \
    $(SRC_DIR)/hal.c \
    $(SRC_DIR)/hal_timer.c \
    $(SRC_DIR)/hal_keyboard.c \
    $(SRC_DIR)/hal_storage.c \
    $(SRC_DIR)/gui/desktop.c \
    $(SRC_DIR)/gui/window.c \
    $(SRC_DIR)/hal_framebuffer.c \
    $(SRC_DIR)/hal_mouse.c
# Generate object file lists
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm,$(OBJ_DIR)/%.o,$(ASM_SOURCES))
OBJS = $(ASM_OBJS) $(C_OBJS)

# Default target
all: kernel.bin

# Link kernel
kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Compile C files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile ASM files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(OBJ_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# Clean up build files
clean:
	rm -f kernel.bin diagnostics.bin $(OBJ_DIR)/*.o

# Create ISO image
iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "Hextrix OS" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
	echo '  boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o hextrix.iso iso

# Simple diagnostic shell version - embed the diagnostic command
# but don't include the actual diagnostics library
diagnostic-simple: kernel.bin
	mkdir -p diag-iso/boot/grub
	cp kernel.bin diag-iso/boot/kernel.bin
	echo 'set timeout=0' > diag-iso/boot/grub/grub.cfg
	echo 'set default=0' >> diag-iso/boot/grub/grub.cfg
	echo 'menuentry "Hextrix OS" {' >> diag-iso/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.bin' >> diag-iso/boot/grub/grub.cfg
	echo '  boot' >> diag-iso/boot/grub/grub.cfg
	echo '}' >> diag-iso/boot/grub/grub.cfg
	grub-mkrescue -o hextrix-diagnostic.iso diag-iso

.PHONY: all clean iso diagnostic-simple