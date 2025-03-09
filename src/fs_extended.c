// src/fs_extended.c
#include "fs_extended.h"
#include "hal_ata.h"
#include "kmalloc.h"
#include "string.h"
#include "terminal.h"
#include "stdio.h"

// Global partition and mount tables
static partition_info_t partitions[MAX_PARTITIONS];
static mount_point_t mount_points[MAX_MOUNT_POINTS];

// Initialize global structures
void fs_extended_init(void) {
    terminal_writestring("Initializing extended file system...\n");
    
    // Clear partition and mount tables
    memset(partitions, 0, sizeof(partitions));
    memset(mount_points, 0, sizeof(mount_points));
    
    // Initialize ATA subsystem
    hal_ata_init();
    
    // Detect partitions on all available drives
    for (uint8_t drive = 0; drive < 4; drive++) {
        ata_device_t* device = hal_ata_get_device(drive);
        if (device && device->present) {
            fs_detect_partitions(drive);
        }
    }
    
    terminal_writestring("Extended file system initialized\n");
}

// Parse the MBR and detect partitions
int fs_detect_partitions(uint8_t drive) {
    ata_device_t* device = hal_ata_get_device(drive);
    if (!device || !device->present) {
        return -1;
    }
    
    // Allocate buffer for MBR
    mbr_t* mbr = kmalloc(512);
    if (!mbr) {
        terminal_writestring("Failed to allocate memory for MBR\n");
        return -1;
    }
    
    // Read MBR (first sector)
    if (hal_ata_read_sectors(drive, 0, 1, mbr) != 0) {
        terminal_writestring("Failed to read MBR\n");
        kfree(mbr);
        return -1;
    }
    
    // Check MBR signature
    if (mbr->signature != 0xAA55) {
        terminal_printf("Invalid MBR signature: 0x%x\n", mbr->signature);
        kfree(mbr);
        return -1;
    }
    
    // Parse partition table
    int partition_count = 0;
    
    terminal_printf("Partitions on drive %d:\n", drive);
    terminal_writestring("Num  Boot  Type        Start       Size(MB)\n");
    terminal_writestring("--------------------------------------------\n");
    
    for (int i = 0; i < 4; i++) {
        mbr_partition_t* part = &mbr->partitions[i];
        
        // Skip empty partitions
        if (part->system_id == 0) {
            continue;
        }
        
        // Create partition entry
        uint8_t partition_idx = drive * 4 + i;
        if (partition_idx < MAX_PARTITIONS) {
            partitions[partition_idx].drive = drive;
            partitions[partition_idx].partition_num = i;
            partitions[partition_idx].type = part->system_id;
            partitions[partition_idx].start_lba = part->start_lba;
            partitions[partition_idx].total_sectors = part->total_sectors;
            partitions[partition_idx].size_mb = part->total_sectors / 2048; // 512 bytes per sector, 1024*1024 bytes per MB
            
            // Determine file system type
            switch (part->system_id) {
                case PART_TYPE_FAT12:
                case PART_TYPE_FAT16:
                case PART_TYPE_FAT16B:
                case PART_TYPE_FAT16X:
                    partitions[partition_idx].fs_type = FS_TYPE_FAT16;
                    break;
                case PART_TYPE_FAT32:
                case PART_TYPE_FAT32X:
                    partitions[partition_idx].fs_type = FS_TYPE_FAT32;
                    break;
                case PART_TYPE_LINUX:
                    partitions[partition_idx].fs_type = FS_TYPE_EXT2;
                    break;
                default:
                    partitions[partition_idx].fs_type = FS_TYPE_UNKNOWN;
                    break;
            }
            
            partition_count++;
            
            // Print partition info
            const char* type_str;
            switch (part->system_id) {
                case PART_TYPE_FAT12:    type_str = "FAT12"; break;
                case PART_TYPE_FAT16:    type_str = "FAT16"; break;
                case PART_TYPE_EXTENDED: type_str = "Extended"; break;
                case PART_TYPE_FAT16B:   type_str = "FAT16B"; break;
                case PART_TYPE_NTFS:     type_str = "NTFS"; break;
                case PART_TYPE_FAT32:    type_str = "FAT32"; break;
                case PART_TYPE_FAT32X:   type_str = "FAT32X"; break;
                case PART_TYPE_FAT16X:   type_str = "FAT16X"; break;
                case PART_TYPE_LINUX:    type_str = "Linux"; break;
                default:                 type_str = "Unknown"; break;
            }
            
            terminal_printf("%d    %s    %-10s  %-10d  %-10d\n",
                i,
                part->bootable == 0x80 ? "Yes" : "No",
                type_str,
                part->start_lba,
                partitions[partition_idx].size_mb);
        }
    }
    
    kfree(mbr);
    
	if (partition_count == 0) {
        terminal_writestring("No partitions found\n");
    }
    
    return partition_count;
}

