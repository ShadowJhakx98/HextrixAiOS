# HextrixAI - Advanced AI System with OS Integration

HextrixAI is a cutting-edge artificial intelligence system that combines a powerful multimodal AI assistant with low-level OS integration capabilities. This project aims to create a comprehensive AI ecosystem that can interact with users through multiple modalities while leveraging deep system integration.

## Core Features

### Stable OS Platform
- **Polling-based I/O**: Reliable event handling without interrupt-related issues
- **Memory Management**: Advanced heap allocation with kmalloc/kfree functions
- **Hierarchical File System**: Directory structure with path handling and navigation
- **Interactive Shell**: Command-line interface with file system operations
- **Scalable Architecture**: Built for future integration with AI components

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
- **UI Control**: clear, echo

### Future AI Capabilities (Planned)
- **Text Processing**: Advanced language understanding using multiple LLM backends
- **Image Processing**: Computer vision and image generation
- **Speech Recognition**: Audio processing and transcription
- **Emotional Intelligence**: Sentiment analysis and emotional context awareness
- **Real-time Processing**: Support for video streams and multi-modal interactions

## Getting Started

### Prerequisites
- 64-bit x86 system for OS components
- QEMU or similar virtual machine for testing

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
Project Structure

src/ - Source files

kernel.c - Main kernel entry point
terminal.c - Text display handling
memory.c - Memory management
kmalloc.c - Heap allocation
fs.c - File system implementation
shell.c - Interactive command shell
interrupts.c - I/O polling implementation
string.c - String handling functions
stdio.c - Input/output utilities
boot.asm - Bootloader and initialization


include/ - Header files

fs.h - File system definitions
shell.h - Shell interface
kmalloc.h - Memory allocation interface
interrupts.h - I/O interface declarations



Current Development Status
The project currently has a stable OS foundation with a polling-based I/O system, replacing the previous interrupt-driven approach. This provides greater stability and reliability, with a hierarchical file system and comprehensive shell interface.
For the latest changes, see CHANGELOG.md.
Roadmap
See TODO.md for the comprehensive development roadmap.
License
This project is licensed under the proprietary license - see LICENSE.md for details.
Acknowledgments

Various open-source OS development resources
Contributors and researchers in OS development
