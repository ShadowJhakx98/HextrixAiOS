ASM = nasm
ASMFLAGS = -f elf32
CC = gcc
CFLAGS = -m32 -ffreestanding -nostdlib -Wall -Iinclude
LD = ld
LDFLAGS = -m elf_i386 -T linker.ld
SRC_DIR = src
OBJ_DIR = build
ASM_SOURCES = $(SRC_DIR)/boot.asm $(SRC_DIR)/test_stubs.asm
C_SOURCES = $(SRC_DIR)/kernel.c \
    $(SRC_DIR)/terminal.c \
    $(SRC_DIR)/string.c \
    $(SRC_DIR)/memory.c \
    $(SRC_DIR)/kmalloc.c \
    $(SRC_DIR)/interrupts.c \
    $(SRC_DIR)/shell.c \
    $(SRC_DIR)/stdio.c \
    $(SRC_DIR)/fs.c
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm,$(OBJ_DIR)/%.o,$(ASM_SOURCES))
OBJS = $(ASM_OBJS) $(C_OBJS)

all: kernel.bin

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.bin $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(OBJ_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -f kernel.bin $(OBJ_DIR)/*.o

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