// Format a partition with a specific file system
int fs_format_partition(uint8_t drive, uint8_t partition, uint8_t fs_type) {
    // Calculate global partition index
    uint8_t partition_idx = drive * 4 + partition;
    
    if (partition_idx >= MAX_PARTITIONS || partitions[partition_idx].drive != drive) {
        terminal_writestring("Invalid partition specified\n");
        return -1;
    }
    
    partition_info_t* part = &partitions[partition_idx];
    
    // Check if partition is mounted
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].drive == drive && mount_points[i].partition == partition) {
            terminal_writestring("Cannot format a mounted partition\n");
            return -1;
        }
    }
    
    // Format based on file system type
    switch (fs_type) {
        case FS_TYPE_FAT16:
            terminal_writestring("FAT16 formatting not implemented\n");
            return -1;
            
        case FS_TYPE_FAT32:
            terminal_writestring("Formatting partition as FAT32...\n");
            
            // Basic FAT32 formatting
            const uint32_t sectors_per_cluster = 8; // 4KB clusters (8 * 512 bytes)
            
            // Allocate buffer for FAT32 boot sector
            uint8_t* boot_sector = kmalloc(512);
            if (!boot_sector) {
                terminal_writestring("Failed to allocate memory for boot sector\n");
                return -1;
            }
            
            // Clear boot sector
            memset(boot_sector, 0, 512);
            
            // Basic FAT32 parameters
            uint32_t reserved_sectors = 32;
            uint32_t fat_size = (part->total_sectors / sectors_per_cluster + 127) / 128;
            uint32_t root_dir_clusters = 1;
            
            // Setup boot sector fields
            boot_sector[0] = 0xEB; // Jump instruction
            boot_sector[1] = 0x58;
            boot_sector[2] = 0x90;
            
            // BIOS Parameter Block
            memcpy(boot_sector + 3, "MSDOS5.0", 8); // OEM ID
            *(uint16_t*)(boot_sector + 11) = 512;   // Bytes per sector
            boot_sector[13] = sectors_per_cluster;  // Sectors per cluster
            *(uint16_t*)(boot_sector + 14) = reserved_sectors; // Reserved sectors
            boot_sector[16] = 2;   // Number of FATs
            *(uint16_t*)(boot_sector + 17) = 0;     // Root entries (0 for FAT32)
            *(uint16_t*)(boot_sector + 19) = 0;     // Total sectors (16-bit, 0 for large disk)
            boot_sector[21] = 0xF8;                 // Media descriptor (0xF8 = fixed disk)
            *(uint16_t*)(boot_sector + 22) = 0;     // Sectors per FAT (0 for FAT32)
            *(uint16_t*)(boot_sector + 24) = 63;    // Sectors per track
            *(uint16_t*)(boot_sector + 26) = 255;   // Number of heads
            *(uint32_t*)(boot_sector + 28) = part->start_lba; // Hidden sectors
            *(uint32_t*)(boot_sector + 32) = part->total_sectors; // Total sectors (32-bit)
            
            // FAT32 Extended BPB
            *(uint32_t*)(boot_sector + 36) = fat_size;         // Sectors per FAT
            *(uint16_t*)(boot_sector + 40) = 0;                // Flags
            *(uint16_t*)(boot_sector + 42) = 0;                // FAT version
            *(uint32_t*)(boot_sector + 44) = 2;                // Root directory cluster
            *(uint16_t*)(boot_sector + 48) = 1;                // FSInfo sector
            *(uint16_t*)(boot_sector + 50) = 6;                // Backup boot sector
            memset(boot_sector + 52, 0, 12);                   // Reserved
            boot_sector[64] = 0x80;                            // Drive number
            boot_sector[65] = 0;                               // Reserved
            boot_sector[66] = 0x29;                            // Extended boot signature
            *(uint32_t*)(boot_sector + 67) = 0x12345678;       // Volume ID
            memcpy(boot_sector + 71, "NO NAME    ", 11);       // Volume label
            memcpy(boot_sector + 82, "FAT32   ", 8);           // File system type
            
            // Boot sector signature
            boot_sector[510] = 0x55;
            boot_sector[511] = 0xAA;
            
            // Write boot sector
            if (hal_ata_write_sectors(drive, part->start_lba, 1, boot_sector) != 0) {
                terminal_writestring("Failed to write boot sector\n");
                kfree(boot_sector);
                return -1;
            }
            
            // Write backup boot sector at sector 6
            if (hal_ata_write_sectors(drive, part->start_lba + 6, 1, boot_sector) != 0) {
                terminal_writestring("Failed to write backup boot sector\n");
                kfree(boot_sector);
                return -1;
            }
            
            // Create and write FSInfo sector
            memset(boot_sector, 0, 512);
            *(uint32_t*)(boot_sector) = 0x41615252;            // FSInfo signature
            *(uint32_t*)(boot_sector + 484) = 0x61417272;      // Second signature
            *(uint32_t*)(boot_sector + 488) = part->total_sectors - (2 * fat_size + reserved_sectors); // Free clusters count
            *(uint32_t*)(boot_sector + 492) = 3;               // Next free cluster
            *(uint16_t*)(boot_sector + 510) = 0xAA55;          // Signature
            
            if (hal_ata_write_sectors(drive, part->start_lba + 1, 1, boot_sector) != 0) {
                terminal_writestring("Failed to write FSInfo sector\n");
                kfree(boot_sector);
                return -1;
            }
            
            // Backup FSInfo sector
            if (hal_ata_write_sectors(drive, part->start_lba + 7, 1, boot_sector) != 0) {
                terminal_writestring("Failed to write backup FSInfo sector\n");
                kfree(boot_sector);
                return -1;
            }
            
            // Initialize FATs
            memset(boot_sector, 0, 512);
            
            // First FAT sector
            *(uint32_t*)(boot_sector) = 0x0FFFFFF8;      // Media descriptor
            *(uint32_t*)(boot_sector + 4) = 0x0FFFFFFF;  // End of cluster chain
            *(uint32_t*)(boot_sector + 8) = 0x0FFFFFFF;  // End of cluster chain for root directory
            
            // Write first FAT sector
            if (hal_ata_write_sectors(drive, part->start_lba + reserved_sectors, 1, boot_sector) != 0) {
                terminal_writestring("Failed to write FAT\n");
                kfree(boot_sector);
                return -1;
            }
            
            // Write first sector of second FAT
            if (hal_ata_write_sectors(drive, part->start_lba + reserved_sectors + fat_size, 1, boot_sector) != 0) {
                terminal_writestring("Failed to write second FAT\n");
                kfree(boot_sector);
                return -1;
            }
            
            // Clear root directory (cluster 2)
            memset(boot_sector, 0, 512);
            uint32_t root_dir_sector = part->start_lba + reserved_sectors + (2 * fat_size);
            
            for (uint32_t i = 0; i < sectors_per_cluster; i++) {
                if (hal_ata_write_sectors(drive, root_dir_sector + i, 1, boot_sector) != 0) {
                    terminal_writestring("Failed to clear root directory\n");
                    kfree(boot_sector);
                    return -1;
                }
            }
            
            kfree(boot_sector);
            
            terminal_writestring("FAT32 formatting complete\n");
            
            // Update partition file system type
            part->fs_type = FS_TYPE_FAT32;
            return 0;
            
        case FS_TYPE_EXT2:
            terminal_writestring("EXT2 formatting not implemented\n");
            return -1;
            
        default:
            terminal_writestring("Unknown file system type\n");
            return -1;
    }
}

