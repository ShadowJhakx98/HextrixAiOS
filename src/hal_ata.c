// include/hal_ata.h
// Keep your existing header file, no changes needed there

// src/hal_ata.c
#include "hal_ata.h"
#include "io.h"
#include "terminal.h"
#include "stdio.h"
#include "string.h"

// Maximum number of ATA devices
#define ATA_MAX_DEVICES 4

// Array of ATA devices
static ata_device_t ata_devices[ATA_MAX_DEVICES];

// Delay function - QEMU sometimes needs longer delays
static void ata_delay(uint16_t port) {
    // Reading the status port 4 times causes ~400ns delay
    inb(port + 7);
    inb(port + 7);
    inb(port + 7);
    inb(port + 7);
}

// Reset ATA controller
static void ata_reset_controller(uint8_t channel) {
    uint16_t control_port = (channel == 0) ? ATA_PRIMARY_CONTROL : ATA_SECONDARY_CONTROL;
    
    // Send reset signal
    outb(control_port, 0x04); // Set bit 2 (SRST)
    ata_delay(control_port);
    
    // Clear reset signal
    outb(control_port, 0x00);
    ata_delay(control_port);
}

// Select a drive
static void ata_select_drive(uint16_t base_port, uint8_t drive) {
    outb(base_port + 6, 0xA0 | (drive << 4)); // 0xA0 = 1010xxxx, bit 5 set for LBA mode
    ata_delay(base_port);
}

// Wait for BSY flag to clear with timeout
static int ata_wait_not_busy(uint16_t base_port, int timeout_ms) {
    uint8_t status;
    int timeout = timeout_ms * 1000; // Convert to µs
    
    while (timeout > 0) {
        status = inb(base_port + 7);
        if (!(status & ATA_STATUS_BSY))
            return 0; // Not busy
        
        // Delay ~1µs
        ata_delay(base_port);
        timeout -= 400; // Approximate delay of ata_delay
    }
    
    return -1; // Timeout
}

// Wait for DRQ flag to set with timeout
static int ata_wait_drq(uint16_t base_port, int timeout_ms) {
    uint8_t status;
    int timeout = timeout_ms * 1000; // Convert to µs
    
    while (timeout > 0) {
        status = inb(base_port + 7);
        if (status & ATA_STATUS_DRQ)
            return 0; // DRQ set
        
        // Check if there's an error
        if (status & ATA_STATUS_ERR) {
            uint8_t error = inb(base_port + 1);
            terminal_printf("ATA Error: 0x%x (%s)\n", error, hal_ata_get_error_string(error));
            return -1;
        }
        
        // Delay ~1µs
        ata_delay(base_port);
        timeout -= 400; // Approximate delay of ata_delay
    }
    
    return -1; // Timeout
}

// Send ATA command
static void ata_send_command(uint16_t base_port, uint8_t command) {
    outb(base_port + 7, command);
    ata_delay(base_port);
}

// Extract string from identify data
static void ata_extract_string(uint16_t* identify_data, int start_word, int word_count, char* output, int output_size) {
    int i, j;
    
    // ATA strings are stored as big-endian words, need to swap bytes
    for (i = 0, j = 0; i < word_count && j < output_size - 1; i++) {
        uint16_t word = identify_data[start_word + i];
        
        // High byte first (weird ATA string format)
        output[j++] = (char)(word >> 8);
        
        // Then low byte
        if (j < output_size - 1)
            output[j++] = (char)(word & 0xFF);
    }
    
    // Null terminate
    output[j] = '\0';
    
    // Trim trailing spaces
    while (j > 0 && output[j-1] == ' ')
        output[--j] = '\0';
}

// Initialize ATA subsystem
void hal_ata_init(void) {
    terminal_writestring("Initializing ATA disk controller...\n");
    
    // Clear device structures
    memset(ata_devices, 0, sizeof(ata_devices));
    
    // Reset both controllers
    ata_reset_controller(0);
    ata_reset_controller(1);
    
    // Detect connected devices
    int device_count = hal_ata_detect_devices();
    
    terminal_printf("ATA: Found %d device(s)\n", device_count);
}

