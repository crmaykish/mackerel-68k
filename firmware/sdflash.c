#include <stdio.h>
#include <string.h>
#include "sdflash.h"
#include "sd_spi.h"
#include "fat16.h"
#include "term.h"

#define WRITE_CHUNK 64 // sectors per progress-bar step
#define VERIFY_CHUNK (SDFLASH_SCRATCH / 512)
#define FAT_DATE 0x5C21 // 2026-01-01 (no RTC)

static fat16_boot_sector_t bs;
static uint32_t fat_start, root_start, root_secs, data_start, clusters, cluster_bytes;

// On-disk FAT fields are little-endian; store bytes explicitly so this is endian-independent
static void put16(uint8_t *p, uint16_t v)
{
    p[0] = v;
    p[1] = v >> 8;
}

static void put32(uint8_t *p, uint32_t v)
{
    put16(p, v);
    put16(p + 2, v >> 16);
}

bool sdflash_prepare(void)
{
    uint32_t p = fat16_part_start();
    fat16_read_boot_sector(p, &bs);

    uint8_t spc = bs.sectors_per_cluster;
    uint32_t total = bs.total_sectors_16 ? bs.total_sectors_16 : bs.total_sectors_32;
    if (bs.boot_sector_signature != 0xAA55 || memcmp(bs.fs_type, "FAT16", 5) != 0 ||
        bs.bytes_per_sector != 512 || spc == 0 || (spc & (spc - 1)) != 0 ||
        bs.num_fats < 1 || bs.num_fats > 2 || bs.fat_size_16 == 0 || bs.root_entries == 0)
    {
        printf("ERROR: no usable FAT16 boot sector at LBA %lu (prepare the card on the host)\r\n", (unsigned long)p);
        return false;
    }

    fat_start = p + bs.reserved_sectors;
    root_start = fat_start + (uint32_t)bs.num_fats * bs.fat_size_16;
    root_secs = ((uint32_t)bs.root_entries * 32 + 511) / 512;
    data_start = root_start + root_secs;
    if (total <= data_start - p)
    {
        printf("ERROR: FAT16 boot sector has an impossible layout\r\n");
        return false;
    }
    clusters = (total - (data_start - p)) / spc;
    uint32_t fat_entries = (uint32_t)bs.fat_size_16 * 256;
    if (clusters > fat_entries - 2)
        clusters = fat_entries - 2;
    if (clusters > 0xFFF4) // cluster numbers 2..0xFFF6
        clusters = 0xFFF4;
    cluster_bytes = (uint32_t)spc * 512;

    printf("FAT16 at LBA %lu: %lu x %lu-byte clusters (max image %lu bytes)\r\n",
           (unsigned long)p, (unsigned long)clusters, (unsigned long)cluster_bytes,
           (unsigned long)(clusters * cluster_bytes));
    return true;
}

// Root directory sector 0: the volume label entry (if any), then optionally IMAGE.BIN
static void build_root(uint8_t *sec, const uint8_t *label, bool with_image, uint32_t len)
{
    memset(sec, 0, 512);
    uint8_t *e = sec;
    if (label[0] != 0)
    {
        memcpy(e, label, 32);
        e += 32;
    }
    if (!with_image)
        return;
    memcpy(e, "IMAGE   BIN", 11);
    e[11] = 0x20; // archive
    put16(e + 16, FAT_DATE); // created
    put16(e + 18, FAT_DATE); // accessed
    put16(e + 24, FAT_DATE); // modified
    put16(e + 26, 2); // first cluster
    put32(e + 28, len);
}

static bool write_sectors(uint32_t lba, const uint8_t *buf, uint32_t count)
{
    if (sd_spi_write(lba, buf, count) == 0)
        return true;
    printf("\r\nERROR: SD write failed at LBA %lu (the card has no IMAGE.BIN now; re-run sdflash)\r\n", (unsigned long)lba);
    return false;
}

// Write the data sectors, the last one zero-padded via sec
static bool write_data(const uint8_t *img, uint32_t len, uint8_t *sec)
{
    uint32_t full = len / 512;
    uint32_t done = 0;
    while (done < full)
    {
        uint32_t n = full - done < WRITE_CHUNK ? full - done : WRITE_CHUNK;
        if (!write_sectors(data_start + done, img + done * 512, n))
            return false;
        done += n;
        term_progress_bar(done * 100 / (full + 1));
    }
    uint32_t rem = len % 512;
    if (rem)
    {
        memset(sec, 0, 512);
        memcpy(sec, img + full * 512, rem);
        if (!write_sectors(data_start + full, sec, 1))
            return false;
    }
    term_progress_bar(100);
    printf("\r\n");
    return true;
}

