// include/syscall.h
#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// System call numbers
#define SYS_EXIT            1
#define SYS_WRITE           2
#define SYS_READ            3
#define SYS_OPEN            4
#define SYS_CLOSE           5
#define SYS_GETPID          6
#define SYS_FORK            7
#define SYS_EXEC            8
#define SYS_SLEEP           9
#define SYS_TIME            10
#define SYS_ALLOCATE        11
#define SYS_FREE            12
#define SYS_STAT            13
#define SYS_SEEK            14
#define SYS_MKDIR           15
#define SYS_RMDIR           16
#define SYS_CHDIR           17
#define SYS_GETCWD          18
#define SYS_DELETE          19
#define SYS_PROCESS_INFO    20

// Error codes
#define SYSCALL_SUCCESS     0
#define SYSCALL_ERROR      -1
#define SYSCALL_ENOENT     -2   // No such file or directory
#define SYSCALL_EACCES     -3   // Permission denied
#define SYSCALL_EINVAL     -4   // Invalid argument
#define SYSCALL_ENOSYS     -5   // Function not implemented
#define SYSCALL_EFAULT     -6   // Bad address
#define SYSCALL_ENOMEM     -7   // Out of memory
#define SYSCALL_EBUSY      -8   // Device or resource busy
#define SYSCALL_EEXIST     -9   // File exists
#define SYSCALL_ENOTDIR    -10  // Not a directory
#define SYSCALL_EISDIR     -11  // Is a directory
#define SYSCALL_EMFILE     -12  // Too many open files

// Initialize system call interface
void syscall_init(void);

// System call handler function pointer type
typedef int (*syscall_handler_t)(uint32_t, uint32_t, uint32_t, uint32_t);

// Register a system call handler
void register_syscall(uint32_t num, syscall_handler_t handler);

// System call dispatcher
int syscall_dispatch(uint32_t num, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4);

// System call wrapper functions
int sys_exit(int status);
int sys_write(int fd, const void* buf, uint32_t count);
int sys_read(int fd, void* buf, uint32_t count);
int sys_open(const char* pathname, int flags);
int sys_close(int fd);
int sys_getpid(void);
int sys_fork(void);
int sys_exec(const char* pathname, char* const argv[]);
int sys_sleep(uint32_t ms);
uint32_t sys_time(void);
void* sys_allocate(uint32_t size);
int sys_free(void* ptr);
int sys_stat(const char* pathname, void* stat_buf);
int sys_seek(int fd, int offset, int whence);
int sys_mkdir(const char* pathname);
int sys_rmdir(const char* pathname);
int sys_chdir(const char* pathname);
char* sys_getcwd(char* buf, uint32_t size);
int sys_delete(const char* pathname);
int sys_process_info(int pid, void* info_buf);

#endif // SYSCALL_H