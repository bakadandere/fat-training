#include <stdio.h>
#include <malloc.h>
#include "hal.h"

FILE *g_media_ptr, *g_tmp_file_ptr;
char g_tmp_file_name[12];
int bytes_per_sector;

bool hal_init(char *disk_name)
{
    unsigned char word[2];
    g_media_ptr = fopen(disk_name, "rb");
    if (g_media_ptr == NULL) {
        printf("Error while opening the media.\n");
        return false;
    }
    fseek(g_media_ptr, 11, SEEK_SET);
    fread(word, sizeof(unsigned char), 2, g_media_ptr);
    bytes_per_sector = word[0] | word[1];
    return true;
}

int kmc_read_sector(unsigned int index, unsigned char *buff)
{
    // Reads the (index)th sector into the (buff) string, returns the number of read bytes.
    fseek(g_media_ptr, index * bytes_per_sector, SEEK_SET);
    return fread(buff, sizeof(unsigned char), bytes_per_sector, g_media_ptr);
}

int kmc_read_multi_sector(unsigned int index, unsigned short num, unsigned char *buff)
{
    // Reads (num) sectors sequentially starting with the (index)th sector into the (buff) string
    // and returns the number of read bytes.
    fseek(g_media_ptr, index * bytes_per_sector, SEEK_SET);
    return fread(buff, sizeof(unsigned char), bytes_per_sector * num, g_media_ptr);
}
