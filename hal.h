#ifndef __HAL_H__
#define __HAL_H__

#include <stdbool.h>

bool hal_init(char*);
int kmc_read_sector(unsigned int, unsigned char*);
int kmc_read_multi_sector(unsigned int, unsigned short, unsigned char*);

#endif
