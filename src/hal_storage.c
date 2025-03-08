// src/hal_storage.c
#include "hal.h"
#include "terminal.h"
#include "stdio.h"
#include "kmalloc.h"
#include "string.h"
#include "io.h"
#include <stdint.h>

// Storage types
#define STORAGE_TYPE_RAM_DISK    0
#define STORAGE_TYPE_ATA_DISK    1  // For future implementation
#define STORAGE_TYPE_ATAPI_CDROM 2  // For future implementation
#define STORAGE_TYPE_FLOPPY      3  // For future implementation

// Storage device private data
typedef struct {
    uint8_t storage_type;         // Type of storage device
    uint8_t device_number;        // Device number/ID
    uint32_t sector_size;         // Size of a sector in bytes
    uint32_t total_sectors;       // Total number of sectors
    uint32_t read_count;          // Number of successful reads
    uint32_t write_count;         // Number of successful writes
    uint32_t error_count;         // Number of errors
    uint8_t* storage_buffer;      // Pointer to storage buffer (for RAM disk)
    uint32_t buffer_size;         // Size of storage buffer
} storage_data_t;

// Local storage device
static storage_data_t storage_data = {0};
static hal_device_t storage_device = {0};

// RAM Disk functions
static int ramdisk_read_sector(uint32_t sector, uint8_t* buffer) {
    storage_data_t* data = &storage_data;
    
    // Check if sector is valid
    if (sector >= data->total_sectors) {
        return -1;
    }
    
    // Calculate offset into RAM disk
    uint32_t offset = sector * data->sector_size;
    
    // Copy data from RAM disk to buffer
    memcpy(buffer, data->storage_buffer + offset, data->sector_size);
    
    // Update statistics
    data->read_count++;
    
    return 0;
}

static int ramdisk_write_sector(uint32_t sector, const uint8_t* buffer) {
    storage_data_t* data = &storage_data;
    
    // Check if sector is valid
    if (sector >= data->total_sectors) {
        return -1;
    }
    
    // Calculate offset into RAM disk
    uint32_t offset = sector * data->sector_size;
    
    // Copy data from buffer to RAM disk
    memcpy(data->storage_buffer + offset, buffer, data->sector_size);
    
    // Update statistics
    data->write_count++;
    
    return 0;
}

// ATA Disk functions (stubs for future implementation)
static int ata_read_sector(uint8_t device, uint32_t sector, uint8_t* buffer) {
    // This would involve sending commands to the ATA controller
    // For now, just return an error
    return -1;
}

static int ata_write_sector(uint8_t device, uint32_t sector, const uint8_t* buffer) {
    // This would involve sending commands to the ATA controller
    // For now, just return an error
    return -1;
}

// Generic storage functions based on device type
static int storage_read_sector(uint32_t sector, uint8_t* buffer) {
    storage_data_t* data = &storage_data;
    
    switch (data->storage_type) {
        case STORAGE_TYPE_RAM_DISK:
            return ramdisk_read_sector(sector, buffer);
        case STORAGE_TYPE_ATA_DISK:
            return ata_read_sector(data->device_number, sector, buffer);
        default:
            data->error_count++;
            return -1;
    }
}

static int storage_write_sector(uint32_t sector, const uint8_t* buffer) {
    storage_data_t* data = &storage_data;
    
    switch (data->storage_type) {
        case STORAGE_TYPE_RAM_DISK:
            return ramdisk_write_sector(sector, buffer);
        case STORAGE_TYPE_ATA_DISK:
            return ata_write_sector(data->device_number, sector, buffer);
        default:
            data->error_count++;
            return -1;
    }
}

