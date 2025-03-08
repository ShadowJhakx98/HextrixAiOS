# Changelog

All notable changes to the HextrixAI project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

# Hextrix OS - Changelog

# Changelog

All notable changes to the HextrixAI project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

# Hextrix OS - Changelog

## Version 0.3.2 (Current)
- Enhanced file system with hierarchical directory structure
- Implemented path handling for both absolute and relative paths
- Added support for tracking and changing current working directory
- Created larger file size capacity (8KB per file)
- Implemented file system metadata including permissions and timestamps
- Added new commands for directory management:
  - pwd - Display current working directory
  - cd - Change directory
  - mkdir - Create new directories
  - ls - List directory contents with improved formatting
- Created standard directories (/, /home, /bin, /etc)
- Fixed bugs in command processing and file system operations
- Improved shell input handling with better character processing
- Enhanced system stability by eliminating interrupt-related crashes

## Version 0.3.1
- Refactored OS to use polling-based I/O instead of interrupts for improved stability
- Implemented reliable timer polling mechanism
- Added keyboard polling support
- Fixed critical issues with system initialization
- Successfully demonstrated timer-based event handling
- Created a stable foundation for further development
- Improved memory management system reliability

## Version 0.3.0
- Added keyboard input driver with PS/2 support
- Implemented memory management with kmalloc/kfree functions
- Added simple in-memory file system with basic operations:
  - File creation and deletion
  - Reading and writing to files
  - File listing capability
- Implemented interactive shell with commands:
  - help - Show available commands
  - clear - Clear the screen
  - ls - List files
  - cat - Display file contents
  - write - Create/write to a file
  - rm - Delete a file
  - meminfo - Display memory usage statistics

## Version 0.2.0
- Converted OS from 64-bit to 32-bit architecture for better compatibility
- Fixed boot process to properly support GRUB multiboot
- Implemented proper kernel entry point with multiboot header
- Added stack initialization in boot sequence
- Created proper linker script for 32-bit mode
- Fixed VirtualBox and QEMU compatibility issues
- Implemented task scheduler with basic preemptive multitasking
- Added terminal driver with basic text output capabilities

## Version 0.1.0
- Initial OS structure and build system
- Basic kernel entry point
- Simple terminal output functionality
- Basic memory management primitives
- Created build system with makefile
- Added ISO creation capability with GRUB bootloader
- Set up basic project structure with header files and source files organization
- Created initial README file

## Roadmap (Upcoming Features)
- Implement a simple shell using polling for keyboard input
- Add a basic file system
- Revisit the interrupt system with a fresh implementation
- Add support for different keyboard layouts
- Implement a more sophisticated memory manager with paging
- Create persistent storage support
- Add simple networking capabilities
- Develop a basic GUI system
- Implement multithreading with proper synchronization
- Add support for user programs and process isolation

# HextrixAI - Changelog

## [Unreleased]

### Added
- Initial project structure
- Basic AI assistant functionality
- Core OS kernel components
- Memory management system
- Emotional state tracking
- Multimodal capabilities (text, vision, speech)

## [0.1.0] - 2025-03-07

### Added
- Initial repository setup
- Base AI system architecture in app.py
- Core OS components:
  - Memory management (memory.c, kmalloc.c)
  - Terminal interface (terminal.c)
  - Interrupt handling (interrupts.c)
  - Basic scheduler (scheduler.c)
- Build system with makefile and linker configuration
- Integration with key AI models:
  - LLaVA for vision understanding
  - Llama 3.3 for text processing
  - Gemini integration for multimodal tasks
  - Whisper for speech recognition
- Initial Google API integrations
- Cloudflare AI inference capabilities
- Emotion detection and emotional state management
- Basic self-awareness modules
- Memory persistence using Google Drive

### Changed
- N/A (initial release)

### Fixed
- N/A (initial release)

## Future Releases

Upcoming releases will focus on:
- Advanced multimodal capabilities
- Enhanced OS integration
- Improved neural memory systems
- Ethical and control mechanism implementation
- Hardware integration with Jetson Thor
- ZEISS smart glass capabilities
- Windows compatibility layer
- Specialized support features for healthcare
