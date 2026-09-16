#ifndef FS_H
#define FS_H

#define FS_MAX_FILES 32
#define FS_NAME_SIZE 28
#define FS_MAX_FILE_SIZE (8 * 4096)

void fs_init(void);

int fs_open(const char *name);
int fs_read(int fd, void *buffer, unsigned int size);
int fs_write(int fd, const void *buffer, unsigned int size);
int fs_close(int fd);
int fs_unlink(const char *name);

void fs_list(void);

#endif