// Device-specific functions
static int storage_init(void* device) {
    hal_device_t* dev = (hal_device_t*)device;
    storage_data_t* data = (storage_data_t*)dev->private_data;
    
    // Initialize RAM disk
    if (data->storage_type == STORAGE_TYPE_RAM_DISK) {
        // Default RAM disk configuration if not already set
        if (data->sector_size == 0) {
            data->sector_size = 512;  // Standard sector size
        }
        
        if (data->total_sectors == 0) {
            data->total_sectors = 4096;  // 2MB RAM disk
        }
        
        // Calculate buffer size
        data->buffer_size = data->sector_size * data->total_sectors;
        
        // Allocate RAM disk buffer if not already allocated
        if (!data->storage_buffer) {
            data->storage_buffer = kmalloc(data->buffer_size);
            if (!data->storage_buffer) {
                terminal_writestring("Failed to allocate RAM disk buffer\n");
                return -1;
            }
            
            // Clear the buffer
            memset(data->storage_buffer, 0, data->buffer_size);
        }
        
        terminal_printf("RAM Disk initialized: %d MB, %d sectors of %d bytes\n", 
                      data->buffer_size / (1024 * 1024),
                      data->total_sectors,
                      data->sector_size);
    } 
    // For future implementations of other storage types
    else {
        terminal_writestring("Unsupported storage type\n");
        return -1;
    }
    
    // Reset statistics
    data->read_count = 0;
    data->write_count = 0;
    data->error_count = 0;
    
    return 0;
}

static int storage_close(void* device) {
    hal_device_t* dev = (hal_device_t*)device;
    storage_data_t* data = (storage_data_t*)dev->private_data;
    
    // Free RAM disk buffer if allocated
    if (data->storage_type == STORAGE_TYPE_RAM_DISK && data->storage_buffer) {
        kfree(data->storage_buffer);
        data->storage_buffer = NULL;
    }
    
    return 0;
}

static int storage_read(void* device, void* buffer, uint32_t size) {
    hal_device_t* dev = (hal_device_t*)device;
    storage_data_t* data = (storage_data_t*)dev->private_data;
    
    // Read is more complex because we need to know the sector
    // This function is meant for the fs layer to use directly
    // For now, we'll just return an error to indicate direct read
    // should not be used
    return -1;
}

static int storage_write(void* device, const void* buffer, uint32_t size) {
    // Similar to read, this is not meant for direct use
    return -1;
}

// More complex operations are supported through IOCTL
static int storage_ioctl(void* device, uint32_t request, void* arg) {
    hal_device_t* dev = (hal_device_t*)device;
    storage_data_t* data = (storage_data_t*)dev->private_data;
    
    switch (request) {
        case 0: // Get sector size
            if (arg) {
                *((uint32_t*)arg) = data->sector_size;
                return 0;
            }
            break;
            
        case 1: // Get total sectors
            if (arg) {
                *((uint32_t*)arg) = data->total_sectors;
                return 0;
            }
            break;
            
        case 2: // Read sector
            if (arg) {
                struct {
                    uint32_t sector;
                    uint8_t* buffer;
                } *read_args = (void*)arg;
                
                return storage_read_sector(read_args->sector, read_args->buffer);
            }
            break;
            
        case 3: // Write sector
            if (arg) {
                struct {
                    uint32_t sector;
                    const uint8_t* buffer;
                } *write_args = (void*)arg;
                
                return storage_write_sector(write_args->sector, write_args->buffer);
            }
            break;
            
        case 4: // Get statistics
            if (arg) {
                struct {
                    uint32_t read_count;
                    uint32_t write_count;
                    uint32_t error_count;
                } *stats = (void*)arg;
                
                stats->read_count = data->read_count;
                stats->write_count = data->write_count;
                stats->error_count = data->error_count;
                return 0;
            }
            break;
            
        case 5: // Get device size in bytes
            if (arg) {
                *((uint64_t*)arg) = (uint64_t)data->sector_size * data->total_sectors;
                return 0;
            }
            break;
    }
    
    return -1;
}

// HAL storage interface functions
uint32_t hal_storage_get_sector_size(void) {
    return storage_data.sector_size;
}

uint32_t hal_storage_get_total_sectors(void) {
    return storage_data.total_sectors;
}

int hal_storage_read_sector(uint32_t sector, void* buffer) {
    return storage_read_sector(sector, buffer);
}

