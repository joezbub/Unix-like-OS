/*
Stores the directory functions.
The directory maps the file information/name to the first block that stores the file in the FAT.
*/
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include "utils.h"

typedef struct block block;

struct block
{
    char name[32];
    uint32_t size;
    uint16_t firstBlock;
    uint8_t type;
    uint8_t perm;
    time_t mtime;
};

typedef struct fat_context fat_context;

struct fat_context
{
    int BLOCK_SIZE;
    int LSB;
    int MSB;
    int NUM_FAT_BLOCKS;
    int NUM_DATA_BLOCKS;
    int NUM_FAT;
    int NUM_FAT_BYTES;
    int NUM_DATA_BYTES;
    int DIR_SIZE;
    int DIRS_PER_BLOCK;
    int fd;
    char *empty_block;
    char *empty_dir;
};

fat_context *constructor(int lsb, int num_fat_blocks, char *filename);

void initialize(fat_context *fc);

void seek_data_region(int block_index, fat_context *fc);

void seek_dir_data_region(int block_index, int dir_i, fat_context *fc);

void seek_to_write_data(int block_index, int curr_size, fat_context *fc);

uint16_t find_available_block(fat_context *fc);

void write_block_fat(int block_index, uint16_t value, fat_context *fc);

uint16_t read_fat_block(int block_index, fat_context *fc);

int find_last_block(int first_block, fat_context *fc);

void write_next_dir_entry(block *dir_entry, fat_context *fc);

int create(char *filename, uint8_t perm, fat_context *fc);

block *dir_to_struct(char *dir, fat_context *fc);

block *find_dir_entry(char *filename, fat_context *fc);

block *read_dir_entry(int dir_block, int dir_num, fat_context *fc);

void update_dir_entry(char *filename, block *dir_entry, fat_context *fc);

void clear_file(char *filename, fat_context *fc);

void fatwrite(char *filename, char *value, int len, fat_context *fc);

char *fatreadblock(int block_index, fat_context *fc);

void fatread(char *filename, fat_context *fc);

void fatremove(char *filename, fat_context *fc);

#endif // !FILESYSTEM_H