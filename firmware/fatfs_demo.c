#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mackerel.h"
#include "sd.h"
#include "fat16.h"

int main()
{
    fat16_boot_sector_t boot_sector;

    fat16_dir_entry_t files_list[16] = {0};

    printf("FAT Filesystem Demo\r\n");

    if (!sd_init())
    {
        return 1;
    }

    // Initialize FAT16 library with the SD card sector read function
    if (fat16_init(sd_read) != 0)
    {
        printf("Failed to initialize FAT16 library\r\n");
        return -1;
    }

    // TODO: read the MBR to find the partition location (for now, just assume partition starts at sector 2048)
    printf("Reading boot sector of first partition\r\n");
    fat16_read_boot_sector(2048, &boot_sector);
    fat16_print_boot_sector_info(&boot_sector);

    printf("Listing files in partition:\r\n");
    fat16_list_files(&boot_sector, files_list);

    for (int i = 0; i < 16; i++)
    {
        if (files_list[i].file_size > 0)
        {
            char filename[13];
            fat16_get_file_name(&files_list[i], filename);
            printf("%s, %u\r\n", filename, files_list[i].file_size);
        }
    }

    printf("Printing example.txt...\r\n");

    for (int i = 0; i < 16; i++)
    {
        if (files_list[i].file_size > 0)
        {
            char filename[13];
            fat16_get_file_name(&files_list[i], filename);

            if (strncmp(filename, "EXAMPLE .TXT", 12) == 0)
            {
                printf("Found file: %s - cluster: %u\r\n", filename, files_list[i].first_cluster_low);

                uint8_t *file = (uint8_t *)0x80000;

                int bytes_read = fat16_read_file(&boot_sector, files_list[i].first_cluster_low, file, files_list[i].file_size);

                if (bytes_read != files_list[i].file_size)
                {
                    printf("File read failed\r\n");
                }

                printf("\r\n");

                memdump((uint32_t)file, bytes_read);

                break;
            }
        }
    }

    return 0;
}