int hal_storage_write_sector(uint32_t sector, const void* buffer) {
    return storage_write_sector(sector, (const uint8_t*)buffer);
}

int hal_storage_read(uint32_t offset, void* buffer, uint32_t size) {
    // Convert offset to sector and offset within sector
    uint32_t sector = offset / storage_data.sector_size;
    uint32_t sector_offset = offset % storage_data.sector_size;
    
    // Calculate number of sectors to read
    uint32_t remaining = size;
    uint32_t buf_offset = 0;
    uint8_t* buf_ptr = (uint8_t*)buffer;
    
    // If offset is not sector-aligned, handle first partial sector
    if (sector_offset > 0) {
        uint8_t temp_buffer[512];  // Maximum sector size
        
        // Read the full sector
        if (storage_read_sector(sector, temp_buffer) != 0) {
            return -1;
        }
        
        // Copy the relevant portion
        uint32_t bytes_to_copy = storage_data.sector_size - sector_offset;
        if (bytes_to_copy > remaining) {
            bytes_to_copy = remaining;
        }
        
        memcpy(buf_ptr, temp_buffer + sector_offset, bytes_to_copy);
        
        // Update pointers and counters
        buf_ptr += bytes_to_copy;
        remaining -= bytes_to_copy;
        sector++;
    }
    
    // Read full sectors
    while (remaining >= storage_data.sector_size) {
        if (storage_read_sector(sector, buf_ptr) != 0) {
            return -1;
        }
        
        buf_ptr += storage_data.sector_size;
        remaining -= storage_data.sector_size;
        sector++;
    }
    
    // Handle last partial sector if needed
    if (remaining > 0) {
        uint8_t temp_buffer[512];  // Maximum sector size
        
        // Read the full sector
        if (storage_read_sector(sector, temp_buffer) != 0) {
            return -1;
        }
        
        // Copy the relevant portion
        memcpy(buf_ptr, temp_buffer, remaining);
    }
    
    return size;
}

int hal_storage_write(uint32_t offset, const void* buffer, uint32_t size) {
    // Convert offset to sector and offset within sector
    uint32_t sector = offset / storage_data.sector_size;
    uint32_t sector_offset = offset % storage_data.sector_size;
    
    // Calculate number of sectors to write
    uint32_t remaining = size;
    uint32_t buf_offset = 0;
    const uint8_t* buf_ptr = (const uint8_t*)buffer;
    
    // If offset is not sector-aligned, handle first partial sector
    if (sector_offset > 0) {
        uint8_t temp_buffer[512];  // Maximum sector size
        
        // Read the full sector first (read-modify-write)
        if (storage_read_sector(sector, temp_buffer) != 0) {
            return -1;
        }
        
        // Copy the relevant portion
        uint32_t bytes_to_copy = storage_data.sector_size - sector_offset;
        if (bytes_to_copy > remaining) {
            bytes_to_copy = remaining;
        }
        
        memcpy(temp_buffer + sector_offset, buf_ptr, bytes_to_copy);
        
        // Write back the modified sector
        if (storage_write_sector(sector, temp_buffer) != 0) {
            return -1;
        }
        
        // Update pointers and counters
        buf_ptr += bytes_to_copy;
        remaining -= bytes_to_copy;
        sector++;
    }
    
    // Write full sectors
    while (remaining >= storage_data.sector_size) {
        if (storage_write_sector(sector, buf_ptr) != 0) {
            return -1;
        }
        
        buf_ptr += storage_data.sector_size;
        remaining -= storage_data.sector_size;
        sector++;
    }
    
    // Handle last partial sector if needed
    if (remaining > 0) {
        uint8_t temp_buffer[512];  // Maximum sector size
        
        // Read the full sector first (read-modify-write)
        if (storage_read_sector(sector, temp_buffer) != 0) {
            return -1;
        }
        
        // Copy the relevant portion
        memcpy(temp_buffer, buf_ptr, remaining);
        
        // Write back the modified sector
        if (storage_write_sector(sector, temp_buffer) != 0) {
            return -1;
        }
    }
    
    return size;
}

