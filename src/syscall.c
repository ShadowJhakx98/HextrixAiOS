// src/syscall.c
#include "syscall.h"
#include "terminal.h"
#include "stdio.h"
#include "process.h"
#include "fs.h"
#include "memory.h"
#include "kmalloc.h"
#include "string.h"
#include "hal.h"

// Array of system call handlers
static syscall_handler_t syscall_handlers[256] = {0};

// Last error code
static int last_error = SYSCALL_SUCCESS;

// Get the last error code
int syscall_get_error(void) {
    return last_error;
}

// Set the last error code
void syscall_set_error(int error) {
    last_error = error;
}

// System call handler for exit
static int handle_sys_exit(uint32_t status, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    process_t* current = process_get_current();
    if (current) {
        // Set exit code and terminate
        current->exit_code = status;
        process_terminate(current->pid);
    }
    return 0;
}

// System call handler for write
static int handle_sys_write(uint32_t fd, uint32_t buf, uint32_t count, uint32_t unused) {
    // Check if buffer is valid
    if (!is_valid_access(buf, MEM_PROT_READ)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Simplified file descriptor handling:
    // fd 0 = stdin, fd 1 = stdout, fd 2 = stderr
    if (fd == 1 || fd == 2) {
        // Write to terminal
        for (uint32_t i = 0; i < count; i++) {
            terminal_putchar(((char*)buf)[i]);
        }
        return count;
    } else {
        // For now, just stub other file writes
        // In a real implementation, we would use fs_write
        syscall_set_error(SYSCALL_ENOSYS);
        return -1;
    }
}

// System call handler for read
static int handle_sys_read(uint32_t fd, uint32_t buf, uint32_t count, uint32_t unused) {
    // Check if buffer is valid
    if (!is_valid_access(buf, MEM_PROT_WRITE)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Simplified file descriptor handling
    if (fd == 0) {
        // Read from keyboard
        uint32_t bytes_read = 0;
        char* buffer = (char*)buf;
        
        while (bytes_read < count) {
            // Use HAL keyboard functions for input
            if (hal_keyboard_is_key_available()) {
                int scancode = hal_keyboard_read();
                
                // Convert scancode to ASCII (simplified)
                if (scancode >= 0 && scancode < 128) {
                    // Very simple conversion - would need proper mapping table
                    char c = 0;
                    if (scancode >= 2 && scancode <= 11) {
                        // Digits 1-0
                        c = '0' + (scancode - 1) % 10;
                    } else if (scancode >= 16 && scancode <= 25) {
                        // Letters Q-P
                        c = 'q' + (scancode - 16);
                    } else if (scancode >= 30 && scancode <= 38) {
                        // Letters A-L
                        c = 'a' + (scancode - 30);
                    } else if (scancode >= 44 && scancode <= 50) {
                        // Letters Z-M
                        c = 'z' + (scancode - 44);
                    } else if (scancode == 57) {
                        // Space
                        c = ' ';
                    } else if (scancode == 28) {
                        // Enter
                        c = '\n';
                    }
                    
                    if (c) {
                        buffer[bytes_read++] = c;
                        
                        // Echo to terminal
                        terminal_putchar(c);
                        
                        // If newline or buffer full, we're done
                        if (c == '\n' || bytes_read >= count) {
                            break;
                        }
                    }
                }
            } else {
                // No key available, yield to other processes
                process_yield();
            }
        }
        
        return bytes_read;
    } else {
        // Read from file
        // In a real implementation, we would use fs_read
        syscall_set_error(SYSCALL_ENOSYS);
        return -1;
    }
}

// System call handler for open
static int handle_sys_open(uint32_t pathname, uint32_t flags, uint32_t unused1, uint32_t unused2) {
    // Check if pathname is valid
    if (!is_valid_access(pathname, MEM_PROT_READ)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Convert pathname to string
    const char* path = (const char*)pathname;
    
    // Check if file exists
    fs_node_t info;
    if (fs_stat(path, &info) < 0) {
        // File doesn't exist, create it if O_CREAT flag is set
        if (flags & 0x40) { // 0x40 = O_CREAT
            if (fs_create(path, FS_TYPE_FILE) < 0) {
                syscall_set_error(SYSCALL_EACCES);
                return -1;
            }
        } else {
            syscall_set_error(SYSCALL_ENOENT);
            return -1;
        }
    }
    
    // For now, return a dummy file descriptor (we don't have proper fd table yet)
    // In a real implementation, we would allocate a file descriptor
    return 3; // First available fd after stdin/stdout/stderr
}

// System call handler for close
static int handle_sys_close(uint32_t fd, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    // For now, just return success
    // In a real implementation, we would free the file descriptor
    return 0;
}

// System call handler for getpid
static int handle_sys_getpid(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    process_t* current = process_get_current();
    if (current) {
        return current->pid;
    }
    return 0;
}

// System call handler for sleep
static int handle_sys_sleep(uint32_t ms, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    process_t* current = process_get_current();
    if (current) {
        process_sleep(current->pid, ms);
    }
    return 0;
}

// System call handler for time
static int handle_sys_time(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    return hal_timer_get_ticks();
}

// System call handler for memory allocation
static int handle_sys_allocate(uint32_t size, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    void* ptr = kmalloc(size);
    if (!ptr) {
        syscall_set_error(SYSCALL_ENOMEM);
        return 0;
    }
    return (uint32_t)ptr;
}

// System call handler for memory deallocation
static int handle_sys_free(uint32_t ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    if (ptr == 0) {
        return 0;
    }
    
    // Validate pointer before freeing
    if (!is_valid_access(ptr, MEM_PROT_READ | MEM_PROT_WRITE)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    kfree((void*)ptr);
    return 0;
}

// System call handler for file stat
static int handle_sys_stat(uint32_t pathname, uint32_t stat_buf, uint32_t unused1, uint32_t unused2) {
    // Check if pathname and stat_buf are valid
    if (!is_valid_access(pathname, MEM_PROT_READ) || 
        !is_valid_access(stat_buf, MEM_PROT_WRITE)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Convert pathname to string
    const char* path = (const char*)pathname;
    
    // Get file status
    return fs_stat(path, (fs_node_t*)stat_buf);
}

// System call handler for mkdir
static int handle_sys_mkdir(uint32_t pathname, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    // Check if pathname is valid
    if (!is_valid_access(pathname, MEM_PROT_READ)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Convert pathname to string
    const char* path = (const char*)pathname;
    
    // Create directory
    return fs_mkdir(path);
}

// System call handler for rmdir (uses fs_delete for now)
static int handle_sys_rmdir(uint32_t pathname, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    // Check if pathname is valid
    if (!is_valid_access(pathname, MEM_PROT_READ)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Convert pathname to string
    const char* path = (const char*)pathname;
    
    // Delete directory (fs_delete handles both files and directories)
    return fs_delete(path);
}

// System call handler for chdir
static int handle_sys_chdir(uint32_t pathname, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    // Check if pathname is valid
    if (!is_valid_access(pathname, MEM_PROT_READ)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Convert pathname to string
    const char* path = (const char*)pathname;
    
    // Change directory
    return fs_chdir(path);
}

// System call handler for getcwd
static int handle_sys_getcwd(uint32_t buf, uint32_t size, uint32_t unused1, uint32_t unused2) {
    // Check if buf is valid
    if (!is_valid_access(buf, MEM_PROT_WRITE)) {
        syscall_set_error(SYSCALL_EFAULT);
        return 0;
    }
    
    // Get current directory
    const char* cwd = fs_getcwd();
    if (!cwd) {
        return 0;
    }
    
    // Copy cwd to buf
    strncpy((char*)buf, cwd, size);
    return (uint32_t)buf;
}

// System call handler for file deletion
static int handle_sys_delete(uint32_t pathname, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    // Check if pathname is valid
    if (!is_valid_access(pathname, MEM_PROT_READ)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Convert pathname to string
    const char* path = (const char*)pathname;
    
    // Delete file
    return fs_delete(path);
}

// System call handler for process info
static int handle_sys_process_info(uint32_t pid, uint32_t info_buf, uint32_t unused1, uint32_t unused2) {
    // Check if info_buf is valid
    if (!is_valid_access(info_buf, MEM_PROT_WRITE)) {
        syscall_set_error(SYSCALL_EFAULT);
        return -1;
    }
    
    // Get process
    process_t* proc = process_get_by_pid(pid);
    if (!proc) {
        syscall_set_error(SYSCALL_ENOENT);
        return -1;
    }
    
    // Copy process info to buffer (simplified for now)
    process_t* dest = (process_t*)info_buf;
    dest->pid = proc->pid;
    dest->state = proc->state;
    dest->priority = proc->priority;
    dest->total_runtime = proc->total_runtime;
    dest->cpu_usage_percent = proc->cpu_usage_percent;
    dest->parent_pid = proc->parent_pid;
    dest->exit_code = proc->exit_code;
    strncpy(dest->name, proc->name, sizeof(dest->name));
    
    return 0;
}

// System call dispatcher
int syscall_dispatch(uint32_t num, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4) {
    // Reset error code
    syscall_set_error(SYSCALL_SUCCESS);
    
    // Check if system call number is valid
    if (num >= 256 || !syscall_handlers[num]) {
        syscall_set_error(SYSCALL_ENOSYS);
        return -1;
    }
    
    // Call system call handler
    return syscall_handlers[num](param1, param2, param3, param4);
}

// Register a system call handler
void register_syscall(uint32_t num, syscall_handler_t handler) {
    if (num < 256) {
        syscall_handlers[num] = handler;
    }
}

// Initialize system call interface
void syscall_init(void) {
    terminal_writestring("Initializing system call interface\n");
    
    // Register system call handlers
    register_syscall(SYS_EXIT, handle_sys_exit);
    register_syscall(SYS_WRITE, handle_sys_write);
    register_syscall(SYS_READ, handle_sys_read);
    register_syscall(SYS_OPEN, handle_sys_open);
    register_syscall(SYS_CLOSE, handle_sys_close);
    register_syscall(SYS_GETPID, handle_sys_getpid);
    register_syscall(SYS_SLEEP, handle_sys_sleep);
    register_syscall(SYS_TIME, handle_sys_time);
    register_syscall(SYS_ALLOCATE, handle_sys_allocate);
    register_syscall(SYS_FREE, handle_sys_free);
    register_syscall(SYS_STAT, handle_sys_stat);
    register_syscall(SYS_MKDIR, handle_sys_mkdir);
    register_syscall(SYS_RMDIR, handle_sys_rmdir);
    register_syscall(SYS_CHDIR, handle_sys_chdir);
    register_syscall(SYS_GETCWD, handle_sys_getcwd);
    register_syscall(SYS_DELETE, handle_sys_delete);
    register_syscall(SYS_PROCESS_INFO, handle_sys_process_info);
    
    terminal_writestring("System call interface initialized\n");
}

// The following are wrapper functions that can be called by kernel code

int sys_exit(int status) {
    return syscall_dispatch(SYS_EXIT, status, 0, 0, 0);
}

int sys_write(int fd, const void* buf, uint32_t count) {
    return syscall_dispatch(SYS_WRITE, fd, (uint32_t)buf, count, 0);
}

int sys_read(int fd, void* buf, uint32_t count) {
    return syscall_dispatch(SYS_READ, fd, (uint32_t)buf, count, 0);
}

int sys_open(const char* pathname, int flags) {
    return syscall_dispatch(SYS_OPEN, (uint32_t)pathname, flags, 0, 0);
}

int sys_close(int fd) {
    return syscall_dispatch(SYS_CLOSE, fd, 0, 0, 0);
}

int sys_getpid(void) {
    return syscall_dispatch(SYS_GETPID, 0, 0, 0, 0);
}

int sys_sleep(uint32_t ms) {
    return syscall_dispatch(SYS_SLEEP, ms, 0, 0, 0);
}

uint32_t sys_time(void) {
    return syscall_dispatch(SYS_TIME, 0, 0, 0, 0);
}

void* sys_allocate(uint32_t size) {
    return (void*)syscall_dispatch(SYS_ALLOCATE, size, 0, 0, 0);
}

int sys_free(void* ptr) {
    return syscall_dispatch(SYS_FREE, (uint32_t)ptr, 0, 0, 0);
}

int sys_stat(const char* pathname, void* stat_buf) {
    return syscall_dispatch(SYS_STAT, (uint32_t)pathname, (uint32_t)stat_buf, 0, 0);
}

int sys_mkdir(const char* pathname) {
    return syscall_dispatch(SYS_MKDIR, (uint32_t)pathname, 0, 0, 0);
}

int sys_rmdir(const char* pathname) {
    return syscall_dispatch(SYS_RMDIR, (uint32_t)pathname, 0, 0, 0);
}

int sys_chdir(const char* pathname) {
    return syscall_dispatch(SYS_CHDIR, (uint32_t)pathname, 0, 0, 0);
}

char* sys_getcwd(char* buf, uint32_t size) {
    return (char*)syscall_dispatch(SYS_GETCWD, (uint32_t)buf, size, 0, 0);
}

int sys_delete(const char* pathname) {
    return syscall_dispatch(SYS_DELETE, (uint32_t)pathname, 0, 0, 0);
}

int sys_process_info(int pid, void* info_buf) {
    return syscall_dispatch(SYS_PROCESS_INFO, pid, (uint32_t)info_buf, 0, 0);
}