// Mount a partition to a mount point
int fs_mount(const char* mount_point, uint8_t drive, uint8_t partition) {
    // Calculate global partition index
    uint8_t partition_idx = drive * 4 + partition;
    
    if (partition_idx >= MAX_PARTITIONS || partitions[partition_idx].drive != drive) {
        terminal_writestring("Invalid partition specified\n");
        return -1;
    }
    
    partition_info_t* part = &partitions[partition_idx];
    
    // Check if mount point already exists
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].drive != 0 && strcmp(mount_points[i].mount_point, mount_point) == 0) {
            terminal_writestring("Mount point already in use\n");
            return -1;
        }
    }
    
    // Find a free mount point slot
    int mount_idx = -1;
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].drive == 0) {
            mount_idx = i;
            break;
        }
    }
    
    if (mount_idx == -1) {
        terminal_writestring("No free mount points available\n");
        return -1;
    }
    
    // Initialize mount point structure
    strcpy(mount_points[mount_idx].mount_point, mount_point);
    mount_points[mount_idx].drive = drive;
    mount_points[mount_idx].partition = partition;
    mount_points[mount_idx].fs_type = part->fs_type;
    
    // Mount the file system based on type
    switch (part->fs_type) {
        case FS_TYPE_FAT16:
            terminal_writestring("FAT16 mounting not implemented\n");
            mount_points[mount_idx].drive = 0; // Clear on failure
            return -1;
            
        case FS_TYPE_FAT32:
            if (fat32_mount(drive, part->start_lba, part->total_sectors, &mount_points[mount_idx].fs_data) != 0) {
                terminal_writestring("Failed to mount FAT32 file system\n");
                mount_points[mount_idx].drive = 0; // Clear on failure
                return -1;
            }
            break;
            
        case FS_TYPE_EXT2:
            terminal_writestring("EXT2 mounting not implemented\n");
            mount_points[mount_idx].drive = 0; // Clear on failure
            return -1;
            
        default:
            terminal_writestring("Unknown file system type\n");
            mount_points[mount_idx].drive = 0; // Clear on failure
            return -1;
    }
    
    terminal_printf("Mounted %s on %s\n", mount_point, 
        part->fs_type == FS_TYPE_FAT32 ? "FAT32" : 
        part->fs_type == FS_TYPE_FAT16 ? "FAT16" : 
        part->fs_type == FS_TYPE_EXT2 ? "EXT2" : "Unknown");
    
    return 0;
}

