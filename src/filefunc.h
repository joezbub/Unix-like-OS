typedef struct fd_entry fd_entry;

void fileFuncConstructor(char *fatfs);

struct fd_entry
{
    char name[32];
    int fat_pointer;   // pointing to the start block of the file
    int curr_data_ptr; // in bytes
};

int f_open(const char *fname, int mode);

int f_read(int fd, int n, char *buf);

int f_write(int fd, const char *str, int n);

int f_close(int fd);

int f_unlink(const char *fname);

int f_lseek(int fd, int offset, int whence);

int f_ls(const char *filename, int fd);

int f_cat(char **input_files, int num_files, int fd);

void f_touch(char **input_files, int num_files);

int f_mv(char *source, char *dest);

void f_rm(char *file);
 
int f_cp(char *source, char *dest);

int f_chmod(char *filename, bool op, char perm);

char** f_findscript(char* fname);