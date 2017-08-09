#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include "fat.h"
#include "fat12.h"
#include "hal.h"

int bytes_per_sector, sectors_per_cluster, reserved_sectors_num,
    fats_num, fat_size, max_root_entry_num;

int get_sectors_per_cluster()
{
    return sectors_per_cluster;
}
int get_max_root_entry_num()
{
    return max_root_entry_num;
}

int get_fat_size()
{
    return fat_size;
}

int get_fats_num()
{
    return fats_num;
}

int get_reserved_sectors_num()
{
    return reserved_sectors_num;
}

int get_bytes_per_sector()
{
    return bytes_per_sector;
}

void read_boot_sector()
{
    unsigned char *one_sector_buffer = (unsigned char*) malloc(sizeof(unsigned char) * MAX_BYTES_PER_SECTOR);
    bytes_per_sector = MAX_BYTES_PER_SECTOR;
    kmc_read_sector(0, one_sector_buffer);
    bytes_per_sector = one_sector_buffer[11] | one_sector_buffer[12] << 8;
    free(one_sector_buffer);
    one_sector_buffer = (unsigned char*) malloc(bytes_per_sector);
    kmc_read_sector(0, one_sector_buffer);
    sectors_per_cluster = one_sector_buffer[13];
    reserved_sectors_num = one_sector_buffer[14] | one_sector_buffer[15] << 8;
    fats_num = one_sector_buffer[16];
    fat_size = one_sector_buffer[22] | one_sector_buffer[23] << 8;
    max_root_entry_num = one_sector_buffer[17] | one_sector_buffer[18] << 8;
}

void print_boot_sector()
{
    printf("Bytes/sector: %d\n", bytes_per_sector);
    printf("Sectors/cluster: %d\n", sectors_per_cluster);
    printf("Number of reversed sectors: %d\n", reserved_sectors_num);
    printf("Number of FATs: %d\n", fats_num);
    printf("Size of a FAT: %d\n", fat_size);
    printf("Number of maximum root entries: %d\n", max_root_entry_num);
}

char *read_name(unsigned char *buffer, int buff_offset)
{
    int i, name_length = 0;
    unsigned char *name;
    for (i = 0; i < 8; i++)
        if (buffer[buff_offset + i] == 0x20)
            break;
        else
            name_length++;
    if (buffer[buff_offset + 8] == 0x20) {
        name = (unsigned char*) malloc(name_length);
        for (i = 0; i < name_length; i++) {
            name[i] = buffer[buff_offset + i];
        }
        name[name_length] = 0;
    } else {
        name = (unsigned char*) malloc(name_length + 4);
        for (i = 0; i < name_length; i++) {
            name[i] = buffer[buff_offset + i];
        }
        name[name_length] = '.';
        name[name_length + 1] = buffer[buff_offset + 8];
        name[name_length + 2] = buffer[buff_offset + 9];
        name[name_length + 3] = buffer[buff_offset + 10];
        name[name_length + 4] = 0;
    }
    return name;
}


void read_entry(Entry *entry, unsigned char *buffer, int buff_offset)
{
    entry->name = read_name(buffer, buff_offset);
    if (buffer[buff_offset + 11] != 0x10 &&
        buffer[buff_offset + 11] != 0x0f) {
        entry->size = buffer[buff_offset + 28] |
            buffer[buff_offset + 29] << 8 |
            buffer[buff_offset + 30] << 16 |
            buffer[buff_offset + 31] << 24;
    } else if (buffer[buff_offset + 11] == 0x10) {
        entry->size = 0;
    }
    entry->first_cluster = buffer[buff_offset + 26] |
        buffer[buff_offset + 27] << 8;
}

void print_list(Entry *files, int files_num, Entry *dirs, int dirs_num, int current_dir_cluster)
{
    int i, total_entries = dirs_num + files_num;
    for (i = 0; i < dirs_num; i++)
        printf("%d. %-21s <DIR>\n",
            i + 1, dirs[i].name, dirs[i].first_cluster);
    for (i = 0; i < files_num; i++) {
        printf("%d. %-12s %8d bytes\n",
            i + dirs_num + 1,
            files[i].name,
            files[i].size,
            files[i].first_cluster);
    }
    do {
        printf("Enter index %d .. %d to open a dir or a file, 0 to quit >> ",
            1, total_entries);
        scanf("%d", &i);
    } while (i < 0 || i > total_entries);
    if (!i)
        return;
    if (i <= dirs_num) {
        i--;
        list_dir(dirs[i].first_cluster);
    } else {
        i = i - dirs_num - 1;
        read_file(files[i], current_dir_cluster);
    }
    free(dirs);
    free(files);
}

int data_to_physical_cluster(int data_cluster)
{
    return (data_cluster - CLUSTER_STARTING_INDEX) * sectors_per_cluster +
        max_root_entry_num * BYTES_PER_DIR_ENTRY / bytes_per_sector +
        reserved_sectors_num + fat_size * fats_num;
}