// Unmount a partition
int fs_unmount(const char* mount_point) {
    // Find the mount point
    int mount_idx = -1;
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].drive != 0 && strcmp(mount_points[i].mount_point, mount_point) == 0) {
            mount_idx = i;
            break;
        }
    }
    
    if (mount_idx == -1) {
        terminal_writestring("Mount point not found\n");
        return -1;
    }
    
    // Unmount based on file system type
    switch (mount_points[mount_idx].fs_type) {
        case FS_TYPE_FAT32:
            if (fat32_unmount(mount_points[mount_idx].fs_data) != 0) {
                terminal_writestring("Failed to unmount FAT32 file system\n");
                return -1;
            }
            break;
            
        case FS_TYPE_FAT16:
            terminal_writestring("FAT16 unmounting not implemented\n");
            return -1;
            
        case FS_TYPE_EXT2:
            terminal_writestring("EXT2 unmounting not implemented\n");
            return -1;
            
        default:
            terminal_writestring("Unknown file system type\n");
            return -1;
    }
    
    // Clear mount point
    memset(&mount_points[mount_idx], 0, sizeof(mount_point_t));
    terminal_printf("Unmounted %s\n", mount_point);
    
    return 0;
}

// Get partition information
partition_info_t* fs_get_partition(uint8_t drive, uint8_t partition) {
    uint8_t partition_idx = drive * 4 + partition;
    
    if (partition_idx >= MAX_PARTITIONS || partitions[partition_idx].drive != drive) {
        return NULL;
    }
    
    return &partitions[partition_idx];
}

// Get mount point information
mount_point_t* fs_get_mount_point(const char* path) {
    // Find longest matching mount point
    mount_point_t* best_match = NULL;
    int best_match_len = 0;
    
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].drive == 0) {
            continue;
        }
        
        int mount_len = strlen(mount_points[i].mount_point);
        
        // Check if path starts with mount point
        if (strncmp(path, mount_points[i].mount_point, mount_len) == 0) {
            // Must be exact match or followed by '/'
            if (path[mount_len] == '/' || path[mount_len] == '\0') {
                if (mount_len > best_match_len) {
                    best_match = &mount_points[i];
                    best_match_len = mount_len;
                }
            }
        }
    }
    
    return best_match;
}

