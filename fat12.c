#include <malloc.h>
#include "fat12.h"
#include "fat.h"

extern bytes_per_sector, sectors_per_cluster;

int cal_next_cluster_fat12(int current_cluster, int *last_fat_sector, unsigned char *one_sector_buffer)
{
    int cluster_offset = (current_cluster / 2) + current_cluster, value,
        current_fat_sector = current_cluster / bytes_per_sector + 1;
    if (current_fat_sector != *last_fat_sector) {
        read_fat_sector(cluster_offset, one_sector_buffer);
        *last_fat_sector = current_cluster;
    }
    value = one_sector_buffer[cluster_offset % bytes_per_sector];
    if (!((cluster_offset + 1) % bytes_per_sector)) {
        read_fat_sector_subsequently(cluster_offset, one_sector_buffer);
        value |= one_sector_buffer[0] << 8;
    } else value |= one_sector_buffer[cluster_offset % bytes_per_sector + 1] << 8;
    if ((current_cluster % 2)) {
        value = (value & HIGHEST_12_BITS) >> 4;
    } else {
        value &= LOWEST_12_BITS;
    }
    
    return value;
}