// Get storage statistics
void hal_storage_get_stats(uint32_t* reads, uint32_t* writes, uint32_t* errors) {
    if (reads) *reads = storage_data.read_count;
    if (writes) *writes = storage_data.write_count;
    if (errors) *errors = storage_data.error_count;
}

// Clear storage (zero all sectors)
int hal_storage_clear(void) {
    storage_data_t* data = &storage_data;
    
    if (data->storage_type == STORAGE_TYPE_RAM_DISK) {
        // Clear the entire buffer at once
        memset(data->storage_buffer, 0, data->buffer_size);
        return 0;
    }
    
    // For other storage types, clear sector by sector
    uint8_t zero_buffer[512];  // Maximum sector size
    memset(zero_buffer, 0, sizeof(zero_buffer));
    
    for (uint32_t sector = 0; sector < data->total_sectors; sector++) {
        if (storage_write_sector(sector, zero_buffer) != 0) {
            return -1;
        }
    }
    
    return 0;
}

// Fill a disk region with a pattern
int hal_storage_fill_pattern(uint32_t start_sector, uint32_t num_sectors, 
                             uint8_t pattern) {
    storage_data_t* data = &storage_data;
    
    // Validate parameters
    if (start_sector + num_sectors > data->total_sectors) {
        return -1;
    }
    
    // For RAM disk, fill directly
    if (data->storage_type == STORAGE_TYPE_RAM_DISK) {
        uint32_t offset = start_sector * data->sector_size;
        uint32_t size = num_sectors * data->sector_size;
        memset(data->storage_buffer + offset, pattern, size);
        return 0;
    }
    
    // For other storage types, fill sector by sector
    uint8_t pattern_buffer[512];  // Maximum sector size
    memset(pattern_buffer, pattern, sizeof(pattern_buffer));
    
    for (uint32_t sector = start_sector; sector < start_sector + num_sectors; sector++) {
        if (storage_write_sector(sector, pattern_buffer) != 0) {
            return -1;
        }
    }
    
    return 0;
}

// Create a simple file system structure on the disk
int hal_storage_create_test_filesystem(void) {
    storage_data_t* data = &storage_data;
    
    // Validate storage is available
    if (data->storage_type != STORAGE_TYPE_RAM_DISK || !data->storage_buffer) {
        return -1;
    }
    
    // Create a simple test structure in the first few sectors
    
    // Sector 0: Boot sector with signature
    uint8_t boot_sector[512];
    memset(boot_sector, 0, sizeof(boot_sector));
    memcpy(boot_sector, "HEXTRIX-FS", 10);  // Signature
    memcpy(boot_sector + 10, "TEST-DISK", 9);  // Disk label
    memcpy(boot_sector + 510, "\x55\xAA", 2);  // Boot signature
    if (storage_write_sector(0, boot_sector) != 0) {
        return -1;
    }
    
    // Sector 1: Superblock
    uint8_t superblock[512];
    memset(superblock, 0, sizeof(superblock));
    
    // Simple superblock structure
    struct {
        char signature[8];
        uint32_t version;
        uint32_t block_size;
        uint32_t total_blocks;
        uint32_t free_blocks;
        uint32_t root_dir_block;
        uint32_t reserved[10];
    } *sb = (void*)superblock;
    
    memcpy(sb->signature, "HTRXFS", 6);
    sb->version = 1;
    sb->block_size = data->sector_size;
    sb->total_blocks = data->total_sectors;
    sb->free_blocks = data->total_sectors - 10;  // Reserve first 10 sectors
    sb->root_dir_block = 2;
    
    if (storage_write_sector(1, superblock) != 0) {
        return -1;
    }
    
    // Sector 2: Root directory
    uint8_t root_dir[512];
    memset(root_dir, 0, sizeof(root_dir));
    
    // Simple directory entry structure
    struct {
        char name[32];
        uint32_t size;
        uint32_t first_block;
        uint32_t attributes;
        uint32_t reserved[2];
    } *entry;
    
    // Add "." entry (current directory)
    entry = (void*)root_dir;
    strcpy(entry->name, ".");
    entry->size = data->sector_size;
    entry->first_block = 2;
    entry->attributes = 0x10;  // Directory
    
    // Add ".." entry (parent directory, same as current for root)
    entry = (void*)(root_dir + 48);
    strcpy(entry->name, "..");
    entry->size = data->sector_size;
    entry->first_block = 2;
    entry->attributes = 0x10;  // Directory
    
    // Add a README.TXT file
    entry = (void*)(root_dir + 96);
    strcpy(entry->name, "README.TXT");
    entry->size = 100;  // 100 bytes
    entry->first_block = 3;
    entry->attributes = 0x20;  // File
    
    if (storage_write_sector(2, root_dir) != 0) {
        return -1;
    }
    
    // Sector 3: README.TXT file content
    uint8_t readme[512];
    memset(readme, 0, sizeof(readme));
    
    const char* readme_text = 
        "Welcome to the Hextrix OS RAM Disk!\n\n"
        "This is a simple test file system structure created\n"
        "to demonstrate the storage driver functionality.\n";
    
    memcpy(readme, readme_text, strlen(readme_text));
    
    if (storage_write_sector(3, readme) != 0) {
        return -1;
    }
    
    terminal_writestring("Created test file system structure on RAM disk\n");
    return 0;
}