// Print partition table
void fs_print_partitions(uint8_t drive) {
    ata_device_t* device = hal_ata_get_device(drive);
    if (!device || !device->present) {
        terminal_writestring("Drive not found\n");
        return;
    }
    
    terminal_printf("Partitions on drive %d (%s):\n", drive, device->model);
    terminal_writestring("Num  Boot  Type        Start       Size(MB)\n");
    terminal_writestring("--------------------------------------------\n");
    
    int found = 0;
    
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t partition_idx = drive * 4 + i;
        
        if (partition_idx < MAX_PARTITIONS && partitions[partition_idx].drive == drive) {
            found = 1;
            
            const char* type_str;
            switch (partitions[partition_idx].type) {
                case PART_TYPE_FAT12:    type_str = "FAT12"; break;
                case PART_TYPE_FAT16:    type_str = "FAT16"; break;
                case PART_TYPE_EXTENDED: type_str = "Extended"; break;
                case PART_TYPE_FAT16B:   type_str = "FAT16B"; break;
                case PART_TYPE_NTFS:     type_str = "NTFS"; break;
                case PART_TYPE_FAT32:    type_str = "FAT32"; break;
                case PART_TYPE_FAT32X:   type_str = "FAT32X"; break;
                case PART_TYPE_FAT16X:   type_str = "FAT16X"; break;
                case PART_TYPE_LINUX:    type_str = "Linux"; break;
                default:                 type_str = "Unknown"; break;
            }
            
            terminal_printf("%d    %s    %-10s  %-10d  %-10d\n",
                i,
                "No", // We don't track bootable flag in our structure
                type_str,
                partitions[partition_idx].start_lba,
                partitions[partition_idx].size_mb);
        }
    }
    
    if (!found) {
        terminal_writestring("No partitions found\n");
    }
}

// Create a partition table on a drive
int fs_create_partition_table(uint8_t drive) {
    ata_device_t* device = hal_ata_get_device(drive);
    if (!device || !device->present) {
        terminal_writestring("Drive not found\n");
        return -1;
    }
    
    // Check if any partition on this drive is mounted
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].drive == drive) {
            terminal_writestring("Cannot create partition table on mounted drive\n");
            return -1;
        }
    }
    
    // Allocate buffer for MBR
    mbr_t* mbr = kmalloc(512);
    if (!mbr) {
        terminal_writestring("Failed to allocate memory for MBR\n");
        return -1;
    }
    
    // Clear MBR
    memset(mbr, 0, 512);
    
    // Create bootstrap code (just a simple infinite loop)
    mbr->bootstrap[0] = 0xEB; // JMP
    mbr->bootstrap[1] = 0xFE; // to self (infinite loop)
    
    // Set boot signature
    mbr->signature = 0xAA55;
    
    // Write the MBR
    if (hal_ata_write_sectors(drive, 0, 1, mbr) != 0) {
        terminal_writestring("Failed to write MBR\n");
        kfree(mbr);
        return -1;
    }
    
    kfree(mbr);
    
    // Clear partition entries for this drive
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t partition_idx = drive * 4 + i;
        if (partition_idx < MAX_PARTITIONS) {
            memset(&partitions[partition_idx], 0, sizeof(partition_info_t));
        }
    }
    
    terminal_writestring("Created empty partition table\n");
    return 0;
}