// Probe for ATA device using IDENTIFY command
static int ata_identify_device(uint16_t base_port, uint8_t drive, uint16_t* identify_data) {
    // Select the drive
    ata_select_drive(base_port, drive);
    
    // Ensure drive is ready
    if (ata_wait_not_busy(base_port, 500) != 0) {
        terminal_printf("  Timeout waiting for drive to be ready\n");
        return -1;
    }
    
    // Zero sector count and LBA registers
    outb(base_port + 2, 0);
    outb(base_port + 3, 0);
    outb(base_port + 4, 0);
    outb(base_port + 5, 0);
    
    // Send IDENTIFY command
    ata_send_command(base_port, ATA_CMD_IDENTIFY);
    
    // Check status
    uint8_t status = inb(base_port + 7);
    
    // If status is 0, no device is present
    if (status == 0) {
        return -1;
    }
    
    // Wait for BSY to clear
    if (ata_wait_not_busy(base_port, 1000) != 0) {
        terminal_printf("  IDENTIFY timeout waiting for BSY to clear\n");
        return -1;
    }
    
    // Check for ATAPI device by reading LBA mid and high
    uint8_t lba_mid = inb(base_port + 4);
    uint8_t lba_high = inb(base_port + 5);
    
    if (lba_mid != 0 || lba_high != 0) {
        // This is likely an ATAPI device, we'll skip it for now
        terminal_printf("  ATAPI device detected (not supported)\n");
        return -1;
    }
    
    // Wait for DRQ to set
    if (ata_wait_drq(base_port, 1000) != 0) {
        terminal_printf("  IDENTIFY timeout waiting for DRQ to set\n");
        return -1;
    }
    
    // Read IDENTIFY data (256 words = 512 bytes)
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(base_port);
    }
    
    return 0;
}

// Detect ATA devices
int hal_ata_detect_devices(void) {
    int count = 0;
    uint16_t base_ports[2] = {ATA_PRIMARY_DATA, ATA_SECONDARY_DATA};
    
    terminal_writestring("Starting ATA device detection...\n");
    
    uint16_t identify_data[256];
    
    // Check each channel and drive
    for (uint8_t channel = 0; channel < 2; channel++) {
        terminal_printf("Checking %s channel...\n", (channel == 0) ? "primary" : "secondary");
        
        uint16_t base_port = base_ports[channel];
        
        for (uint8_t drive = 0; drive < 2; drive++) {
            terminal_printf("  Checking %s drive...\n", (drive == 0) ? "master" : "slave");
            
            uint8_t device_index = channel * 2 + drive;
            
            // Try to identify the device
            if (ata_identify_device(base_port, drive, identify_data) == 0) {
                // Device found, fill in basic info
                ata_devices[device_index].present = 1;
                ata_devices[device_index].type = ATA_DEVICE_TYPE_ATA;
                ata_devices[device_index].channel = channel;
                ata_devices[device_index].drive = drive;
                
                // Extract model string (words 27-46)
                ata_extract_string(identify_data, 27, 20, ata_devices[device_index].model, sizeof(ata_devices[device_index].model));
                
                // Extract serial number (words 10-19)
                ata_extract_string(identify_data, 10, 10, ata_devices[device_index].serial, sizeof(ata_devices[device_index].serial));
                
                // Extract size in sectors
                if (identify_data[83] & (1 << 10)) {
                    // LBA48 supported - limit to 32-bit for now
                    ata_devices[device_index].lba_mode = 2;
                    ata_devices[device_index].size = 
                        ((uint32_t)identify_data[100]) |
                        ((uint32_t)identify_data[101] << 16);
                } else if (identify_data[83] & (1 << 9) || identify_data[49] & (1 << 9)) {
                    // LBA28 supported
                    ata_devices[device_index].lba_mode = 1;
                    ata_devices[device_index].size = 
                        ((uint32_t)identify_data[60]) |
                        ((uint32_t)identify_data[61] << 16);
                } else {
                    // CHS only
                    ata_devices[device_index].lba_mode = 0;
                    uint16_t cylinders = identify_data[1];
                    uint16_t heads = identify_data[3];
                    uint16_t sectors_per_track = identify_data[6];
                    ata_devices[device_index].size = 
                        (uint32_t)cylinders * heads * sectors_per_track;
                }
                
                // Device found
                count++;
                
                terminal_printf("ATA: Found %s drive on %s channel, %s, %d MB\n",
                    (drive == 0) ? "master" : "slave",
                    (channel == 0) ? "primary" : "secondary",
                    ata_devices[device_index].model,
                    ata_devices[device_index].size / 2048);
            } else {
                terminal_printf("  No device detected\n");
            }
        }
    }
    
    return count;
}

// Read sectors using LBA28
int hal_ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, void* buffer) {
    if (drive >= ATA_MAX_DEVICES || !ata_devices[drive].present)
        return -1;
    
    if (sector_count == 0)
        return 0;
    
    uint16_t base_port = (ata_devices[drive].channel == 0) ? 
                       ATA_PRIMARY_DATA : ATA_SECONDARY_DATA;
    
    // Wait for drive to be ready
    if (ata_wait_not_busy(base_port, 500) != 0)
        return -1;
    
    // Select drive and set up LBA addressing
    outb(base_port + 6, 0xE0 | (ata_devices[drive].drive << 4) | ((lba >> 24) & 0x0F));
    
    // Send parameters
    outb(base_port + 2, sector_count);
    outb(base_port + 3, lba & 0xFF);
    outb(base_port + 4, (lba >> 8) & 0xFF);
    outb(base_port + 5, (lba >> 16) & 0xFF);
    
    // Send command
    ata_send_command(base_port, ATA_CMD_READ_PIO);
    
    // Read data
    uint16_t* buf = (uint16_t*)buffer;
    
    for (int i = 0; i < sector_count; i++) {
        // Wait for data to be ready
        if (ata_wait_not_busy(base_port, 500) != 0 || ata_wait_drq(base_port, 500) != 0)
            return -1;
        
        // Read one sector (256 words = 512 bytes)
        for (int j = 0; j < 256; j++) {
            buf[i * 256 + j] = inw(base_port);
        }
    }
    
    return 0;
}

