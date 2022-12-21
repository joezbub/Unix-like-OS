#include "filesystem.h"
const uint16_t root = 0xFFFF; // fat[1]
const uint8_t zeros = 0x0000; //everything else
/*
FAT:
index
  0  | 2002
  1  | FFFF
  ...
___________
data region:
  0  | 0 block, empty - index = NUM_FAT_BYTES + 0*BLOCK_SIZE
  1  | first directory block, index = NUM_FAT_BYTES + 1*BLOCK_SIZE
  ...
*/

fat_context* constructor(int lsb, int num_fat_blocks, char* filename) {
    if (num_fat_blocks > 32 || num_fat_blocks < 1) {
        printf("%s\n", "ERROR: invalid fat size");
        return NULL;
    }
    struct fat_context* fc = (struct fat_context*) malloc(sizeof(struct fat_context));
    fc->fd = open(filename, O_RDWR | O_CREAT);
    fc->LSB = lsb;
    if (lsb == 0) {
        fc->BLOCK_SIZE = 256;
    } else if (lsb == 1) {
        fc->BLOCK_SIZE = 512;
    } else if (lsb == 2) {
        fc->BLOCK_SIZE = 1024;
    } else if (lsb == 3) {
        fc->BLOCK_SIZE = 2048;
    } else if (lsb == 4) {
        fc->BLOCK_SIZE = 4096;
    } else {
        printf("%s\n", "ERROR: invalid block size");
        return NULL;
    }
    fc->NUM_FAT_BLOCKS = num_fat_blocks; // 32
    fc->NUM_FAT = ((fc->BLOCK_SIZE)*(fc->NUM_FAT_BLOCKS)) / 2;
    fc->NUM_DATA_BLOCKS = fc->NUM_FAT;
    fc->NUM_FAT_BYTES = (fc->BLOCK_SIZE)*(fc->NUM_FAT_BLOCKS);
    fc->NUM_DATA_BYTES = (fc->BLOCK_SIZE)*(fc->NUM_DATA_BLOCKS);
    fc->DIR_SIZE = 64; // fixed
    fc->DIRS_PER_BLOCK = (fc->BLOCK_SIZE)/(fc->DIR_SIZE);
    fc->LSB = lsb;
    
    printf("block size: %d\n", fc->BLOCK_SIZE);
    printf("number of fat blocks: %d\n", fc->NUM_FAT_BLOCKS);
    printf("number of data blocks: %d\n", fc->NUM_DATA_BLOCKS);
    printf("number of fat entries: %d\n", fc->NUM_FAT);
    printf("number of fat bytes: %d\n", fc->NUM_FAT_BYTES);
    printf("number of data bytes: %d\n", fc->NUM_DATA_BYTES);
    printf("number of directories per block: %d\n", fc->DIRS_PER_BLOCK);

    fc->empty_block = malloc(fc->BLOCK_SIZE);
    for (int i = 0; i < fc->BLOCK_SIZE; i++) {
        fc->empty_block[i] = 0x0000;
    }
    fc->empty_dir = malloc(fc->DIR_SIZE);
    for (int i = 0; i < fc->DIR_SIZE; i++) {
        fc->empty_dir[i] = 0x0000;
    }

    return fc;
}

void initialize(fat_context* fc) {
    // initializes an empty filesystem in the specified file
    // hexdump 
    // fat [0] fat [1] 1024*32 bytes of 0s
    // FILE* fd = fopen("binary_file.bin", "w+");
    int lsb = fc->LSB;
    uint16_t metadata = char_to_16((char)fc->NUM_FAT_BLOCKS, (char)lsb);
    printf("%hu\n", metadata);
    write(fc->fd, &metadata, 2);
    write(fc->fd, &root, 2);
    for (int i = 0; i < fc->NUM_FAT_BYTES - 4; i ++) { // all remaining fat
        write(fc->fd, &zeros, 1);
    }
    for (int i = 0; i < fc->NUM_DATA_BYTES; i++) {
        write(fc->fd, &zeros, 1);
    }
}

void seek_data_region(int block_index, fat_context* fc) {
    // printf("block_index: %d\n", block_index);
    // printf("num to seek: %d\n", (block_index*(fc->BLOCK_SIZE) + fc->NUM_FAT_BYTES));
    lseek(fc->fd, block_index*(fc->BLOCK_SIZE) + fc->NUM_FAT_BYTES, SEEK_SET);
}