// Add a partition to a drive
int fs_add_partition(uint8_t drive, uint32_t start_lba, uint32_t size_sectors, uint8_t type) {
    ata_device_t* device = hal_ata_get_device(drive);
    if (!device || !device->present) {
        terminal_writestring("Drive not found\n");
        return -1;
    }
    
    // Find free partition slot
    int free_slot = -1;
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t partition_idx = drive * 4 + i;
        if (partition_idx < MAX_PARTITIONS && partitions[partition_idx].drive == 0) {
            free_slot = i;
            break;
        }
    }
    
    if (free_slot == -1) {
        terminal_writestring("No free partition slots available\n");
        return -1;
    }
    
    // Check for overlapping partitions
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t partition_idx = drive * 4 + i;
        if (partition_idx < MAX_PARTITIONS && partitions[partition_idx].drive == drive) {
            uint32_t part_start = partitions[partition_idx].start_lba;
            uint32_t part_end = part_start + partitions[partition_idx].total_sectors - 1;
            
            uint32_t new_end = start_lba + size_sectors - 1;
            
            if ((start_lba >= part_start && start_lba <= part_end) ||
                (new_end >= part_start && new_end <= part_end) ||
                (start_lba <= part_start && new_end >= part_end)) {
                terminal_writestring("Partition overlaps with existing partition\n");
                return -1;
            }
        }
    }
    
    // Allocate buffer for MBR
    mbr_t* mbr = kmalloc(512);
    if (!mbr) {
        terminal_writestring("Failed to allocate memory for MBR\n");
        return -1;
    }
    
    // Read MBR
    if (hal_ata_read_sectors(drive, 0, 1, mbr) != 0) {
        terminal_writestring("Failed to read MBR\n");
        kfree(mbr);
        return -1;
    }
    
    // Check MBR signature
    if (mbr->signature != 0xAA55) {
        terminal_writestring("Invalid MBR signature\n");
        kfree(mbr);
        return -1;
    }
    
    // Set up partition entry
    mbr_partition_t* part = &mbr->partitions[free_slot];
    
    // Convert LBA to CHS (approximate, for compatibility with older systems)
    uint32_t c = start_lba / (16 * 63);
    uint32_t h = (start_lba / 63) % 16;
    uint32_t s = (start_lba % 63) + 1;
    
    part->bootable = 0x00; // Not bootable
    part->start_head = h;
    part->start_sector = s | ((c >> 2) & 0xC0);
    part->start_cylinder = c & 0xFF;
    
    uint32_t end_lba = start_lba + size_sectors - 1;
    c = end_lba / (16 * 63);
    h = (end_lba / 63) % 16;
    s = (end_lba % 63) + 1;
    
    part->end_head = h;
    part->end_sector = s | ((c >> 2) & 0xC0);
    part->end_cylinder = c & 0xFF;
    
    part->system_id = type;
    part->start_lba = start_lba;
    part->total_sectors = size_sectors;
    
    // Write updated MBR
    if (hal_ata_write_sectors(drive, 0, 1, mbr) != 0) {
        terminal_writestring("Failed to write MBR\n");
        kfree(mbr);
        return -1;
    }
    
    kfree(mbr);
    
    // Update partition table in memory
    uint8_t partition_idx = drive * 4 + free_slot;
    partitions[partition_idx].drive = drive;
    partitions[partition_idx].partition_num = free_slot;
    partitions[partition_idx].type = type;
    partitions[partition_idx].start_lba = start_lba;
    partitions[partition_idx].total_sectors = size_sectors;
    partitions[partition_idx].size_mb = size_sectors / 2048;
    
    // Determine file system type
    switch (type) {
        case PART_TYPE_FAT12:
        case PART_TYPE_FAT16:
        case PART_TYPE_FAT16B:
        case PART_TYPE_FAT16X:
            partitions[partition_idx].fs_type = FS_TYPE_FAT16;
            break;
        case PART_TYPE_FAT32:
        case PART_TYPE_FAT32X:
            partitions[partition_idx].fs_type = FS_TYPE_FAT32;
            break;
        case PART_TYPE_LINUX:
            partitions[partition_idx].fs_type = FS_TYPE_EXT2;
            break;
        default:
            partitions[partition_idx].fs_type = FS_TYPE_UNKNOWN;
            break;
    }
    
    terminal_printf("Added partition %d of type %02X, starting at LBA %d, size %d MB\n",
        free_slot, type, start_lba, partitions[partition_idx].size_mb);
    
    return free_slot;
}

// Delete a partition
int fs_delete_partition(uint8_t drive, uint8_t partition) {
    uint8_t partition_idx = drive * 4 + partition;
    
    if (partition_idx >= MAX_PARTITIONS || partitions[partition_idx].drive != drive) {
        terminal_writestring("Invalid partition specified\n");
        return -1;
    }
    
    // Check if partition is mounted
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].drive == drive && mount_points[i].partition == partition) {
            terminal_writestring("Cannot delete a mounted partition\n");
            return -1;
        }
    }
    
    // Allocate buffer for MBR
    mbr_t* mbr = kmalloc(512);
    if (!mbr) {
        terminal_writestring("Failed to allocate memory for MBR\n");
        return -1;
    }
    
    // Read MBR
    if (hal_ata_read_sectors(drive, 0, 1, mbr) != 0) {
        terminal_writestring("Failed to read MBR\n");
        kfree(mbr);
        return -1;
    }
    
    // Clear partition entry
    memset(&mbr->partitions[partition], 0, sizeof(mbr_partition_t));
    
    // Write updated MBR
    if (hal_ata_write_sectors(drive, 0, 1, mbr) != 0) {
        terminal_writestring("Failed to write MBR\n");
        kfree(mbr);
        return -1;
    }
    
    kfree(mbr);
    
    // Clear partition in memory
    memset(&partitions[partition_idx], 0, sizeof(partition_info_t));
    
    terminal_printf("Deleted partition %d\n", partition);
    return 0;
}