// Write sectors using LBA28
int hal_ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, const void* buffer) {
    if (drive >= ATA_MAX_DEVICES || !ata_devices[drive].present)
        return -1;
    
    if (sector_count == 0)
        return 0;
    
    uint16_t base_port = (ata_devices[drive].channel == 0) ? 
                       ATA_PRIMARY_DATA : ATA_SECONDARY_DATA;
    
    // Wait for drive to be ready
    if (ata_wait_not_busy(base_port, 500) != 0)
        return -1;
    
    // Select drive and set up LBA addressing
    outb(base_port + 6, 0xE0 | (ata_devices[drive].drive << 4) | ((lba >> 24) & 0x0F));
    
    // Send parameters
    outb(base_port + 2, sector_count);
    outb(base_port + 3, lba & 0xFF);
    outb(base_port + 4, (lba >> 8) & 0xFF);
    outb(base_port + 5, (lba >> 16) & 0xFF);
    
    // Send command
    ata_send_command(base_port, ATA_CMD_WRITE_PIO);
    
    // Write data
    const uint16_t* buf = (const uint16_t*)buffer;
    
    for (int i = 0; i < sector_count; i++) {
        // Wait for drive to be ready to accept data
        if (ata_wait_drq(base_port, 500) != 0)
            return -1;
        
        // Write one sector (256 words = 512 bytes)
        for (int j = 0; j < 256; j++) {
            outw(base_port, buf[i * 256 + j]);
        }
        
        // Flush the buffer
        ata_send_command(base_port, ATA_CMD_CACHE_FLUSH);
        
        // Wait for completion
        if (ata_wait_not_busy(base_port, 500) != 0)
            return -1;
    }
    
    return 0;
}

// Get ATA device information
ata_device_t* hal_ata_get_device(uint8_t drive) {
    if (drive >= ATA_MAX_DEVICES || !ata_devices[drive].present)
        return NULL;
    
    return &ata_devices[drive];
}

// Print ATA device information
void hal_ata_print_info(void) {
    terminal_writestring("ATA Drive Information:\n");
    terminal_writestring("-----------------------\n");
    
    int found = 0;
    
    for (int i = 0; i < ATA_MAX_DEVICES; i++) {
        if (ata_devices[i].present) {
            found = 1;
            terminal_printf("Drive %d:\n", i);
            terminal_printf("  Model: %s\n", ata_devices[i].model);
            terminal_printf("  Serial: %s\n", ata_devices[i].serial);
            terminal_printf("  Type: %s\n", 
                (ata_devices[i].type == ATA_DEVICE_TYPE_ATA) ? "ATA" : 
                (ata_devices[i].type == ATA_DEVICE_TYPE_ATAPI) ? "ATAPI" : "Unknown");
            terminal_printf("  Size: %d MB (%d sectors)\n", 
                ata_devices[i].size / 2048, ata_devices[i].size);
            terminal_printf("  LBA Mode: %s\n",
                (ata_devices[i].lba_mode == 0) ? "CHS" :
                (ata_devices[i].lba_mode == 1) ? "LBA28" : "LBA48");
            terminal_printf("  Location: %s channel, %s drive\n",
                (ata_devices[i].channel == 0) ? "Primary" : "Secondary",
                (ata_devices[i].drive == 0) ? "Master" : "Slave");
            terminal_writestring("\n");
        }
    }
    
    if (!found) {
        terminal_writestring("No ATA drives detected\n");
    }
}

// Get string description of ATA error code
const char* hal_ata_get_error_string(uint8_t error) {
    if (error & ATA_ERROR_BBK) return "Bad Block";
    if (error & ATA_ERROR_UNC) return "Uncorrectable Data";
    if (error & ATA_ERROR_MC) return "Media Changed";
    if (error & ATA_ERROR_IDNF) return "ID Not Found";
    if (error & ATA_ERROR_MCR) return "Media Change Request";
    if (error & ATA_ERROR_ABRT) return "Command Aborted";
    if (error & ATA_ERROR_TK0NF) return "Track 0 Not Found";
    if (error & ATA_ERROR_AMNF) return "Address Mark Not Found";
    return "Unknown Error";
}