void seek_dir_data_region(int block_index, int dir_i, fat_context* fc) {
    // printf("block_index: %d\n", block_index);
    // printf("data reg to seek: %d\n", ((block_index*(fc->BLOCK_SIZE)) + (fc->NUM_FAT_BYTES) + (dir_i*(fc->DIR_SIZE))));
    lseek(fc->fd, (block_index*(fc->BLOCK_SIZE)) + fc->NUM_FAT_BYTES + (dir_i*(fc->DIR_SIZE)), SEEK_SET);
}

void seek_to_write_data(int block_index, int curr_size, fat_context* fc) {
    lseek(fc->fd, block_index*(fc->BLOCK_SIZE) + (fc->NUM_FAT_BYTES) + curr_size, SEEK_SET);
}

uint16_t find_available_block(fat_context* fc) {
    // scan for the next free block 
    // we're looking in the FAT
    int count = 1; // the index that we're on 
    uint16_t res = 0x0000;
    char* buf = malloc(2);
    while (count < fc->NUM_FAT) {
        lseek(fc->fd, 2*count, SEEK_SET);
        read(fc->fd, buf, 2); // read the next 2 bytes. read will move the file offset. 
        if (is_zero(buf)) {
            res = (uint16_t) count; // set the result
            break;
        }
        count += 1;
    }
    return res;
}

void write_block_fat(int block_index, uint16_t value, fat_context* fc) {
    lseek(fc->fd, block_index*2, SEEK_SET); // lseek seeks by bytes, 2 bytes in every block
    write(fc->fd, &value, 2);
}

uint16_t read_fat_block(int block_index, fat_context* fc) {
    lseek(fc->fd, block_index*2, SEEK_SET);
    char* buf = malloc(2);
    read(fc->fd, buf, 2);
    uint16_t link = char_to_16(buf[0], buf[1]);
    return link;
}


int find_last_block(int first_block, fat_context* fc) {
    uint16_t link = read_fat_block(first_block, fc); // read the link of the first block
    int block = first_block; // curr block is the first block 
    while (link != root) { // while this current block isn't the last block
        block = (int) link; // find the next block index
        link = read_fat_block(block, fc); // read the next block
    }

    return block;
}

void write_next_dir_entry(block* dir_entry, fat_context* fc) {
    // finds the next free directory entry
    int last_dir_block = find_last_block(1, fc); // we know that the first fat block is the first dir block.
    // int start_block = 1;
    // printf("last_dir_block: %d\n", last_dir_block);
    // go to the data region corresponding to the block
    int link = 1;

    // case 1: dir block is not full
    // case 2: dir block is full, allocate new fat block.
    char* buf = malloc(fc->DIR_SIZE);
    do {
        seek_data_region(link, fc);
        for (int i = 0; i < fc->DIRS_PER_BLOCK; i++ ) {
            read(fc->fd, buf, fc->DIR_SIZE);
            if (is_zero(buf)) {
                seek_dir_data_region(link, i, fc);
                write(fc->fd, dir_entry, fc->DIR_SIZE);
                return;
            }
            
        }
        link = (int)read_fat_block(link, fc);
    } while (link != root);
    
    // if we get here, it's all full
    uint16_t next_dir_fat_entry = find_available_block(fc);
    write_block_fat(last_dir_block, next_dir_fat_entry, fc);
    write_block_fat(next_dir_fat_entry, root, fc); // update the fat with this new data region block
    // fill in the data region block with the dir

    int new_last_block = (int) next_dir_fat_entry;
    seek_data_region(new_last_block, fc);
    write(fc->fd, dir_entry, fc->DIR_SIZE);
    return;
}

int create(char* filename, uint8_t perm, fat_context* fc) {
    // create an empty file
    // create the directory block
    uint16_t firstBlock = find_available_block(fc); // first block in fat that is available
    struct block* new_dir = (struct block*) malloc(sizeof(struct block));
    strcpy(new_dir->name, filename);
    new_dir->size = 0x00000;
    new_dir->firstBlock = firstBlock;
    new_dir->type = 1;
    new_dir->perm = 6;
    time_t seconds;
    time(&seconds);
    new_dir->mtime = seconds;

    // write the directory block to the fat
    write_block_fat(firstBlock, root, fc);
    // write the directory block to the data region
    write_next_dir_entry(new_dir, fc);
    return (int) firstBlock;
}



block* dir_to_struct(char* dir, fat_context* fc) { 
    struct block* new_dir = (struct block*) malloc(sizeof(struct block));
    char name[32];
    for (int i = 0; i < 32; i ++) {
        name[i] = dir[i];
    }
    strcpy(new_dir->name, name);
    uint32_t size = char_to_32(dir[32], dir[33], dir[34], dir[35]);
    new_dir->size = size;
    uint16_t firstBlock = char_to_16(dir[36], dir[37]);
    new_dir->firstBlock = firstBlock;
    uint8_t type = char_to_8(dir[38]);
    new_dir->type = type;
    uint8_t perm = char_to_8(dir[39]);
    new_dir->perm = perm;

    uint32_t time = char_to_32(dir[40], dir[41], dir[42], dir[43]);
    new_dir->mtime = (time_t) time;
    return new_dir;
}

