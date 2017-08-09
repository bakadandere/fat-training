#ifndef __FAT12_H__
#define __FAT12_H__

#define HIGHEST_12_BITS 0xfff0
#define LOWEST_12_BITS  0x0fff

int cal_next_cluster_fat12(int, int*, unsigned char*);

#endif
