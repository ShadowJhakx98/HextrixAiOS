# HextrixAI - Advanced AI System with OS Integration

HextrixAI is a cutting-edge artificial intelligence system that combines a powerful multimodal AI assistant with low-level OS integration capabilities. This project aims to create a comprehensive AI ecosystem that can interact with users through multiple modalities while leveraging deep system integration.

## Current Development Status (v0.3.8-dev)

The project is currently in active development with a significantly improved user interface and stability:
- ✅ Fully functional command-line shell with reliable keyboard input
- ✅ Comprehensive Hardware Abstraction Layer (HAL) with device drivers
- ✅ Memory protection with software validation
- ✅ Hierarchical file system with full command interface
- ✅ Cooperative multitasking with priority-based scheduling
- ✅ Command history and tab completion
- ✅ Enhanced printf functionality with proper text formatting

## Core Features

### Command Line Shell
- **User-Friendly Interface**: Reliable text-based interface with proper keyboard mapping
- **Command Processing**: Support for a wide range of system commands
- **History & Completion**: Command history navigation and tab completion
- **Formatted Output**: Proper text formatting for readable output

### Hardware Abstraction Layer (HAL)
- **Device Abstraction**: Uniform interface for hardware access
- **Polling-Based Drivers**: Stable device interaction without interrupts
- **Extensible Architecture**: Easy addition of new device drivers
- **Hardware Independence**: OS code isolated from hardware details

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

# Run in QEMU
qemu-system-i386 -cdrom hextrix.iso -m 512M
```

## Project Structure

```
src/               - Source files
  kernel.c         - Main kernel entry point
  terminal.c       - Text display handling
  memory.c         - Memory management
  kmalloc.c        - Heap allocation
  fs.c             - File system implementation
  shell.c          - Interactive command shell
  hal.c            - Hardware Abstraction Layer core
  hal_timer.c      - Timer device implementation
  hal_keyboard.c   - Keyboard device implementation
  hal_storage.c    - Storage device implementation
  string.c         - String handling functions
  stdio.c          - Input/output utilities
  boot.asm         - Bootloader and initialization
  process.c        - Process management
  scheduler.c      - Task scheduling

include/           - Header files
  hal.h            - Hardware Abstraction Layer interface
  fs.h             - File system definitions
  shell.h          - Shell interface
  kmalloc.h        - Memory allocation interface
  process.h        - Process management declarations
  scheduler.h      - Task scheduler interface
```

## Development Roadmap

### Current Sprint (v0.3.8-dev)
- ✅ Fix keyboard input and text formatting issues
- ✅ Stabilize shell environment for usability
- 🟡 Enhance file system functionality
- 🟡 Improve memory management efficiency
- 🔲 Implement system call framework

### Future Sprints
- Add support for command piping and I/O redirection
- Implement proper disk-based file system
- Create expanded device driver support
- Add networking capabilities
- Develop graphical user interface
- Integrate AI components

## License
This project is licensed under the proprietary license - see LICENSE.md for details.

## Acknowledgments
- Various open-source OS development resources
- Contributors and researchers in OS development
