#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include "filesystem.h"
#include "filefunc.h"
#include "utils.h"
#include "errno.h"
#define MAXLINELENGTH 4096
#define F_SEEK_SET 0
#define F_SEEK_CUR 1
#define F_SEEK_END 2

int NUM_FAT_ENTRIES;
int fd_ptr = 2;
const uint16_t f_eof = 0xFFFF;

// array of file descriptors
struct fd_entry **files; // 0 = STDIN, 1 = STDOUT
fat_context *f_fc;

/**

NAME fileFuncConstructor
DESCRIPTION constructor
FOUND IN filefunc.c
RETURNS void
ERRORS
*/
void fileFuncConstructor(char *fatfs)
{
    // call mkfs to make the binary file
    char buf[2];
    int fd = open(fatfs, O_RDWR | O_CREAT);
    read(fd, buf, 2);
    int lsb = (int)char_to_8(buf[1]); // size of block: 0: 256, 1: 512, 2: 1024, 3: 2048, 4: 4096
    int msb = (int)char_to_8(buf[0]); // fat size
    printf("%d\n", lsb);
    printf("%d\n", msb);
    if (lsb < 0 || lsb > 4)
    {
        printf("%s\n", "ERROR: invalid block size");
    }
    f_fc = constructor(lsb, msb, fatfs);
    
    NUM_FAT_ENTRIES = f_fc->NUM_FAT;
    files = malloc(sizeof(struct fd_entry*)*NUM_FAT_ENTRIES);
    printf("%s\n", "created");
}

