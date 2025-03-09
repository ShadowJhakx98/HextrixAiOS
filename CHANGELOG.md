# Changelog

All notable changes to the HextrixAI project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

# Hextrix OS - Changelog

## Version 0.4.1-beta (Current - In Progress)
- Fixed critical compilation errors in HAL and GUI subsystems:
  - Resolved undefined references in hal_framebuffer.c and hal_mouse.c
  - Implemented missing functions (find_window_at, find_control_at, abs)
  - Fixed sprintf and snprintf implementations in stdio.c
  - Resolved linker errors for missing standard library functions
  - Corrected header file inclusions and dependencies
  - Updated Makefile to properly handle nested build directories
- Added new standard library implementation:
  - Created stdlib.c with essential functions like abs()
  - Added proper stdio.h header with complete function declarations
- Successfully generated bootable ISO with complete GUI subsystem
- Improved build system reliability with better directory handling
- Eliminated critical warnings that were impacting build process

## Version 0.4.0-beta (Previous)
- Implemented framebuffer graphics driver for GUI rendering
- Added PS/2 mouse driver with event handling capabilities
- Created comprehensive window management system with:
  - Draggable, resizable windows with title bars
  - Z-order management for overlapping windows 
  - Client/server message passing architecture
  - Automated repainting and efficient region updates
- Implemented UI controls framework with buttons, labels, textboxes, etc.
- Added desktop environment with:
  - Desktop icons and taskbar
  - System clock display
  - Application launcher functionality
- Created fundamental GUI applications:
  - File browser
  - Terminal emulator
  - Text editor
  - Settings manager
- Implemented event-driven programming model for GUI applications
- Added double-buffered rendering for smooth animations
- Integrated GUI system with existing HAL architecture

## Version 0.3.9-beta
- Implemented ATA/IDE disk driver for persistent storage
- Added partition table support (MBR format)
- Created filesystem abstraction layer with FAT32 support
- Implemented disk management commands (disk info, disk part, etc.)
- Added mounting and unmounting capabilities
- Extended HAL to include storage devices
- Added proper error handling in disk operations

## Version 0.3.8-alpha
- Fixed keyboard scancode mapping to correctly interpret user input
- Fixed scheduler priority boosting to prevent infinite loop
- Integrated terminal_printf improvements for proper string formatting
- Fixed shell command display for proper help text rendering
- Resolved linker errors related to missing function implementations
- Successfully implemented complete shell input/output workflow
- Enhanced shell UI with proper VGA text mode rendering
- Ensured command history and tab completion functionality works properly
- Overall system stability improvements for day-to-day usage

## Version 0.3.7-dev-v2
- Implemented Hardware Abstraction Layer (HAL) for device independence
- Created uniform HAL interface for all hardware interactions
- Implemented timer and keyboard device drivers on HAL
- Maintained polling-based approach with a clean abstraction layer
- Simplified hardware access through standardized functions
- Added hal.h interface with clear device API
- Created device registration capability for future extension
- Updated shell commands to use HAL for input handling
- Improved project documentation to reflect HAL architecture
- Made version numbering consistent across the project

## Version 0.3.6-dev
- Implemented stable polling-based I/O model
- Removed experimental interrupt code
- Cleaned up shell commands for consistency
- Simplified system diagnostics
- Updated all documentation to reflect current approach
- Created foundation for Hardware Abstraction Layer (HAL)
- Improved system stability and reliability

## Version 0.3.5-dev
- Added interrupt diagnostics tools for system analysis
- Identified issues with interrupt system:
  - All IRQs masked in PIC
  - Interrupts disabled at CPU level (IF flag)
- Implemented simplified diagnostic command (intdiag)
- Prepared groundwork for proper interrupt implementation
- Made system more stable with improved error handling
- Created transition plan from polling to interrupt-driven I/O

## Version 0.3.4
- Implemented basic memory protection mechanisms
- Added memory region management for access control
- Added page-level protection through paging mechanism
- Implemented flags for read, write, and execute permissions
- Added kernel/user memory separation
- Implemented memory region tracking and validation
- Added shell commands for memory protection
- Added memory protection testing and validation
- Updated internal memory management to respect protection flags
- Eliminated page fault and interrupt handler dependencies
- Enhanced system stability with purely software-based protection
- Disabled memory protection by default for system stability
- Added more extensive memory mappings for kernel code
- Improved CPU state management to avoid crashes
- Enhanced page alignment verification with strict checks
- Simplified memory subsystem to avoid triggering hardware exceptions

## Earlier versions

## Version 0.3.3
- Implemented enhanced process scheduler with proper process management
- Added priority-based scheduling (LOW, NORMAL, HIGH, REALTIME priority levels)
- Added process CPU usage and runtime tracking
- Added support for process states (READY, RUNNING, BLOCKED, TERMINATED, SLEEPING)
- Implemented parent-child process relationships
- Fixed terminal scrolling to properly handle screen overflow
- Added process command extensions:
  - ps - List running processes with detailed stats (CPU usage, priority, state)
  - nice - Change process priority at runtime
  - sleep - Pause execution for a specified time
- Enhanced process scheduling algorithm with priority-based execution
- Added improved process statistics in process list display
- Fixed several system stability issues

## Version 0.3.2 
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
- Implemented memory management with kmalloc/kfree
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
