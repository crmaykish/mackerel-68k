// Mackerel-F SD file read + speed test (RAM program, ymodem + run).
//
// Dumps the MBR partition table, mounts FAT16, lists files, reads IMAGE.BIN into
// SDRAM (validate), then times a raw 1 MB sequential read with the 100 Hz timer
// to report throughput (kept separate from the file read so its progress bar /
// console output doesn't skew the number).

#include <stdio.h>
#include <string.h>
#include "mackerel.h"
#include "sd_spi.h"
#include "fat16.h"

#define TIMER_CTRL   (TIMER_BASE + 0) // [0]=ENABLE, [5:4]=FREQ (3=100 Hz)
#define TIMER_STATUS (TIMER_BASE + 2) // read [0]=PENDING ; write = ACK
#define TICK_VECTOR  (EXCEPTION_AUTOVECTOR + IRQ_NUM_TIMER) // level 6 -> vector 30

#define IMG_LOAD 0x100000 // load buffer (1 MB up; clear of this program + heap)
#define IMG_MAX  0x600000 // cap the file read at 6 MB (ends well below the 0x7C0000 reserve)
#define SPEED_BLOCKS 2048 // 1 MB raw read for the throughput test

volatile uint32_t g_ticks = 0; // 10 ms each (100 Hz timer ISR)

void __attribute__((interrupt)) timer_isr(void)
{
    MEM(TIMER_STATUS) = 0; // ack the level-6 IRQ before RTE
    g_ticks++;
}

static uint32_t rd32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void dump_mbr(const uint8_t *mbr)
{
    printf("MBR partitions:\r\n");
    for (int i = 0; i < 4; i++)
    {
        const uint8_t *e = mbr + 0x1BE + i * 16;
        uint8_t type = e[4];
        if (type == 0)
            continue;
        uint32_t lba = rd32le(e + 8);
        uint32_t cnt = rd32le(e + 12);
        printf("  [%d] type=0x%02X start=%lu count=%lu (~%lu MB)\r\n",
               i, type, (unsigned long)lba, (unsigned long)cnt, (unsigned long)(cnt / 2048));
    }
}

int main(void)
{
    static uint8_t mbr[512];
    fat16_boot_sector_t bs;
    fat16_dir_entry_t files[16] = {0};

    printf("\r\n=== %s SD file read + speed test (%s %s) ===\r\n", SYSTEM_NAME, __DATE__, __TIME__);

    if (!sd_spi_init())
    {
        printf("SD init FAILED\r\n");
        return 1;
    }
    sd_spi_print_info();

    // --- MBR partition table ---
    if (sd_spi_read(0, mbr, 1) != 0)
    {
        printf("MBR read FAILED\r\n");
        return 1;
    }
    if (mbr[510] != 0x55 || mbr[511] != 0xAA)
    {
        printf("no MBR signature (%02X%02X)\r\n", mbr[510], mbr[511]);
        return 1;
    }
    dump_mbr(mbr);

    // --- mount FAT16 ---
    fat16_init(sd_spi_read);
    if (fat16_find_partition() != 0)
    {
        printf("no FAT16 partition (FAT32/exFAT would need a different parser)\r\n");
        return 1;
    }
    printf("FAT16 partition at LBA %lu\r\n", (unsigned long)fat16_part_start());

    fat16_read_boot_sector(fat16_part_start(), &bs);
    fat16_print_boot_sector_info(&bs);

    // --- list files, find IMAGE.BIN ---
    printf("\r\nFiles:\r\n");
    int n = fat16_list_files(&bs, files);
    int img = -1;
    for (int i = 0; i < n; i++)
    {
        char name[13];
        fat16_get_file_name(&files[i], name);
        printf("  %s  %lu bytes\r\n", name, (unsigned long)files[i].file_size);
        if (strncmp(name, "IMAGE   .BIN", 12) == 0)
            img = i;
    }
    if (img < 0)
    {
        printf("IMAGE.BIN not found\r\n");
        return 1;
    }

    // --- validate the file read ---
    uint32_t size = files[img].file_size;
    uint32_t rd = size > IMG_MAX ? IMG_MAX : size;
    printf("\r\nReading IMAGE.BIN (%lu bytes%s) to 0x%X...\r\n",
           (unsigned long)size, size > IMG_MAX ? ", capped" : "", IMG_LOAD);
    int got = fat16_read_file(&bs, files[img].first_cluster_low, (uint8_t *)IMG_LOAD, rd);
    printf("read %d of %lu bytes  ", got, (unsigned long)rd);
    printf("first 16: ");
    for (int i = 0; i < 16; i++)
        printf("%02X ", MEM(IMG_LOAD + i));
    printf("\r\n%s\r\n", got == (int)rd ? "FILE READ OK" : "SHORT READ");

    // --- speed test: timed raw 1 MB sequential read (no console in the loop) ---
    set_interrupts(false);
    set_exception_handler(TICK_VECTOR, timer_isr);
    g_ticks = 0;
    MEM(TIMER_CTRL) = 0x01 | (3 << 4); // enable, 100 Hz
    set_interrupts(true);

    int sok = (sd_spi_read(fat16_part_start(), (uint8_t *)IMG_LOAD, SPEED_BLOCKS) == 0);

    set_interrupts(false);
    MEM(TIMER_CTRL) = 0x00;

    uint32_t ms = g_ticks * 10;
    uint32_t kb = (SPEED_BLOCKS * 512U) / 1024U;
    printf("\r\nspeed: %lu KB raw read in %lu ms = %lu KB/s %s\r\n",
           (unsigned long)kb, (unsigned long)ms,
           (unsigned long)(ms ? kb * 1000U / ms : 0), sok ? "" : "(read err!)");

    return (got == (int)rd && sok) ? 0 : 1;
}