/**

NAME find_file
DESCRIPTION finds a file fd based on filename
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int find_file(char *filename)
{
    for (int i = 0; i < 32; i++)
    {
        fd_entry *curr_file = files[i];
        if (curr_file == NULL)
        {
            continue;
        }
        if (strcmp(curr_file->name, filename) == 0)
        {
            return i;
        }
    }
    return -1;
}

/**

NAME find_next_available_fd
DESCRIPTION finds the next available fd to use
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int find_next_available_fd() {
    for (int i = 2; i < NUM_FAT_ENTRIES; i ++) {
        if (files[i] == NULL) {
            return i;
        }
    }
    errno = EFTF;
    return -1;
}
/**

NAME f_open
DESCRIPTION open a file name fname with the mode mode and returns a file descriptor on success and a negative value on error
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_open(const char *fname, int mode)
{
    // printf("hello this is fopen\n");
    block* curr_file = find_dir_entry((char *)fname, f_fc);
    int fd = find_file((char *)fname);
    // printf("mode: %d\n", mode);

    if (curr_file != NULL) {
        // file exists in the fat
        if (fd >= 0) {
            // file exists in fd table
            if (mode) { // mode == true if overwrite
                // printf("%s\n", "overwrite");
                clear_file(curr_file->name, f_fc);
            } 
            else {
                if (curr_file->size != 0) {
                    f_write(fd, "\n", strlen("\n"));
                }
            }
            return fd;
        }
    }
    int fat_block_index;
    if (curr_file == NULL) {
        // file doesn't exist in the filesystem at all
        fat_block_index = create((char *)fname, mode, f_fc);
    } else {
        fat_block_index = curr_file->firstBlock;
    }


    
    fd = find_next_available_fd();
    fd_entry *newfile = (struct fd_entry *)malloc(sizeof(struct fd_entry));
    strcpy(newfile->name, fname);
    newfile->fat_pointer = fat_block_index;
    newfile->curr_data_ptr = 0;
    files[fd] = newfile;
    fd_ptr = MAX(fd, fd_ptr);
    return fd;
}
/**

NAME f_read
DESCRIPTION read n bytes from the file referenced by fd. On return, f_read returns the number of bytes read, 0 if EOF is reached, or a negative number on error.
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_read(int fd, int n, char *buf)
{
    if (files[fd] == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    fd_entry *file = files[fd];

    int blocks_to_read = n / (f_fc->BLOCK_SIZE) + (n % f_fc->BLOCK_SIZE != 0);
    int curr_fat_block = file->fat_pointer;
    int buf_ptr = 0;
    int rem = n;

    // printf("n: %d\n", n);
    // printf("blocks to read: %d\n", blocks_to_read);
    // printf("curr_fat_block: %d\n", curr_fat_block);

    for (int i = 0; i < blocks_to_read; i++)
    {
        if (curr_fat_block == (int)0xFFFF)
        {
            errno = EEOF;
            return -1;
        }
        char *block = fatreadblock(curr_fat_block, f_fc);
        // printf("block: %s\n", block);
        int to_fill = MIN(f_fc->BLOCK_SIZE, rem);
        // printf("to fill: %d\n", to_fill);

        for (int j = 0; j < to_fill; j++)
        {
            buf[j] = block[j + buf_ptr];
        }
        buf_ptr += f_fc->BLOCK_SIZE;
        rem -= to_fill;
        curr_fat_block = (int)read_fat_block(curr_fat_block, f_fc);
    }
    buf[n] = '\0';
    return fd;
}
/**

NAME f_write
DESCRIPTION write n bytes of the string referenced by str to the file fd and increment the file pointer by n. On return, f_write returns the number of bytes written, or a negative value on error.
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_write(int fd, const char *str, int n)
{
    char toWrite[n];
    // printf("char to write: %d\n", n);
    if (files[fd] == NULL)
    {
        errno = ENOENT;
        return -1;
    }
    for (int index = 0; index < n; index++)
    {
        toWrite[index] = '\0';
    }
    for (int i = 0; i < n; i++)
    {
        toWrite[i] = str[i];
    }
    // printf("%s\n", toWrite);
    fatwrite(files[fd]->name, toWrite, n, f_fc);
    files[fd]->curr_data_ptr = files[fd]->curr_data_ptr + n;
    return n;
}
/**

NAME f_close
DESCRIPTION close the file fd and return 0 on success, or a negative value on failure.
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_close(int fd)
{
    if (files[fd] != NULL && fd >= 2) {
        f_rm(files[fd]->name);
    } else {
        errno = ENOENT;
        return -1;
    }
    return 0;
}
/**

NAME f_unlink
DESCRIPTION remove the file and throw -1 on error
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_unlink(const char *fname)
{
    int fd = find_file((char *)fname);
    if (fd < 2) {
        errno = ENOENT;
        return -1;
    }
    files[fd] = NULL;
    fatremove((char *)fname, f_fc);
    return 0;
}
/**

NAME f_lseek
DESCRIPTION reposition the file pointer for fd to the offset relative to whence.
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_lseek(int fd, int offset, int whence)
{
    if (files[fd] == NULL)
    {
        errno = ENOENT;
        return -1;
    }
    if (whence == F_SEEK_SET)
    {
        files[fd]->fat_pointer = offset;
    }
    else if (whence == F_SEEK_CUR)
    {
        files[fd]->fat_pointer = files[fd]->fat_pointer + offset;
    }
    else if (whence == F_SEEK_END)
    {
        files[fd]->fat_pointer = NUM_FAT_ENTRIES + offset;
    }
    else
    {
        errno = EUDWV;
    }
    return fd;
}
/**

NAME f_ls
DESCRIPTION list the file filename in the current directory. If filename is NULL, list all files in the current directory.
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_ls(const char *filename, int fd)
{
    if (filename != NULL)
    {
        printf("%s\n", filename);
    }
    else
    {
        int curr_block = 1;
        uint16_t link;
        do {
            for (int i = 0; i < f_fc->DIRS_PER_BLOCK; i++) {
                block* dir = read_dir_entry(curr_block, i, f_fc);
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
                        errno = EINVPERM;
                        return -1;
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
                        errno = EINVPERM;
                        return -1;
                    }
                    printf("%d ", dir->size); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                    printf("%s ", monthChar); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                    printf("%02d ", day); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                    printf("%02d:%02d ", hours, minutes); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                    printf("%s\n", dir->name); // PRINTING TO THE TERMINAL DO NOT COMMENT OUT
                }
            }
            link = read_fat_block(curr_block, f_fc);
            curr_block = (int)link;
        } while (link != f_eof);
    }
    return fd;
}
/**

NAME f_cat
DESCRIPTION Concatenates files together with input files as an input
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_cat(char **input_files, int num_files, int fd)
{
    // printf("fd output: %d\n", fd);
    block *output_block;
    fd_entry *to_write = files[fd]; //fd entry to write to, null if printing to terminal
    if (fd > 1)
    {
        // we want to write to a file that exists
        // file descriptor lookup
        output_block = find_dir_entry(to_write->name, f_fc); // find the dir entry for the output file, if it exists.
    }
    if (to_write != NULL && output_block == NULL)
    { // only create if we need output
        // create the file bc we want to write to file, but file doesn't exist
        create(to_write->name, 6, f_fc);
    }
    for (int i = 0; i < num_files; i++)
    {
        char *file = input_files[i];
        // printf("file: %s\n", file);
        block *dir_block = find_dir_entry(file, f_fc);
        if (dir_block == NULL) {
            errno = ENOENT;
            return -1;
        }
        // printf("file found: %s\n", dir_block->name);
        // read the entire file. If output, we write to output.
        int link = (int)dir_block->firstBlock;
        do
        {
            char *filePart = fatreadblock(link, f_fc);
            if (to_write == NULL)
            {
                printf("%s", filePart); // printing to the terminal DO NOT COMMENT OUT
            }
            else
            {   
                // fatwrite(to_write->name, filePart, f_fc);
                f_write(fd, filePart, strlen(filePart));
            }
            link = (int)read_fat_block(link, f_fc);
        } while (link != f_eof);
        if (to_write == NULL)
        {
            printf("\n"); // printing to terminal DO NOT COMMENT OUT
        }
    }
    return 0;
}
/**

NAME f_touch
DESCRIPTION Creates the files if they do not exist
FOUND IN filefunc.c
RETURNS void
ERRORS
*/
void f_touch(char **input_files, int num_files)
{
    for (int i = 0; i < num_files; i++)
    {
        char *file = input_files[i];
        // printf("%s\n", file);
        f_open(file, true);
    }
}
/**

NAME f_mv
DESCRIPTION Renames SOURCE to DEST
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_mv(char *source, char *dest)
{
    // grab the current file
    // modify the file name to dest
    // printf("hello\n");
    block *source_dir = find_dir_entry(source, f_fc);
    // printf("name: %s\n", source_dir->name);
    if (source_dir == NULL) {
        errno = ENOENT;
        return -1;
    }
    // update source_dir name
    strcpy(source_dir->name, dest);
    int fd = find_file(source);
    strcpy(files[fd]->name, dest);
    update_dir_entry(source, source_dir, f_fc);
    return 0;
}
/**

NAME f_rm
DESCRIPTION Removes the files
FOUND IN filefunc.c
RETURNS void
ERRORS
*/
void f_rm(char *file)
{
    // int fd = find_file(file);
    // if (fd < 2) {
    //     printf("ERROR: file doesn't exist");
    //     return;
    // }
    // files[fd] = NULL;
    // fatremove(file, f_fc);
    f_unlink(file);
}
/**

NAME f_cp
DESCRIPTION Copies SOURCE to DEST
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_cp(char *source, char *dest)
{
    // create files if they don't exist:
    // printf("hi0\n");
    block *src = find_dir_entry(source, f_fc);
    block *dt = find_dir_entry(dest, f_fc);
    // printf("hi1\n");
    if (src == NULL)
    {
        errno = ENOENT;
        return -1;
    }
    if (dt == NULL)
    {
        create(dest, 7, f_fc);
    }
    // read source and dest from filesystem
    block *src_dir = find_dir_entry(source, f_fc);
    // fatread from src_dir and fatwrite to dest_dir
    int link = (int)src_dir->firstBlock;
    do
    {
        char *filePart = fatreadblock(link, f_fc);
        int fd = find_file(dest);
        f_write(fd, filePart, strlen(filePart));
        link = read_fat_block(link, f_fc);
    } while (link != f_eof);
    return 0;
}
/**

NAME f_chmod
DESCRIPTION Change the permissions based on input
FOUND IN filefunc.c
RETURNS int
ERRORS -1
*/
int f_chmod(char *filename, bool op, char perm) //op = true if plus
{
    block *dir_block = find_dir_entry(filename, f_fc);
    if (dir_block == NULL) {
        errno = ENOENT;
        return -1;
    }
    if (op) {
        if (perm == 'r') {
            dir_block->perm = 4 | dir_block->perm;
        } else if (perm == 'w') {
            dir_block->perm = 2 | dir_block->perm;
        } else if (perm == 'x') {
            dir_block->perm = 1 | dir_block->perm;
        } else {
                errno = EINVPERM;
                return -1;
        }
    } else {
        if (perm == 'r') {
            dir_block->perm = ~4 & dir_block->perm;
        } else if (perm == 'w') {
            dir_block->perm = ~2 & dir_block->perm;
        } else if (perm == 'x') {
            dir_block->perm = ~1 & dir_block->perm;
        } else {
                errno = EINVPERM;
                return -1;
        }
    }
    update_dir_entry(dir_block->name, dir_block, f_fc);
    return 0;
}