block* find_dir_entry(char* filename, fat_context* fc) {
    // find the directory block corresponding to a filename
    int curr_block = 1; // representing the fat block we are currently on. 
    uint16_t link; // value of the link of the current fat block.
    char* buf = malloc(fc->DIR_SIZE);

    do {
        lseek(fc->fd, fc->NUM_FAT_BYTES + curr_block*(fc->BLOCK_SIZE), SEEK_SET); // go to data region block.
        for (int i = 0; i < fc->DIRS_PER_BLOCK; i++) {
            // iterate through dir blocks 
            read(fc->fd, buf, fc->DIR_SIZE);
            block* dir_block = dir_to_struct(buf, fc);
            if (strcmp(dir_block->name, filename) == 0) {
                return dir_block;
            } 
        }
        link = read_fat_block(curr_block, fc);
        curr_block = (int) link;
    } while (link != root);
    return NULL;
}

block* read_dir_entry(int dir_block, int dir_num, fat_context* fc) {
    seek_dir_data_region(dir_block, dir_num, fc);
    char* buf = malloc(fc->DIR_SIZE);
    read(fc->fd, buf, fc->DIR_SIZE);
    return dir_to_struct(buf, fc);
}

void clear_data_region(int data_block, fat_context*fc) {
    seek_data_region(data_block, fc);
    write(fc->fd, fc->empty_block, fc->BLOCK_SIZE);
}

void update_dir_entry(char* filename, block* dir_entry, fat_context* fc) {
    int curr_block = 1;
    uint16_t link;
    char* buf = malloc(fc->DIR_SIZE);
    do{
        lseek(fc->fd, fc->NUM_FAT_BYTES + curr_block*(fc->BLOCK_SIZE), SEEK_SET);
        for (int i = 0; i < fc->DIRS_PER_BLOCK; i++) {
            read(fc->fd, buf, fc->DIR_SIZE);
            block* dir_block = dir_to_struct(buf, fc);
            if (strcmp(dir_block->name, filename) == 0) {
                seek_dir_data_region(curr_block, i, fc);
                if (dir_entry != NULL) {
                    write(fc->fd, dir_entry, fc->DIR_SIZE); // either fill it in with a new dir
                } else {
                    write(fc->fd, fc->empty_dir, fc->DIR_SIZE); // erase it 
                }
                return;
            }
        }
        link = read_fat_block(curr_block, fc);
        curr_block = (int) link;
    } while (link != root);
    printf("%s\n", "FILE NOT FOUND");
}

void clear_file(char* filename, fat_context* fc) {
    block* dir_entry = find_dir_entry(filename, fc);
    dir_entry->size = 0;
    int link = (int)dir_entry->firstBlock;
    do {
        // printf("curr: %d\n", link);
        seek_data_region(link, fc);
        write(fc->fd, fc->empty_block, fc->BLOCK_SIZE);
        // overwrite the fat back to 0 too.
        int temp = link;
        link = (int)read_fat_block(link, fc);
        write_block_fat(temp, zeros, fc);
        clear_data_region(temp, fc);
    } while (link != root);
    write_block_fat((int)dir_entry->firstBlock, root, fc);
    update_dir_entry(dir_entry->name, dir_entry, fc);
}

