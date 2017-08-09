#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include "fat.h"

#define ROOT_DIRECTORY 0

bool is_init_fat = false;
char *file_name;

void list_dir(int cluster_index)
{
    unsigned char *root_dir_buffer;
    int i, dirs_num = 0, files_num = 0,
        file_index = 0, dir_index = 0;
    Entry *file_arr, *dir_arr;
    if (!is_init_fat) {
        if (!fat_init(file_name)) {
            return;
        }
        is_init_fat = true;
    }
    if (cluster_index >= 2) {
        open_sub_dir(cluster_index);
    } else { // Open Root Dir
        root_dir_buffer = (unsigned char*) malloc(
            sizeof(unsigned char) * (get_max_root_entry_num() * BYTES_PER_DIR_ENTRY));
        kmc_read_multi_sector(
            get_reserved_sectors_num() + get_fats_num() * get_fat_size(),
            get_max_root_entry_num() * BYTES_PER_DIR_ENTRY / get_bytes_per_sector(),
            root_dir_buffer
        );
        i = 0;
        while (root_dir_buffer[i]) {
            if (root_dir_buffer[i + 11] == 0x10) {
                dirs_num++;
            } else if (root_dir_buffer[i + 11] == 0x0f) {
                i += BYTES_PER_DIR_ENTRY;
                continue;
            } else if (root_dir_buffer[i] != 0xe5 && root_dir_buffer[i] != 0) {
                files_num++;
            }
            i += BYTES_PER_DIR_ENTRY;
        }
        dir_arr = (Entry*) malloc(dirs_num * sizeof(Entry));
        file_arr = (Entry*) malloc(files_num * sizeof(Entry));

        i = 0;
        while (root_dir_buffer[i]) {
            if (root_dir_buffer[i + 11] == 0x10) {
                read_entry(&dir_arr[dir_index], root_dir_buffer, i);
                dir_index++;
            }
            else if (root_dir_buffer[i + 11] == 0x0f) {
                i += BYTES_PER_DIR_ENTRY;
                continue;
            }
            else if (root_dir_buffer[i] != 0xe5) {
                read_entry(&file_arr[file_index], root_dir_buffer, i);
                file_index++;
            }
            i += BYTES_PER_DIR_ENTRY;
        }
        print_list(file_arr, files_num, dir_arr, dirs_num, cluster_index);
    }
}

void print_content(int cluster, int clusters_num, char* format)
{
    // This func is just to debug, app. layer (file main.c) does not include this func.
    int i, number_of_bytes = clusters_num * get_bytes_per_sector(),
        first_byte = cluster * get_bytes_per_sector();
    unsigned char *buffer = (unsigned char*) malloc(number_of_bytes);
    kmc_read_multi_sector(cluster, clusters_num, buffer);

    printf("\nContent from byte 0x%04x to byte 0x%04x:\n", first_byte, first_byte + number_of_bytes - 1);
    printf("        0 ");
    for (i = 1; i < 16; i++)
        printf("%2x ", i);
    printf("\n");
    printf("0x%04x ", first_byte);
    for (i = 0; i < number_of_bytes; i++)
    {
        printf(format, buffer[i]);
        if (!((i + 1) % 16) && i > 0 && i != (number_of_bytes - 1))
            printf("\n0x%04x ", first_byte + i + 1);
    }
}

int main(int argc, char *argv[])
{
    file_name = (char*) malloc(sizeof(char) * strlen(argv[1]));
    strcpy(file_name, argv[1]);
    list_dir(ROOT_DIRECTORY);
    
    /*fat_init(file_name);
    print_content(19, 1, "%02x ");
    print_content(20, 1, "%02x ");*/
    

    // print_content(923, 1, "%c");
    // print_content(923, 1, "%02x ");
    // print_content(924, 1, "%c");
    // print_content(924, 1, "%02x ");
    // print_content(929, 1, "%c");
    // print_content(929, 1, "%02x ");
    // print_content(((0x37c / 2) + 0x37c)/512 + 1, 1, "%c");
    // print_content(((0x37c / 2) + 0x37c)/512 + 1, 1, "%02x ");
}
