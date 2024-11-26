#ifndef IDE_H_
#define IDE_H_

#include "mackerel.h"

// IDE mem-mapped registers
#define IDE_DATA IDE_BASE + 0x00
// #define IDE_ERROR IDE_BASE + 0x02
#define IDE_FEATURE IDE_BASE + 0x02
#define IDE_SECTOR_COUNT IDE_BASE + 0x04
#define IDE_SECTOR_START IDE_BASE + 0x06
#define IDE_LBA_MID IDE_BASE + 0x08
#define IDE_LBA_HIGH IDE_BASE + 0x0A
#define IDE_DRIVE_SEL IDE_BASE + 0x0C
#define IDE_STATUS IDE_BASE + 0x0E
#define IDE_COMMAND IDE_BASE + 0x0E

// IDE commands
#define IDE_CMD_RESET 0x08
#define IDE_CMD_READ_SECTOR 0x20
#define IDE_CMD_WRITE_SECTOR 0x30
#define IDE_CMD_IDENTIFY 0xEC
#define IDE_CMD_SET_FEATURES 0xEF

// Status register bits
#define IDE_SR_BSY 0x80  // Busy
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_DF 0x20   // Drive write fault
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DRQ 0x08  // Data request ready
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_ERR 0x01  // Error

void IDE_wait_for_device_ready();

void IDE_wait_for_data_ready();

void IDE_read_sector(uint16_t *buf, uint32_t lba);

void IDE_device_info(uint16_t *buf);

#endif