bool fat_init(char *media_file_name)
{
    if (!hal_init(media_file_name))
        return false;
    read_boot_sector();
    print_boot_sector();
    return true;
}

void read_file(Entry file, int current_dir_cluster)
{
    int length = strlen(file.name), i, bytes_per_cluster = bytes_per_sector * sectors_per_cluster,
        next_cluster, last_fat_sector = -1;
    unsigned char tmp_file_name[12], ext[3],
        *one_cluster_buffer = (unsigned char*) malloc(sizeof(unsigned char) * bytes_per_cluster),
        *one_sector_buffer = (unsigned char*) malloc(sizeof(unsigned char) * bytes_per_sector);
    FILE *fp2;

    ext[3] = 0;
    ext[2] = tolower(file.name[length - 1]);
    ext[1] = tolower(file.name[length - 2]);
    ext[0] = tolower(file.name[length - 3]);
    strcpy(tmp_file_name, TMP_FILE_NAME);
    strcat(tmp_file_name, ext);
    fp2 = fopen(tmp_file_name, "wb");
    next_cluster = file.first_cluster;
    while (next_cluster != LAST_CLUSTER_SIG) {
        kmc_read_multi_sector(data_to_physical_cluster(next_cluster),
            sectors_per_cluster, one_cluster_buffer);
        if (cal_next_cluster_fat12(next_cluster, &last_fat_sector, one_sector_buffer) == LAST_CLUSTER_SIG) {
            fwrite(one_cluster_buffer, sizeof(unsigned char), file.size % bytes_per_cluster, fp2);
        } else {
            fwrite(one_cluster_buffer, sizeof(unsigned char), bytes_per_cluster, fp2);
        }
        next_cluster = cal_next_cluster_fat12(next_cluster, &last_fat_sector, one_sector_buffer);
    }
    
    fclose(fp2);
    ShellExecute(0, 0, tmp_file_name, 0, 0, SW_SHOW);
    list_dir(current_dir_cluster);
    remove(tmp_file_name);
}

int read_fat_sector(unsigned int cluster_offset, unsigned char *buff)
{
    return kmc_read_sector(cluster_offset / bytes_per_sector + 1, buff);
}

int read_fat_sector_subsequently(unsigned int cluster_offset, unsigned char *buff)
{
    return kmc_read_sector(cluster_offset / bytes_per_sector + 2, buff);
}

void open_sub_dir(int cluster_index)
{
    int i, dirs_num = 0, files_num = 0,
        file_index = 0, dir_index = 0,
        bytes_per_cluster = bytes_per_sector * sectors_per_cluster,
        next_cluster, last_fat_sector = -1;
    Entry *file_arr, *dir_arr;
    unsigned char *one_cluster_buffer = (unsigned char*) malloc(sizeof(unsigned char*) * bytes_per_cluster),
        *one_sector_buffer = (unsigned char*) malloc(sizeof(unsigned char*) * bytes_per_sector);

    next_cluster = cluster_index;
    while (next_cluster != LAST_CLUSTER_SIG) {
        kmc_read_multi_sector(data_to_physical_cluster(next_cluster),
            sectors_per_cluster, one_cluster_buffer);
        i = 0;
        while (one_cluster_buffer[i]) {
            if (one_cluster_buffer[i + 11] == 0x10) {
                dirs_num++;
            } else if (one_cluster_buffer[i + 11] == 0x0f) {
                i += BYTES_PER_DIR_ENTRY;
                continue;
            } else if (one_cluster_buffer[i] != 0xe5 && one_cluster_buffer[i] != 0) {
                files_num++;
            }
            i += BYTES_PER_DIR_ENTRY;
        }
        next_cluster = cal_next_cluster_fat12(next_cluster, &last_fat_sector, one_sector_buffer);
    }

    dir_arr = (Entry*) malloc(dirs_num * sizeof(Entry));
    file_arr = (Entry*) malloc(files_num * sizeof(Entry));

    // Reading Entries
    next_cluster = cluster_index;
    while (next_cluster != LAST_CLUSTER_SIG) {
        kmc_read_multi_sector(data_to_physical_cluster(next_cluster),
            sectors_per_cluster, one_cluster_buffer);
        i = 0;
        while (one_cluster_buffer[i]) {
            if (one_cluster_buffer[i + 11] == 0x10) {
                read_entry(&dir_arr[dir_index], one_cluster_buffer, i);
                dir_index++;
            }
            else if (one_cluster_buffer[i + 11] == 0x0f) {
                i += BYTES_PER_DIR_ENTRY;
                continue;
            }
            else if (one_cluster_buffer[i] != 0xe5) {
                read_entry(&file_arr[file_index], one_cluster_buffer, i);
                file_index++;
            }
            i += BYTES_PER_DIR_ENTRY;
        }
        next_cluster = cal_next_cluster_fat12(next_cluster, &last_fat_sector, one_sector_buffer);
    }
    print_list(file_arr, files_num, dir_arr, dirs_num, cluster_index);
}
