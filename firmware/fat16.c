#include <stdio.h>
#include <string.h>
#include "fat16.h"

fat16_read_sector_f read_sector = NULL;

void bswap_boot_sector(fat16_boot_sector_t *b)
{
    if (b == NULL)
        return;

    b->bytes_per_sector = bswap16(b->bytes_per_sector);
    b->reserved_sectors = bswap16(b->reserved_sectors);
    b->root_entries = bswap16(b->root_entries);
    b->total_sectors_16 = bswap16(b->total_sectors_16);
    b->fat_size_16 = bswap16(b->fat_size_16);
    b->sectors_per_track = bswap16(b->sectors_per_track);
    b->heads = bswap16(b->heads);
    b->hidden_sectors = bswap32(b->hidden_sectors);
    b->total_sectors_32 = bswap32(b->total_sectors_32);
    b->volume_id = bswap32(b->volume_id);
    b->boot_sector_signature = bswap16(b->boot_sector_signature);
}

void bswap_dir_entry(fat16_dir_entry_t *d)
{
    if (d == NULL)
        return;

    d->creation_time = bswap16(d->creation_time);
    d->creation_date = bswap16(d->creation_date);
    d->last_access_date = bswap16(d->last_access_date);
    d->first_cluster_high = bswap16(d->first_cluster_high);
    d->modification_time = bswap16(d->modification_time);
    d->modification_date = bswap16(d->modification_date);
    d->first_cluster_low = bswap16(d->first_cluster_low);
    d->file_size = bswap32(d->file_size);
}

int fat16_init(fat16_read_sector_f read_sector_fun)
{
    if (read_sector_fun)
    {
        read_sector = read_sector_fun;
        return 0;
    }

    return -1;
}

void fat16_read_boot_sector(uint32_t starting_sector, fat16_boot_sector_t *buffer)
{
    read_sector(starting_sector, (uint8_t *)(buffer));
    bswap_boot_sector(buffer);
}

void fat16_print_boot_sector_info(fat16_boot_sector_t *buffer)
{
    printf("\r\nBoot sector info:\r\n");
    printf("Bytes per sector: %u\n", buffer->bytes_per_sector);
    printf("Sectors per cluster: %u\n", buffer->sectors_per_cluster);
    printf("Reserved sectors: %u\n", buffer->reserved_sectors);
    printf("FAT count: %u\n", buffer->num_fats);
    printf("Root directory entries: %u\n", buffer->root_entries);
    printf("Total sectors: %u\n", buffer->total_sectors_16);
    printf("FAT size (sectors): %u\n", buffer->fat_size_16);
    printf("Sectors per track: %u\n", buffer->sectors_per_track);
    printf("Head count: %u\n", buffer->heads);
    printf("Hidden sectors: %lu\n", buffer->hidden_sectors);
    printf("Total sectors (large): %lu\n", buffer->total_sectors_32);
}

int fat16_list_files(fat16_boot_sector_t *boot_sector, fat16_dir_entry_t files_list[])
{
    if (read_sector == NULL)
    {
        printf("read_sector function pointer is NULL. Call fat16_init() first\r\n");
        return -1;
    }

    if (boot_sector == NULL)
    {
        printf("boot_sector is NULL\r\n");
        return -1;
    }

    int valid_files = 0;

    uint32_t root_dir_sector = 2048 + boot_sector->reserved_sectors + (boot_sector->num_fats * boot_sector->fat_size_16);
    uint8_t buffer[512];

    read_sector(root_dir_sector, buffer);

    // TODO: this 16 value should be determined by the root directory count in the boot sector
    // After 16 entries, a new sector needs to be read (I think)
    for (int i = 0; i < 16; i++)
    {
        fat16_dir_entry_t *entry = (fat16_dir_entry_t *)(buffer + i * 32);

        bswap_dir_entry(entry);

        // Check if the entry is free (0x00) or deleted (0xE5)
        if (entry->filename[0] == 0x00 || entry->filename[0] == 0xE5)
        {
            continue; // Skip empty or deleted entries
        }

        // Check if the entry represents a valid file or directory
        if (entry->attributes & 0x10)
        {
            // This is a directory (attribute 0x10)
            continue;
        }

        memcpy(&files_list[valid_files], entry, 32);
        valid_files++;
    }

    return valid_files;
}

void fat16_get_file_name(fat16_dir_entry_t *dir_entry, char *filename)
{
    if (dir_entry == NULL)
    {
        return;
    }

    if (filename == NULL)
    {
        return;
    }

    memset(filename, ' ', 13);

    // Copy the name
    strncpy(filename, (char *)dir_entry->filename, 8);
    // Add a dot between the name and extension
    filename[8] = '.';
    // Copy the extension
    strncpy(filename + 9, (char *)dir_entry->extension, 3);

    filename[12] = 0; // Null-terminate the string
}

// Function to get the next cluster from the FAT
uint16_t get_fat_entry(fat16_boot_sector_t *boot_sector, uint16_t cluster)
{
    static uint32_t cached_sector = (uint32_t)-1;
    static uint8_t fat_buffer[1024]; // Make sure this is large enough for your largest sector size

    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = boot_sector->hidden_sectors + boot_sector->reserved_sectors + (fat_offset / boot_sector->bytes_per_sector);
    uint32_t fat_entry_offset = fat_offset % boot_sector->bytes_per_sector;

    // Only read if not already cached
    if (fat_sector != cached_sector) {
        read_sector(fat_sector, fat_buffer);
        cached_sector = fat_sector;
    }

    uint16_t next_cluster = (fat_buffer[fat_entry_offset] | (fat_buffer[fat_entry_offset + 1] << 8)) & 0xFFFF;
    return next_cluster;
}

int fat16_read_file(fat16_boot_sector_t *boot_sector, uint16_t starting_cluster, uint8_t *buffer, uint32_t buffer_size)
{
    uint32_t buffer_index = 0;
    uint16_t current_cluster = starting_cluster;

    // Calculate the first sector of the cluster
    uint32_t sector_size = boot_sector->bytes_per_sector;
    uint32_t sectors_per_cluster = boot_sector->sectors_per_cluster;

    while (current_cluster != 0xFFFF && buffer_index < buffer_size)
    {
        // printf("current cluster: %u\r\n", current_cluster);

        // TODO: where is this magic 32 coming from?
        uint32_t first_sector_of_cluster = 32 + 2048 + (current_cluster - 2) * sectors_per_cluster + boot_sector->reserved_sectors + (boot_sector->num_fats * boot_sector->fat_size_16);

        // Read the sectors of the current cluster into the buffer
        for (uint32_t sector_offset = 0; sector_offset < sectors_per_cluster && buffer_index < buffer_size; sector_offset++)
        {
            uint32_t sector = first_sector_of_cluster + sector_offset;
            uint8_t sector_buffer[sector_size];

            // Read the sector into the buffer
            // printf("file sector %u\r\n", sector);
            read_sector(sector, sector_buffer);

            // Copy the sector data into the main buffer
            uint32_t bytes_to_copy = (buffer_size - buffer_index < sector_size) ? (buffer_size - buffer_index) : sector_size;
            memcpy(buffer + buffer_index, sector_buffer, bytes_to_copy);
            buffer_index += bytes_to_copy;
        }

        // Progress reporting
        duart_putc('.');

        // Get the next cluster from the FAT
        current_cluster = get_fat_entry(boot_sector, current_cluster);
    }

    printf("\r\n");

    return buffer_index; // Returns the number of bytes read
}