// Stub for FAT32 mounting - actual implementation would be more complex
int fat32_mount(uint8_t drive, uint32_t start_lba, uint32_t sectors, void** fs_data) {
    // Simplified FAT32 mount implementation - just for demonstration
    // In a real driver, this would read and validate FAT32 structures
    
    // Allocate structure for FAT32 file system data
    typedef struct {
        uint8_t drive;
        uint32_t start_lba;
        uint32_t sectors;
        uint32_t reserved_sectors;
        uint32_t sectors_per_cluster;
        uint32_t root_dir_cluster;
        uint32_t fat_size;
        uint32_t first_data_sector;
    } fat32_fs_data_t;
    
    fat32_fs_data_t* fat_data = kmalloc(sizeof(fat32_fs_data_t));
    if (!fat_data) {
        terminal_writestring("Failed to allocate memory for FAT32 data\n");
        return -1;
    }
    
    // Read boot sector
    uint8_t* boot_sector = kmalloc(512);
    if (!boot_sector) {
        terminal_writestring("Failed to allocate memory for boot sector\n");
        kfree(fat_data);
        return -1;
    }
    
    if (hal_ata_read_sectors(drive, start_lba, 1, boot_sector) != 0) {
        terminal_writestring("Failed to read boot sector\n");
        kfree(boot_sector);
        kfree(fat_data);
        return -1;
    }
    
    // Basic validation of FAT32 signature
    if (boot_sector[510] != 0x55 || boot_sector[511] != 0xAA) {
        terminal_writestring("Invalid boot sector signature\n");
        kfree(boot_sector);
        kfree(fat_data);
        return -1;
    }
    
    // Extract basic FAT32 parameters
    fat_data->drive = drive;
    fat_data->start_lba = start_lba;
    fat_data->sectors = sectors;
    fat_data->reserved_sectors = *(uint16_t*)(boot_sector + 14);
    fat_data->sectors_per_cluster = boot_sector[13];
    fat_data->root_dir_cluster = *(uint32_t*)(boot_sector + 44);
    fat_data->fat_size = *(uint32_t*)(boot_sector + 36);
    
    // Calculate first data sector
    fat_data->first_data_sector = fat_data->reserved_sectors + (2 * fat_data->fat_size);
    
    kfree(boot_sector);
    
    *fs_data = fat_data;
    return 0;
}

// Stub for FAT32 unmounting
int fat32_unmount(void* fs_data) {
    if (fs_data) {
        kfree(fs_data);
    }
    return 0;
}

// These are stub implementations - in a real FAT32 driver, these would be fully implemented
int fat32_read_file(void* fs_data, const char* path, void* buffer, size_t size) {
    terminal_writestring("FAT32 read_file not implemented\n");
    return -1;
}

int fat32_write_file(void* fs_data, const char* path, const void* buffer, size_t size) {
    terminal_writestring("FAT32 write_file not implemented\n");
    return -1;
}

int fat32_create_file(void* fs_data, const char* path) {
    terminal_writestring("FAT32 create_file not implemented\n");
    return -1;
}

int fat32_delete_file(void* fs_data, const char* path) {
	terminal_writestring("FAT32 delete_file not implemented\n");
	return -1;
 }
 
 int fat32_create_directory(void* fs_data, const char* path) {
	terminal_writestring("FAT32 create_directory not implemented\n");
	return -1;
 }
 
 int fat32_delete_directory(void* fs_data, const char* path) {
	terminal_writestring("FAT32 delete_directory not implemented\n");
	return -1;
 }
 
 int fat32_list_directory(void* fs_data, const char* path) {
	terminal_writestring("FAT32 list_directory not implemented\n");
	return -1;
 }
 
 int fat32_get_file_size(void* fs_data, const char* path) {
	terminal_writestring("FAT32 get_file_size not implemented\n");
	return -1;
 }