void fatwrite(char* filename, char* value, int len, fat_context* fc) {
    // examine the current fat spot
    // see if we can fill the current space 
    block* dir_block = find_dir_entry(filename, fc);
    if (dir_block == NULL) {
        printf("%s\n", "FILE NOT FOUND");
        return;
    }
    int size = (int)dir_block->size;
    int rem = fc->BLOCK_SIZE - size % (fc->BLOCK_SIZE);
    // printf("rem: %d\n", rem);
    // printf("len: %d\n", len);
    int last_block = find_last_block((int)dir_block->firstBlock, fc);
    // printf("last: %d\n", last_block);
    int curr_val_ptr = 0;
    int curr_fat_index = dir_block->firstBlock;
    // printf("first: %d\n", curr_fat_index);

    dir_block->size += len;
    // re-write the dir_block
    update_dir_entry(dir_block->name, dir_block, fc);
    
    // fill the last block
    seek_to_write_data(last_block, size, fc);
    if (len <= rem) {
        // printf("%s\n", "hello");
        // printf("%s\n", value);
        // fill the block
        write(fc->fd, value, len);
    } else {
        // fill the block partway 
        char* curr_buf = malloc(rem);
        for (int i = 0; i < rem; i++) {
            curr_buf[i] = value[i];
        }
        write(fc->fd, value, rem);

        // we've already processed rem characters in value
        curr_val_ptr = rem;

        len -= rem; // subtract the already written values

        // fill more blocks
        char* value_buf = malloc(fc->BLOCK_SIZE);
        int num_blocks_to_allocate = len/(fc->BLOCK_SIZE) + (len % fc->BLOCK_SIZE != 0) ; // round up the number of blocks to allocate
        // find a fat spot that is available
        for (int i = 0; i < num_blocks_to_allocate; i++) {
            uint16_t block_index = find_available_block(fc);
            // printf("index: %d\n", (int)block_index);
            // assign the fat 
            write_block_fat(curr_fat_index, block_index, fc);
            curr_fat_index = (int)block_index;
            // assign the new fat block.
            write_block_fat(curr_fat_index, root, fc);

            // write the value
            // amount to write is either the entire block size, or less.  
            int to_write = MIN(fc->BLOCK_SIZE, len); 
            // printf("to write: %d\n", to_write);
            for (int j = 0; j < to_write; j++) {
                value_buf[j] = value[j + curr_val_ptr];
            }
            seek_data_region(curr_fat_index, fc); // go to the place in the program
            write(fc->fd, value_buf, to_write); // write the value
            curr_val_ptr += to_write; // increment the curr value pointer 
        }
    }
}

char* fatreadblock(int block_index, fat_context* fc) {
    // given the block_index, return a string pointer to the text within the block.
    char* text = malloc(fc->BLOCK_SIZE);
    seek_data_region(block_index, fc);
    read(fc->fd, text, fc->BLOCK_SIZE);
    return text;
}

void fatread(char* filename, fat_context* fc) {
    // go to the fat and iterate through the directory blocks to match the strings.
    // once the file is found, while the fat link is not EOF, call the fatreadblock function on the block index. 
    block* dir_block = find_dir_entry(filename, fc);
    int link = (int)dir_block->firstBlock;
    do {
        char* filePart = fatreadblock(link, fc);
        printf("%s\n", filePart); // PRINTS TO TERMINAL
        link = read_fat_block(link, fc);
    } while (link != root);
}

void fatremove(char* filename, fat_context* fc) {
    block* dir_block = find_dir_entry(filename, fc);
    if (dir_block == NULL) {
        printf("%s\n", "ERROR: file not found");
        return;
    }
    
    int link = (int)dir_block->firstBlock;
    

    do {
        // go to the file and overwrite the data region with 0s
        // printf("curr: %d\n", link);
        seek_data_region(link, fc);
        write(fc->fd, fc->empty_block, fc->BLOCK_SIZE);
        // overwrite the fat back to 0 too.
        int temp = link;
        link = (int)read_fat_block(link, fc);
        write_block_fat(temp, zeros, fc);
    } while (link != root);

    update_dir_entry(dir_block->name, NULL, fc);
}

// int main(int argc, char** argv) {
//    fat_context* fc = constructor(1024, 32, "bf.bin");
//    initialize(fc);
//        lseek(fc->fd, 0, SEEK_SET);
//     create("test.txt", 0);
//     lseek(fd, BLOCK_SIZE + NUM_FAT_BYTES, SEEK_SET);
//     char* buf = malloc(64);
//     read(fd, buf, 64);
//     printf("%s\n", buf);
//     if (is_zero(buf)){
//         printf("%s\n", "hello");
//     }
//     lseek(fd, NUM_FAT_BYTES + 2*1024, SEEK_SET);
//     char* buf = malloc(64);
//     read(fd, buf, 64);
//     dir_to_struct(buf);
//     char* val = malloc(sizeof("Hi I'm Annie and this is my 380 assignmentHi I'm Annie and this is my 380 assignmentHi I'm Annie and this is my 380 assignmentHi I'm Annie and this is my 380 assignmentHi I'm Annie and this is my 380 assignment"));
//     val = "Hi I'm Annie and this is my 380 assignmentHi I'm Annie and this is my 380 assignmentHi I'm Annie and this is my 380 assignmentHi I'm Annie and this is my 380 assignmentHi I'm Annie and this is my 380 assignment";
//     fatwrite("test.txt", val);
//     uint16_t next = find_available_block();
//     printf("next: %d\n", (int)next);
//     write_block_fat(2, next);
//     seek_data_region((int)next);
//     fatread("test.txt");
//     fatremove("test.txt");
// }