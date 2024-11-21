#ifndef FAT16_H_
#define FAT16_H_

#include "mackerel.h"

// Function to read a single sector from a block device
typedef void (*fat16_read_sector_f)(uint32_t, uint8_t *);

typedef struct __attribute__((packed))
{
    uint8_t jump_instruction[3]; // Jump instruction to boot code (0xEB + 2 bytes)
    uint8_t oem_name[8];         // OEM Name (e.g., "MSWIN4.1")
    uint16_t bytes_per_sector;   // Bytes per sector (usually 512)
    uint8_t sectors_per_cluster; // Sectors per cluster (1 byte, usually 1, 2, 4, or 8)
    uint16_t reserved_sectors;   // Reserved sectors before the first FAT
    uint8_t num_fats;            // Number of FATs (typically 2)
    uint16_t root_entries;       // Maximum number of entries in the root directory
    uint16_t total_sectors_16;   // Total number of sectors (16-bit field)
    uint8_t media_descriptor;    // Media descriptor type (0xF8 for hard drives)
    uint16_t fat_size_16;        // Size of one FAT in sectors (16-bit)
    uint16_t sectors_per_track;  // Sectors per track (for compatibility with BIOS)
    uint16_t heads;              // Number of heads (for compatibility with BIOS)
    uint32_t hidden_sectors;     // Number of hidden sectors (usually 0)
    uint32_t total_sectors_32;   // Total number of sectors (32-bit field, used for larger drives)

    // FAT16 specific BPB (BIOS Parameter Block)
    uint8_t drive_number;     // Drive number (usually 0x80 for hard drives)
    uint8_t reserved_1;       // Reserved (0x00)
    uint8_t boot_signature;   // Boot signature (0x29)
    uint32_t volume_id;       // Volume ID (unique to each volume)
    uint8_t volume_label[11]; // Volume label (e.g., "NO NAME    ")
    uint8_t fs_type[8];       // File system type (e.g., "FAT16   ")

    uint8_t boot_code[448];         // Boot code (can be used by the bootloader)
    uint16_t boot_sector_signature; // Boot sector signature (0x55AA)
} fat16_boot_sector_t;

typedef struct __attribute__((packed))
{
    uint8_t filename[8];         // 8 characters for file name
    uint8_t extension[3];        // 3 characters for file extension
    uint8_t attributes;          // File attributes (e.g., 0x20 for regular files)
    uint8_t reserved;            // Reserved (usually 0)
    uint8_t creation_time_tenth; // Creation time (tenth of second)
    uint16_t creation_time;      // Creation time
    uint16_t creation_date;      // Creation date
    uint16_t last_access_date;   // Last access date
    uint16_t first_cluster_high; // High word of first cluster
    uint16_t modification_time;  // Last modification time
    uint16_t modification_date;  // Last modification date
    uint16_t first_cluster_low;  // Low word of first cluster
    uint32_t file_size;          // File size (in bytes)
} fat16_dir_entry_t;

int fat16_init(fat16_read_sector_f read_sector);

void fat16_read_boot_sector(uint32_t starting_sector, fat16_boot_sector_t *buffer);

void fat16_print_boot_sector_info(fat16_boot_sector_t *buffer);

int fat16_list_files(fat16_boot_sector_t *boot_sector, fat16_dir_entry_t files_list[]);

void fat16_get_file_name(fat16_dir_entry_t *dir_entry, char *filename);

int fat16_read_file(fat16_boot_sector_t *boot_sector, uint16_t starting_cluster, uint8_t *buffer, uint32_t buffer_size);

#endif
