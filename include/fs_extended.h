// include/fs_extended.h
#ifndef FS_EXTENDED_H
#define FS_EXTENDED_H

#include <stdint.h>
#include <stddef.h>
#include "fs.h"

// Maximum number of partitions
#define MAX_PARTITIONS 16

// Maximum number of mount points
#define MAX_MOUNT_POINTS 8

// Partition types
#define PART_TYPE_UNKNOWN  0x00
#define PART_TYPE_FAT12    0x01
#define PART_TYPE_FAT16    0x04
#define PART_TYPE_EXTENDED 0x05
#define PART_TYPE_FAT16B   0x06
#define PART_TYPE_NTFS     0x07
#define PART_TYPE_FAT32    0x0B
#define PART_TYPE_FAT32X   0x0C
#define PART_TYPE_FAT16X   0x0E
#define PART_TYPE_EXTENDED2 0x0F
#define PART_TYPE_LINUX    0x83

// File system types
#define FS_TYPE_UNKNOWN   0
#define FS_TYPE_FAT16     1
#define FS_TYPE_FAT32     2
#define FS_TYPE_EXT2      3
#define FS_TYPE_RAMFS     4

// Partition structure
typedef struct {
    uint8_t bootable;       // 0x80 = bootable, 0x00 = not bootable
    uint8_t start_head;     // Starting head
    uint8_t start_sector;   // Starting sector (bits 0-5) and high bits of cylinder (bits 6-7)
    uint8_t start_cylinder; // Low bits of starting cylinder
    uint8_t system_id;      // Partition type
    uint8_t end_head;       // Ending head
    uint8_t end_sector;     // Ending sector (bits 0-5) and high bits of cylinder (bits 6-7)
    uint8_t end_cylinder;   // Low bits of ending cylinder
    uint32_t start_lba;     // Starting LBA address
    uint32_t total_sectors; // Total sectors in partition
} __attribute__((packed)) mbr_partition_t;

// MBR (Master Boot Record) structure
typedef struct {
    uint8_t bootstrap[446];          // Bootstrap code
    mbr_partition_t partitions[4];   // Partition table (4 entries)
    uint16_t signature;              // Boot signature (0xAA55)
} __attribute__((packed)) mbr_t;

// Partition information
typedef struct {
    uint8_t drive;          // Drive number
    uint8_t partition_num;  // Partition number on the drive
    uint8_t type;           // Partition type
    uint8_t fs_type;        // Detected file system type
    uint32_t start_lba;     // Starting LBA address
    uint32_t total_sectors; // Total sectors in partition
    uint32_t size_mb;       // Size in megabytes
} partition_info_t;

// Mount point structure
typedef struct {
    char mount_point[FS_MAX_PATH];   // Mount point path
    uint8_t drive;                   // Drive number
    uint8_t partition;               // Partition number
    uint8_t fs_type;                 // File system type
    void* fs_data;                   // File system specific data
} mount_point_t;

// Initialize extended file system
void fs_extended_init(void);

// Detect partitions on a drive
int fs_detect_partitions(uint8_t drive);

// Format a partition
int fs_format_partition(uint8_t drive, uint8_t partition, uint8_t fs_type);

// Mount a partition
int fs_mount(const char* mount_point, uint8_t drive, uint8_t partition);

// Unmount a partition
int fs_unmount(const char* mount_point);

// Get partition information
partition_info_t* fs_get_partition(uint8_t drive, uint8_t partition);

// Get mount point information
mount_point_t* fs_get_mount_point(const char* path);

// Print partition table
void fs_print_partitions(uint8_t drive);

// Create a partition table on a drive
int fs_create_partition_table(uint8_t drive);

// Add a partition to a drive
int fs_add_partition(uint8_t drive, uint32_t start_lba, uint32_t size_sectors, uint8_t type);

// Delete a partition
int fs_delete_partition(uint8_t drive, uint8_t partition);

// FAT32-specific operations
int fat32_mount(uint8_t drive, uint32_t start_lba, uint32_t sectors, void** fs_data);
int fat32_unmount(void* fs_data);
int fat32_read_file(void* fs_data, const char* path, void* buffer, size_t size);
int fat32_write_file(void* fs_data, const char* path, const void* buffer, size_t size);
int fat32_create_file(void* fs_data, const char* path);
int fat32_delete_file(void* fs_data, const char* path);
int fat32_create_directory(void* fs_data, const char* path);
int fat32_delete_directory(void* fs_data, const char* path);
int fat32_list_directory(void* fs_data, const char* path);
int fat32_get_file_size(void* fs_data, const char* path);

#endif // FS_EXTENDED_H