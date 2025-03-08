# HextrixAI - Advanced AI System with OS Integration

HextrixAI is a cutting-edge artificial intelligence system that combines a powerful multimodal AI assistant with low-level OS integration capabilities. This project aims to create a comprehensive AI ecosystem that can interact with users through multiple modalities while leveraging deep system integration.

## Current Development Status (v0.3.7-dev-v2)

The project is currently in active development with key features implemented:
- Stable OS foundation with comprehensive process management
- Hardware Abstraction Layer (HAL) with polling-based device drivers
- Memory protection with software validation
- Hierarchical file system with full command interface
- Cooperative multitasking with priority-based scheduling
- Interactive shell with various system commands
- Diagnostic tools for system analysis and debugging

Key development focus areas:
- Enhancing memory management system
- Improving scheduler capabilities
- Creating system call interface
- Expanding driver model

## Core Features

### Hardware Abstraction Layer (HAL)
- **Device Abstraction**: Uniform interface for hardware access
- **Polling-Based Drivers**: Stable device interaction without interrupts
- **Extensible Architecture**: Easy addition of new device drivers
- **Hardware Independence**: OS code isolated from hardware details

### Stable OS Platform
- **Process Management**: Complete PCB-based process management with state tracking
- **Memory Protection**: Region-based memory protection with access controls
- **Hierarchical File System**: Directory structure with path handling and navigation
- **Interactive Shell**: Command-line interface with file system operations
- **Scalable Architecture**: Built for future integration with AI components
- **Diagnostic Tools**: System analysis capabilities for development and debugging

### File System Features
- Directory structure with navigation (cd, pwd)
- File operations (create, read, write, delete)
- Path handling (absolute and relative paths)
- Metadata support (permissions, timestamps)
- Standard directory hierarchy (/, /home, /bin, /etc)

### Shell Commands
- **File Management**: ls, cat, write, rm
- **Directory Operations**: cd, pwd, mkdir
- **System Information**: meminfo, version, help
- **Process Management**: ps, kill, nice, sleep
- **Memory Management**: memenable, memdisable, memcheck, memregions
- **Diagnostics**: diag
- **UI Control**: clear, echo

### Future AI Capabilities (Planned)
- **Text Processing**: Advanced language understanding using multiple LLM backends
- **Image Processing**: Computer vision and image generation
- **Speech Recognition**: Audio processing and transcription
- **Emotional Intelligence**: Sentiment analysis and emotional context awareness
- **Real-time Processing**: Support for video streams and multi-modal interactions

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