// Every FAT copy: media + end-of-chain markers, then the chain 2 -> 3 -> ... -> EOC, rest free
static bool write_fats(uint32_t need, uint8_t *sec)
{
    uint32_t last = need + 1;
    for (uint32_t s = 0; s < bs.fat_size_16; s++)
    {
        for (uint32_t i = 0; i < 256; i++)
        {
            uint32_t c = s * 256 + i;
            uint16_t v = 0;
            if (c == 0)
                v = 0xFF00 | bs.media_descriptor;
            else if (c == 1 || c == last)
                v = 0xFFFF;
            else if (c < last)
                v = c + 1;
            put16(sec + i * 2, v);
        }
        for (uint32_t f = 0; f < bs.num_fats; f++)
            if (!write_sectors(fat_start + f * bs.fat_size_16 + s, sec, 1))
                return false;
    }
    return true;
}

static bool verify(const uint8_t *img, uint32_t len, uint8_t *scratch)
{
    fat16_dir_entry_t files[16];
    memset(files, 0, sizeof(files));
    fat16_list_files(&bs, files);
    bool found = false;
    for (int i = 0; i < 16 && !found; i++)
    {
        char fn[13];
        fat16_get_file_name(&files[i], fn);
        found = strncmp(fn, "IMAGE   .BIN", 12) == 0 && files[i].file_size == len && files[i].first_cluster_low == 2;
    }
    if (!found)
    {
        printf("ERROR: verify: IMAGE.BIN directory entry missing or wrong\r\n");
        return false;
    }

    uint32_t secs = (len + 511) / 512;
    for (uint32_t done = 0; done < secs;)
    {
        uint32_t n = secs - done < VERIFY_CHUNK ? secs - done : VERIFY_CHUNK;
        if (sd_spi_read(data_start + done, scratch, n) != 0)
        {
            printf("\r\nERROR: verify: SD read failed at LBA %lu\r\n", (unsigned long)(data_start + done));
            return false;
        }
        uint32_t off = done * 512;
        uint32_t bytes = len - off < n * 512 ? len - off : n * 512;
        if (memcmp(scratch, img + off, bytes) != 0)
        {
            for (uint32_t b = 0; b < bytes; b++)
                if (scratch[b] != img[off + b])
                {
                    printf("\r\nERROR: verify: mismatch at byte %lu (LBA %lu)\r\n",
                           (unsigned long)(off + b), (unsigned long)(data_start + done + b / 512));
                    break;
                }
            return false;
        }
        done += n;
        term_progress_bar(done * 100 / secs);
    }
    printf("\r\n");
    return true;
}

bool sdflash_write(const uint8_t *img, uint32_t len, uint8_t *scratch)
{
    uint32_t need = (len + cluster_bytes - 1) / cluster_bytes;
    if (len == 0 || need > clusters)
    {
        printf("ERROR: image is %lu bytes, the partition holds at most %lu\r\n",
               (unsigned long)len, (unsigned long)(clusters * cluster_bytes));
        return false;
    }

    // Keep the volume label entry, if the first root sector has one
    uint8_t label[32];
    memset(label, 0, sizeof(label));
    if (sd_spi_read(root_start, scratch, 1) != 0)
    {
        printf("ERROR: SD read failed at LBA %lu\r\n", (unsigned long)root_start);
        return false;
    }
    for (int i = 0; i < 16; i++)
    {
        uint8_t *e = scratch + i * 32;
        if (e[0] != 0x00 && e[0] != 0xE5 && (e[11] & 0x3F) == 0x08)
        {
            memcpy(label, e, 32);
            break;
        }
    }

    // Drop the old directory first, so a failure part-way leaves no IMAGE.BIN to autoboot
    printf("Clearing the root directory...\r\n");
    build_root(scratch, label, false, 0);
    if (!write_sectors(root_start, scratch, 1))
        return false;
    memset(scratch, 0, 512);
    for (uint32_t i = 1; i < root_secs; i++)
        if (!write_sectors(root_start + i, scratch, 1))
            return false;

    bool ok;
    term_cursor_set_vis(false);
    printf("Writing %lu bytes (%lu clusters)...\r\n", (unsigned long)len, (unsigned long)need);
    ok = write_data(img, len, scratch);
    if (ok)
    {
        printf("Writing the FAT...\r\n");
        ok = write_fats(need, scratch);
    }
    if (ok)
    {
        build_root(scratch, label, true, len);
        ok = write_sectors(root_start, scratch, 1);
    }
    if (ok)
    {
        printf("Verifying...\r\n");
        ok = verify(img, len, scratch);
    }
    term_cursor_set_vis(true);

    if (ok)
        printf("IMAGE.BIN written and verified (%lu bytes)\r\n", (unsigned long)len);
    return ok;
}
