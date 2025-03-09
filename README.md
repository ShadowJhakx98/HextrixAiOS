# HextrixAI - Advanced AI System with OS Integration

HextrixAI is a cutting-edge artificial intelligence system that combines a powerful multimodal AI assistant with low-level OS integration capabilities. This project aims to create a comprehensive AI ecosystem that can interact with users through multiple modalities while leveraging deep system integration.

## Current Development Status (v0.3.9-dev)

The project is in active development with significant progress in core OS foundations:
- ✅ Fully functional Hardware Abstraction Layer (HAL) with device drivers
- ✅ Memory management with paging and protection capabilities
- ✅ Process management with priority-based scheduling
- ✅ Shell environment with history and tab completion
- ✅ ATA/IDE disk driver for persistent storage
- ✅ File system with partition support and FAT32 capabilities
- ✅ Comprehensive command-line interface

## Core Features

### Hardware Abstraction Layer (HAL)
- **Device Abstraction**: Uniform interface for hardware access
- **Polling-Based Drivers**: Stable device interaction without interrupts
- **Extensible Architecture**: Easy addition of new device drivers
- **Hardware Independence**: OS code isolated from hardware details

### Storage System
- **ATA/IDE Disk Driver**: Support for hard disk storage
- **Partition Management**: MBR partition table support
- **File System Support**: Basic FAT32 implementation
- **Mount Points**: Ability to mount and unmount partitions

### Process Management
- **Process Control**: Complete PCB-based process management with state tracking
- **Priority Scheduling**: Priority-based process scheduling with aging
- **Memory Protection**: Region-based memory protection with access controls
- **Task Management**: Process creation, termination, and priority adjustment

### File System
- **Hierarchical Structure**: Directory structure with navigation (cd, pwd)
- **File Operations**: File creation, reading, writing, and deletion
- **Path Handling**: Support for absolute and relative paths
- **Metadata**: File permissions and attribute management

### Memory Management
- **Protection Mechanisms**: Software-based memory protection
- **Region Control**: Memory region management for access control
- **Paging System**: Page-level protection through paging
- **Permission Flags**: Read/write/execute permission enforcement

## Command Set

### File Management
- `ls` - List files in directory
- `cat` - Display file contents
- `write` - Create/edit a file
- `rm` - Delete a file
- `pwd` - Show current directory
- `cd` - Change current directory
- `mkdir` - Create a directory

### Disk Management
- `disk info` - Display disk information
- `disk part` - Display partition table
- `disk mkpart` - Create a partition
- `disk rmpart` - Delete a partition
- `disk format` - Format a partition
- `mount` - Mount a file system
- `umount` - Unmount a file system

### Process Management
- `ps` - List running processes
- `kill` - Terminate a process
- `nice` - Change process priority
- `sleep` - Sleep for milliseconds
- `sched` - Display scheduler info

### Memory Management
- `meminfo` - Display memory usage
- `memenable` - Enable memory protection
- `memdisable` - Disable memory protection
- `memcheck` - Check memory access validity
- `memregions` - Display memory regions

### Shell Commands
- `help` - Show available commands
- `clear` - Clear the screen
- `echo` - Display text
- `version` - Show OS version
- `history` - Show command history
- `reboot` - Reboot the system
- `exit` - Exit the shell

### System Tools
- `diag` - Run system diagnostics

## Getting Started

### Prerequisites
- 64-bit x86 system for development
- QEMU or similar virtual machine for testing
- GCC cross-compiler for i686 target
- GRUB for bootloader

### Building and Running
```bash
# Clone the repository
git clone https://github.com/yourusername/hextrixai.git

# Navigate to the project directory
cd hextrixai

# Build the kernel
make clean
make

# Create an ISO image
make iso

# Create a virtual disk (if needed)
qemu-img create -f raw hextrix_disk.img 64M

# Run in QEMU
qemu-system-i386 -cdrom hextrix.iso -drive file=hextrix_disk.img,format=raw,if=ide,index=0,media=disk -m 512M
