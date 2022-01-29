/**
 * @file ch376s.h
 * @author Colin Maykish (crmaykish@gmail.com)
 * @brief Control API for the CH376S USB module in UART mode
 * @date 2022-01-26
 * 
 * @copyright Copyright (c) 2022
 */

#ifndef CH376S_H
#define CH376S_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Command return values
#define CH376S_CMD_RET_SUCCESS 0x51
#define CH376S_CMD_RET_ABORT 0x5F

// Interrupt return values
#define CH376S_USB_INT_SUCCESS 0x14
#define CH376S_USB_INT_CONNECT 0x15
#define CH376S_USB_INT_DISCONNECT 0x16
#define CH376S_USB_INT_DISK_READ 0x1D

#define CH376S_ERR_OPEN_DIR 0x41
#define CH376S_ERR_MISS_FILE 0x42
#define CH376S_ERR_FILE_CLOSE 0xB4

// Status register bits
#define CH376S_PARA_STATE_INTB 0b10000000
#define CH376S_PARA_STATE_BZ 0b00010000

// USB command bytes
#define CH376S_CMD_SET_BAUDRATE 0x02
#define CH376S_CMD_RESET_ALL 0x05
#define CH376S_CMD_CHECK_EXIST 0x06
#define CH376S_CMD_SET_MODE 0x15
#define CH376S_CMD_GET_STATUS 0x22
#define CH376S_CMD_READ_USB_DATA0 0x27
#define CH376S_CMD_SET_FILENAME 0x2F
#define CH376S_CMD_DISK_CONNECT 0x30
#define CH376S_CMD_DISK_MOUNT 0x31
#define CH376S_CMD_FILE_OPEN 0x32
#define CH376S_CMD_NEXT_FILE 0x33
#define CH376S_CMD_FILE_CLOSE 0x36
#define CH376S_CMD_DIR_INFO 0x37
#define CH376S_CMD_BYTE_READ 0x3A
#define CH376S_CMD_BYTE_RD_GO 0x3B

// Mode values
#define CH376S_USB_HOST_MODE 0x05

// void ch376s_init();
// void ch376s_send_command(uint8_t command);
// void ch376s_send_byte(uint8_t b);
// void ch376s_send_string(char *s);
// uint8_t ch376s_get_byte();

// size_t ch376s_file_read(char *filename, uint8_t *buffer, size_t max_length);

// void ch376s_file_list();

#endif
