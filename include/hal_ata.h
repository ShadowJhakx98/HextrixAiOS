// include/hal_ata.h
#ifndef HAL_ATA_H
#define HAL_ATA_H

#include <stdint.h>

// ATA/IDE controller ports
#define ATA_PRIMARY_DATA        0x1F0
#define ATA_PRIMARY_ERROR       0x1F1
#define ATA_PRIMARY_SECTOR_CNT  0x1F2
#define ATA_PRIMARY_LBA_LOW     0x1F3
#define ATA_PRIMARY_LBA_MID     0x1F4
#define ATA_PRIMARY_LBA_HIGH    0x1F5
#define ATA_PRIMARY_DRIVE_HEAD  0x1F6
#define ATA_PRIMARY_STATUS      0x1F7
#define ATA_PRIMARY_COMMAND     0x1F7
#define ATA_PRIMARY_ALT_STATUS  0x3F6
#define ATA_PRIMARY_CONTROL     0x3F6

// Secondary controller ports (for second IDE channel)
#define ATA_SECONDARY_DATA        0x170
#define ATA_SECONDARY_ERROR       0x171
#define ATA_SECONDARY_SECTOR_CNT  0x172
#define ATA_SECONDARY_LBA_LOW     0x173
#define ATA_SECONDARY_LBA_MID     0x174
#define ATA_SECONDARY_LBA_HIGH    0x175
#define ATA_SECONDARY_DRIVE_HEAD  0x176
#define ATA_SECONDARY_STATUS      0x177
#define ATA_SECONDARY_COMMAND     0x177
#define ATA_SECONDARY_ALT_STATUS  0x376
#define ATA_SECONDARY_CONTROL     0x376

// ATA command codes
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_IDENTIFY          0xEC
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA

// ATA status register bits
#define ATA_STATUS_ERR            0x01
#define ATA_STATUS_DRQ            0x08
#define ATA_STATUS_SRV            0x10
#define ATA_STATUS_DF             0x20
#define ATA_STATUS_RDY            0x40
#define ATA_STATUS_BSY            0x80

// ATA error register bits
#define ATA_ERROR_BBK             0x80
#define ATA_ERROR_UNC             0x40
#define ATA_ERROR_MC              0x20
#define ATA_ERROR_IDNF            0x10
#define ATA_ERROR_MCR             0x08
#define ATA_ERROR_ABRT            0x04
#define ATA_ERROR_TK0NF           0x02
#define ATA_ERROR_AMNF            0x01

// ATA device types
#define ATA_DEVICE_TYPE_UNKNOWN   0
#define ATA_DEVICE_TYPE_ATA       1
#define ATA_DEVICE_TYPE_ATAPI     2

// ATA device structure
typedef struct {
    uint8_t present;               // Is device present
    uint8_t type;                  // Device type
    uint8_t channel;               // Primary (0) or secondary (1) channel
    uint8_t drive;                 // Master (0) or slave (1) drive
    uint16_t signature;            // Device signature
    uint16_t capabilities;         // Device capabilities
    uint32_t command_sets;         // Supported command sets
    uint32_t size;                 // Size in sectors
    char model[41];                // Model string from identify command
    char serial[21];               // Serial number from identify command
    uint8_t lba_mode;              // 0: CHS, 1: LBA28, 2: LBA48
} ata_device_t;

// Function declarations
void hal_ata_init(void);
int hal_ata_detect_devices(void);
int hal_ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, void* buffer);
int hal_ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, const void* buffer);
int hal_ata_identify(uint8_t drive, void* buffer);
void hal_ata_print_info(void);
const char* hal_ata_get_error_string(uint8_t error);

// Get ATA device information
ata_device_t* hal_ata_get_device(uint8_t drive);

#endif // HAL_ATA_H