int filefunc(int argc, char **argv)
{

    fileFuncConstructor("minfs");
    int fd = f_open("hi.txt", 5);
    // printf("find file: %d\n", find_file("hi.txt"));
    int n = strlen("Hi this is Annie");
    printf("%d\n", fd);
    f_write(fd, "Hi this is Annie", n);
    char buf[n + 1];
    f_read(fd, n, buf);
    printf("buf: %s\n", buf);

    return 0;
}
/**

NAME f_findscript
DESCRIPTION Runs a script with commands located in a file
FOUND IN filefunc.c
RETURNS char**
ERRORS
*/
char** f_findscript(char* fname) {
    int fd = find_file(fname);
    // printf("fd: %d\n", fd);
    if (fd < 0) {
        errno = ENOENT;
        return NULL;
    }
    block *dir_block = find_dir_entry(fname, f_fc);
    int size = dir_block->size;
    if (size == 0) {
        printf("SCRIPT EMPTY\n");
        return NULL;
    }
    if (dir_block->perm != 7 && dir_block->perm != 1) {
        printf("ERROR: INVALID PERMISSIONS\n");
        return NULL;
    }
    char* buf = malloc(size);
    f_read(fd, size, buf);
    // printf("buf: %s\n", buf);
    
    
    int line_count = 1;
    for (int i = 0; i < size; i++) {
        if (buf[i] == '\n'){
            line_count ++;
        }
    }
    // printf("line count: %d\n", line_count);
    const char s[2] = "\n";
    char *token;
    token = strtok(buf, s);
    char **commands = (char **)malloc(sizeof(char *) * (line_count + 1));
    
    int index = 0;
    while (token != NULL) {
        commands[index] = token;
        token = strtok(NULL, s);
        index ++;
    }
    // for (int j = 0; j < line_count; j ++) {
    //     printf("%s\n", commands[j]);
    // }
    commands[line_count] = NULL;
    return commands;
}