// Dump disk contents (for debugging)
void hal_storage_dump(uint32_t start_sector, uint32_t num_sectors) {
    uint8_t buffer[512];  // Maximum sector size
    
    for (uint32_t sector = start_sector; 
         sector < start_sector + num_sectors && sector < storage_data.total_sectors; 
         sector++) {
        
        if (storage_read_sector(sector, buffer) == 0) {
            terminal_printf("Sector %d:\n", sector);
            
            // Dump in hex format with ASCII on the side
            for (uint32_t i = 0; i < storage_data.sector_size; i += 16) {
                terminal_printf("%04x: ", i);
                
                // Hex values
                for (uint32_t j = 0; j < 16 && i + j < storage_data.sector_size; j++) {
                    terminal_printf("%02x ", buffer[i + j]);
                }
                
                // Padding if less than 16 bytes
                for (uint32_t j = 0; i + j >= storage_data.sector_size && j < 16; j++) {
                    terminal_writestring("   ");
                }
                
                // ASCII representation
                terminal_writestring(" |");
                for (uint32_t j = 0; j < 16 && i + j < storage_data.sector_size; j++) {
                    char c = buffer[i + j];
                    if (c >= 32 && c <= 126) {
                        terminal_putchar(c);
                    } else {
                        terminal_putchar('.');
                    }
                }
                terminal_writestring("|\n");
                
                // Don't dump too much at once to avoid flooding the terminal
                if (i >= 128 && i < storage_data.sector_size - 16) {
                    terminal_writestring("...\n");
                    break;
                }
            }
            
            terminal_writestring("\n");
        } else {
            terminal_printf("Error reading sector %d\n", sector);
        }
    }
}

// Initialize and register storage device
int hal_storage_init(void) {
    // Setup device
    storage_device.type = HAL_DEVICE_STORAGE;
    storage_device.mode = HAL_MODE_POLLING;
    storage_device.private_data = &storage_data;
    
    // Set default RAM disk parameters
    storage_data.storage_type = STORAGE_TYPE_RAM_DISK;
    storage_data.device_number = 0;
    storage_data.sector_size = 512;
    storage_data.total_sectors = 8192;  // 4MB RAM disk (512 * 8192)
    
    // Set functions
    storage_device.init = storage_init;
    storage_device.close = storage_close;
    storage_device.read = storage_read;
    storage_device.write = storage_write;
    storage_device.ioctl = storage_ioctl;
    
    // Register with HAL
    int result = hal_register_device(&storage_device);
    if (result != 0) {
        return result;
    }
    
    // Initialize the device
    result = storage_init(&storage_device);
    if (result != 0) {
        return result;
    }
    
    // Create a test file system
    hal_storage_create_test_filesystem();
    
    return 0;
}