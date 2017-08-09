#ifndef __FAT_H__
#define __FAT_H__

#define BYTES_PER_DIR_ENTRY     32
#define MAX_BYTES_PER_SECTOR    4096
#define CLUSTER_STARTING_INDEX  2
#define LAST_CLUSTER_SIG        0xFFF
#define TMP_FILE_NAME           "tmp_file."

#include <stdbool.h>

typedef struct {
    char *name;
    int first_cluster, size;
} Entry;

bool fat_init(char*);

void read_boot_sector();
void read_entry(Entry*, unsigned char*, int);
char *read_name(unsigned char*, int);
void read_file(Entry, int);

void print_boot_sector();
void print_list(Entry*, int, Entry*, int, int);

int data_to_physical_cluster(int);
int read_fat_sector_subsequently(unsigned int, unsigned char*);
int read_fat_sector(unsigned int, unsigned char*);
int get_bytes_per_sector();
int get_reserved_sectors_num();
int get_fats_num();
int get_fat_size();
int get_sectors_per_cluster();
int get_max_root_entry_num();

#endif
