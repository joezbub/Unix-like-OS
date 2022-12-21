/*
 * pennfat executable shell
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#define MAXLINELENGTH 4096
#include "filesystem.h"

const uint16_t eof = 0xFFFF;

uint16_t *fat;
fat_context* fc;

void mkfs(char* fs_name, int blocks_in_fat, int block_size_config) {
    fc = constructor(block_size_config, blocks_in_fat, fs_name);
    // printf("%d\n", fc->fd);
    initialize(fc);
    printf("%d\n", fc->BLOCK_SIZE);
    printf("%d\n", fc->DIRS_PER_BLOCK);
}

void fake_initialize() {
    fc = constructor(1024, 32, "t.bin");
    initialize(fc);
    // int fd1 = open("t.bin", O_RDWR | O_CREAT);
    // fc->fd = fd1;
}

void mount(char* fs_name){
    int fs_fd = open(fs_name, O_RDWR);
    char buf[2];
    read(fs_fd, buf, 2);
    int lsb = (int) char_to_8(buf[1]); // size of block: 0: 256, 1: 512, 2: 1024, 3: 2048, 4: 4096
    int msb = (int) char_to_8(buf[0]); // fat size
    printf("%d\n", lsb);
    printf("%d\n", msb);
    if (lsb < 0 || lsb > 4) {
        printf("%s\n", "ERROR: invalid block size");
    }
    fc = constructor(lsb, msb, fs_name);
    // fat = mmap(NULL, fc->NUM_FAT_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
}

void unmount(){
    fc = NULL;
    return;
}

void touch(char** files, int num_files) {
    for (int i = 0; i < num_files; i++) {
        char* file = files[i];
        // printf("%s\n", file);

        block* dir = find_dir_entry(file, fc);
        if (dir != NULL) {
            // file exists
            time_t seconds;
            time(&seconds);
            dir->mtime = seconds;
            update_dir_entry(dir->name, dir, fc);
        } else {
            create(file, 7, fc);
        }
    }
}

void mv(char* source, char* dest) {
    // grab the current file
    // modify the file name to dest
    
    block* source_dir = find_dir_entry(source, fc);
    block* dest_dir = find_dir_entry(dest, fc);
    if (source_dir == NULL) {
        printf("%s\n", "ERROR: file not found");
        return;
    }
    if (dest_dir != NULL) {
        fatremove(dest, fc);
    }
    for (int i = 0; i < 32; i++) {
        if (dest[i] != '\0') {
            source_dir->name[i] = dest[i];
        }
    }
    update_dir_entry(source, source_dir, fc);
}

void rm(char* file) {
    fatremove(file, fc);
}

void cp(char* source, char* dest, bool source_os, bool dest_os) {
    // create files if they don't exist:
    block* src = find_dir_entry(source, fc);
    block* dt = find_dir_entry(dest, fc);
    if (src == NULL && !source_os) {
        printf("%s\n", "ERROR: file not found");
        return;
    }
    if (dt == NULL && !dest_os) {
        create(dest, 7, fc);
    }
    if (!source_os && !dest_os) { // read source and dest from filesystem
        block* src_dir = find_dir_entry(source, fc);
        // fatread from src_dir and fatwrite to dest_dir
        int link = (int)src_dir->firstBlock;
        do {
            char* filePart = fatreadblock(link, fc);
            fatwrite(dest, filePart, strlen(filePart), fc);
            link = read_fat_block(link, fc);
        } while (link != eof);
    } else if (source_os && !dest_os) { // read source from os and dest from filesystem
        int fd1 = open(source, O_RDWR | O_CREAT);
        int file_size = lseek(fd1, 0, SEEK_END);
        // printf("file size: %d\n", file_size);
        char value[file_size];
        int i = 0;

        // printf("%d\n", file_size);
        lseek(fd1, 0, SEEK_SET);
        read(fd1, value, file_size);
        while (value[i] != '\0') {
            i ++;
        }
        // printf("strlen: %d\n", i);
        // printf("to cp: %s\n", value);
        fatwrite(dest, value, file_size, fc);
    } else if (!source_os && dest_os) { // read source from filesystem and dest from os
        int fd2 = open(dest, O_RDWR | O_CREAT);
        int file_size = lseek(fd2, 0, SEEK_END);
        char* value = malloc(file_size);
        lseek(fd2, 0, SEEK_SET);
        read(fd2, value, file_size);
        block* src_dir = find_dir_entry(source, fc);
        int link = (int)src_dir->firstBlock;
        int size = (int)src_dir->size;
        do {
            char* filePart = fatreadblock(link, fc);
            // printf("%s\n", filePart);
            write(fd2, filePart, size);
            link = read_fat_block(link, fc);
        } while (link != (int)eof);
        // write(fd2, '\0', 1);
        // printf("size: %d\n", size);

    }
}

void cat_from_terminal(char* to_write, char* output, bool overwrite) {
    block* output_block;
    if (output != NULL) {
        output_block = find_dir_entry(output, fc); // find the dir entry for the output file, if it exists.
        if (output_block == NULL) {
            // create the file if it doesnt exist
            create(output, 7, fc);
        } 
        if (overwrite) {
            clear_file(output_block->name, fc);
        }
        fatwrite(output, to_write, strlen(to_write), fc);
    } else {
        printf("%s\n", to_write);
    }
    
}


void cat(char** files, int num_files, char* output, bool overwrite) {
    block* output_block;
    if (output != NULL) {
        output_block = find_dir_entry(output, fc); // find the dir entry for the output file, if it exists. 
    }
    if (output != NULL && output_block == NULL) { // only create if we need output
        // create the file if it doesnt exist
        create(output, 7, fc);
    }
    for (int i = 0; i < num_files; i++) {
        char* file = files[i];
        // printf("file: %s\n", file);
        block* dir_block = find_dir_entry(file, fc);
        if (dir_block == NULL) {
            printf("%s\n", "ERROR: File not found");
            return;
        }
        // printf("file found: %s\n", dir_block->name);
        if (overwrite) {
            clear_file(output_block->name, fc);
        }
        // read the entire file. If output, we write to output. 
        int link = (int)dir_block->firstBlock;
        do {
            char* filePart = fatreadblock(link, fc);
            if (output == NULL) {
                printf("%s", filePart); // printing to the terminal DO NOT COMMENT OUT
            } else {
                // TODO: check overwrite or not.
                fatwrite(output, filePart, strlen(filePart), fc);
                
            }
            link = (int)read_fat_block(link, fc);
        } while (link != eof);
        if (output == NULL) {
            printf("\n"); // printing to terminal DO NOT COMMENT OUT
        }
    }
}

void ls() {
    // TODO: add extra info 
    int curr_block = 1;
    uint16_t link;
    // printf("%d\n", fc->DIRS_PER_BLOCK);
    do {
        for (int i = 0; i < fc->DIRS_PER_BLOCK; i++) {
            block* dir = read_dir_entry(curr_block, i, fc);
            if (!is_zero(dir->name)) {
                printf("%d ", curr_block); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                struct tm *currTime = localtime(&(dir->mtime));
                int hours = currTime->tm_hour;
                int minutes = currTime->tm_min;  
                int day = currTime->tm_mday;
                int month = currTime->tm_mon + 1;
                char monthChar[4];
                if (month == 1) {
                    strcpy(monthChar, "Jan");
                } else if (month == 2) {
                    strcpy(monthChar, "Feb");
                } else if (month == 3) {
                    strcpy(monthChar, "Mar");
                } else if (month == 4) {
                    strcpy(monthChar, "Apr");
                } else if (month == 5) {
                    strcpy(monthChar, "May");
                } else if (month == 6) {
                    strcpy(monthChar, "Jun");
                } else if (month == 7) {
                    strcpy(monthChar, "Jul");
                } else if (month == 8) {
                    strcpy(monthChar, "Aug");
                } else if (month == 9) {
                    strcpy(monthChar, "Sep");
                } else if (month == 10) {
                    strcpy(monthChar, "Oct");
                } else if (month == 11) {
                    strcpy(monthChar, "Nov");
                } else if (month == 12) {
                    strcpy(monthChar, "Dec");
                } else {
                    printf("%s\n", "ERROR: Invalid Date");
                }
                if (dir->perm == 0) {
                    printf("%s ", "---"); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                } else if (dir->perm == 2) {
                    printf("%s ", "-w-"); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                } else if (dir->perm == 4) {
                    printf("%s ", "r--"); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                } else if (dir->perm == 5) {
                    printf("%s ", "r-x"); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                } else if (dir->perm == 6) {
                    printf("%s ", "rw-"); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                } else if (dir->perm == 7) {
                    printf("%s ", "rwx"); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                } else {
                    printf("%s\n", "ERROR: Invalid Permissions");
                }
                printf("%d ", dir->size); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                printf("%s ", monthChar); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                printf("%02d ", day); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                printf("%02d:%02d ", hours, minutes); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                printf("%s\n", dir->name); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
            }
        }
        // printf("curr block: %d\n", curr_block);
        link = read_fat_block(curr_block, fc);
        // printf("link: %d\n", link);
        curr_block = (int)link;
        // printf("curr eof symbol: %d\n", eof);
    } while (link != eof);
}

void chmod(char* filename, int perm) {
    block* dir_block = find_dir_entry(filename, fc);
    dir_block->perm = perm;
    update_dir_entry(dir_block->name, dir_block, fc);
}

int count_args(char *str, int length) {
    int count = 0;
    int curr = 0;
    while (curr < length) {
        if (str[curr] == ' ') {
            curr ++;
        } else{
            count++;
            while((str[curr] != ' ') && curr < length) {
                curr ++;
            }
        }
    }
    return count;
}

int main(int argc, char** argv) {
    while(1) {
        //printf("%s\n", "pennfat#");
        write(STDIN_FILENO, "pennfat# ", strlen("pennfat# "));
        const int maxLineLength = 4096;
        char cmd[maxLineLength];
        for (int index = 0; index < maxLineLength; index++) {
            cmd[index] = '\0';
        }
        
        int numBytes = read(STDIN_FILENO, cmd, MAXLINELENGTH);
        if (cmd[numBytes - 1] == '\n') { // Get rid of trailing newline.
            cmd[numBytes - 1] = ' ';
        }        
        
        int numArguments = count_args(cmd, strlen(cmd));

        //tokenize the string and allocate memory with size equivalent to number of arguments
        char* tokens = strtok(cmd, " ");
        int index = 0;
        char** arguments = malloc(sizeof(char*) * (numArguments + 1));
        while (tokens != NULL) {
            arguments[index] = tokens;
            tokens = strtok(NULL, " ");
            index += 1;
        }
        //make sure the last element in the arguments array is null terminated
        arguments[numArguments] = NULL;

        //case on the commands to see what function to run
        // if (fc == NULL && strcmp(arguments[0], "mkfs") != 0) {
        //     printf("%s\n", "ERROR: no file mounted");
        //     continue;
        // }
        
        if (strcmp(arguments[0], "mkfs") == 0) {
            // printf("%d\n", atoi(arguments[2]));
            mkfs(arguments[1], atoi(arguments[2]), atoi(arguments[3]));
        } else if (strcmp(arguments[0], "mount") == 0) {
            mount(arguments[1]);
        } else if (strcmp(arguments[0], "unmount") == 0) {
            unmount();
        } else if (strcmp(arguments[0], "touch") == 0) {
            char **files = (char**)malloc((numArguments-1)*sizeof(char*));
            for (int i = 1; i < numArguments; i++) {
                files[i-1] = arguments[i];
                // printf("%s\n", files[i-1]);
            }
            touch(files, numArguments - 1);
        } else if (strcmp(arguments[0], "mv") == 0) {
            mv(arguments[1], arguments[2]);
        } else if (strcmp(arguments[0], "rm") == 0) {
            for (int i = 1; i < numArguments; i++) {
                rm(arguments[i]);
            }
        } else if (strcmp(arguments[0], "cat") == 0) {
            int num_files = 0;
            for (int i = 1; i < numArguments; i++) {
                if (strcmp(arguments[i], "-w") == 0 || strcmp(arguments[i], "-a") == 0) {
                    break;
                }
                num_files += 1;
            }
            char* output_file = arguments[numArguments - 1];
            char** files = (char**)malloc(num_files*sizeof(char*));
            for (int i = 1; i < numArguments; i++) {
                if (strcmp(arguments[i], "-w") == 0 | strcmp(arguments[i], "-a") == 0) {
                    break;
                }
                files[i-1] = arguments[i];
            }
            // for (int i = 0; i < num_files; i++) {
            //     printf("file: %s\n", files[i]);
            // }
            if (strcmp(arguments[1], "-w") == 0) {
                // cat read from terminal and overwrite output
                // printf("%s\n", "boooo");
                while (1) {
                    char cat_cmd[maxLineLength];
                    for (int index = 0; index < maxLineLength; index++) {
                        cat_cmd[index] = '\0';
                    }
                    int cat_numBytes = read(STDIN_FILENO, cat_cmd, MAXLINELENGTH);
                    if (cat_cmd[cat_numBytes - 1] == '\n') { // Get rid of trailing newline.
                        cat_from_terminal(cat_cmd, arguments[2], true);
                        break;
                    }
                }
                
            } else if (strcmp(arguments[1], "-a") == 0) {
                // cat read from terminal and append to output
                // printf("%s\n", "success");
                while (1) {
                    char cat_cmd[maxLineLength];
                    for (int index = 0; index < maxLineLength; index++) {
                        cat_cmd[index] = '\0';
                    }
                    int cat_numBytes = read(STDIN_FILENO, cat_cmd, MAXLINELENGTH);
                    if (cat_cmd[cat_numBytes - 1] == '\n') { // Get rid of trailing newline.
                        cat_from_terminal(cat_cmd, arguments[2], false);
                        break;
                    }
                }
            } else if (strcmp(arguments[numArguments - 2], "-a") == 0) {
                // cat read from files and append to output
                cat(files, num_files, output_file, false);
                // printf("%s\n", "cat -w success");
            } else if (strcmp(arguments[numArguments - 2], "-w") == 0) {
                // cat read from files and overwrite to files
                cat(files, num_files, output_file, true);
                // printf("%s\n", "cat -w end");
            } else if (num_files >= 1) {
                // cat read to terminal
                // printf("%s\n", files[0]);
                cat(files, num_files, NULL, false);
                // printf("%s\n", "cat to terminal");
            } else {
                printf("%s\n", "ERROR: called cat incorrectly");
            }
        } else if (strcmp(arguments[0], "cp") == 0) {
            if (strcmp(arguments[1], "-h") == 0) {
                // source is from os
                cp(arguments[2], arguments[3], true, false);
            } else if (strcmp(arguments[2], "-h") == 0) {
                // dest is from os
                cp(arguments[1], arguments[3], false, true);
            } else if (numArguments == 3) {
                // both are from fs: three arguments: cp source dest
                cp(arguments[1], arguments[2], false, false);
            }
            else {
                printf("%s\n", "ERROR: called cp incorrectly");
            }
        } else if (strcmp(arguments[0], "ls") == 0) {
            ls();
        } else if (strcmp(arguments[0], "chmod") == 0) {
            // printf("%d\n", atoi(arguments[1]));
            int perm = atoi(arguments[1]);
            if (perm < 0 || perm > 7) {
                printf("%s\n", "ERROR: invalid perm for chmod");
            } else {
                chmod(arguments[2], perm);
            }
        } else {
            printf("%s\n", "ERROR: incorrect function call");
        }
    }
    return 0;
}