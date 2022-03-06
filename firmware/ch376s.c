#include "ch376s.h"
#include "mackerel.h"

static bool interrupt_ready()
{
    return ((MEM(USB_COMMAND) & 0b10000000) == 0);
}

static uint8_t wait_for_interrupt()
{
    while (!interrupt_ready())
    {
    }

    MEM(USB_COMMAND) = CH376S_CMD_GET_STATUS;
    return MEM(USB_DATA);
}

static void send_string(char *s)
{
    int i = 0;
    while (s[i] != 0)
    {
        MEM(USB_DATA) = s[i];
        i++;
    }

    MEM(USB_DATA) = 0;
}

uint8_t usb_reset()
{
    MEM(USB_COMMAND) = CH376S_CMD_RESET_ALL;
    delay(20000);
    MEM(USB_COMMAND) = CH376S_CMD_SET_MODE;
    MEM(USB_DATA) = CH376S_USB_HOST_MODE;
    delay(10000);

    return MEM(USB_DATA);
}

size_t file_read(char *file_name, uint8_t *buffer)
{
    size_t total_bytes_read = 0;
    uint8_t response = 0;

    MEM(USB_COMMAND) = CH376S_CMD_SET_FILENAME;
    send_string(file_name);

    response = MEM(USB_DATA);

    MEM(USB_COMMAND) = CH376S_CMD_FILE_OPEN;
    response = wait_for_interrupt();

    if (response == CH376S_USB_INT_SUCCESS)
    {
        bool file_done = false;
        bool page_done = false;

        while (!file_done)
        {
            page_done = false;

            MEM(USB_COMMAND) = CH376S_CMD_BYTE_READ;
            MEM(USB_DATA) = 0x00;
            MEM(USB_DATA) = 0x10;

            response = wait_for_interrupt();

            if (response == CH376S_USB_INT_SUCCESS)
            {
                page_done = true;
                file_done = true;
            }

            while (!page_done)
            {
                if (response == CH376S_USB_INT_DISK_READ)
                {
                    MEM(USB_COMMAND) = CH376S_CMD_READ_USB_DATA0;
                    uint8_t bytes_available = MEM(USB_DATA);

                    for (int i = 0; i < bytes_available; i++)
                    {
                        buffer[total_bytes_read] = MEM(USB_DATA);
                        total_bytes_read++;
                    }

                    MEM(USB_COMMAND) = CH376S_CMD_BYTE_RD_GO;
                    response = wait_for_interrupt();
                }
                else if (response == CH376S_USB_INT_SUCCESS)
                {
                    page_done = true;
                }
                else
                {
                    // printf("Failed to read file data: %02X\r\n", response);
                    page_done = true;
                    file_done = true;
                }
            }

            mfp_putc('.');
        }
    }

    // Zero-terminate the buffer (in case it's used as a string)
    buffer[total_bytes_read] = 0;

    MEM(USB_COMMAND) = CH376S_CMD_FILE_CLOSE;
    MEM(USB_DATA) = 0;

    wait_for_interrupt();

    return total_bytes_read;
}
