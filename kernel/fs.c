#include "fs.h"
#include "ramdisk.h"
#include "vga.h"

typedef struct
{
    char name[FS_NAME_SIZE];
    unsigned int size;
    unsigned int used;
    unsigned char data[FS_MAX_FILE_SIZE];
} fs_file_t;

static fs_file_t files[FS_MAX_FILES];

static int string_equal(const char *a, const char *b)
{
    unsigned int i = 0;

    while (a[i] != '\0' && b[i] != '\0')
    {
        if (a[i] != b[i])
        {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

static void string_copy(char *destination, const char *source)
{
    unsigned int i = 0;

    while (i < FS_NAME_SIZE - 1 && source[i] != '\0')
    {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}

void fs_init(void)
{
    unsigned int i;

    ramdisk_init();

    for (i = 0; i < FS_MAX_FILES; i++)
    {
        files[i].used = 0;
        files[i].size = 0;
        files[i].name[0] = '\0';
    }
}

int fs_open(const char *name)
{
    unsigned int i;

    for (i = 0; i < FS_MAX_FILES; i++)
    {
        if (files[i].used && string_equal(files[i].name, name))
        {
            return (int)i;
        }
    }

    for (i = 0; i < FS_MAX_FILES; i++)
    {
        if (!files[i].used)
        {
            files[i].used = 1;
            files[i].size = 0;
            string_copy(files[i].name, name);

            return (int)i;
        }
    }

    return -1;
}

int fs_read(int fd, void *buffer, unsigned int size)
{
    unsigned int i;
    unsigned char *destination;

    if (fd < 0 || fd >= FS_MAX_FILES || !files[fd].used || buffer == 0)
    {
        return -1;
    }

    if (size > files[fd].size)
    {
        size = files[fd].size;
    }

    destination = (unsigned char *)buffer;

    for (i = 0; i < size; i++)
    {
        destination[i] = files[fd].data[i];
    }

    return (int)size;
}

int fs_write(int fd, const void *buffer, unsigned int size)
{
    unsigned int i;
    const unsigned char *source;

    if (fd < 0 || fd >= FS_MAX_FILES || !files[fd].used || buffer == 0)
    {
        return -1;
    }

    if (size > FS_MAX_FILE_SIZE)
    {
        size = FS_MAX_FILE_SIZE;
    }

    source = (const unsigned char *)buffer;

    for (i = 0; i < size; i++)
    {
        files[fd].data[i] = source[i];
    }

    files[fd].size = size;

    return (int)size;
}

int fs_close(int fd)
{
    if (fd < 0 || fd >= FS_MAX_FILES || !files[fd].used)
    {
        return -1;
    }

    return 0;
}

int fs_unlink(const char *name)
{
    unsigned int i;

    for (i = 0; i < FS_MAX_FILES; i++)
    {
        if (files[i].used && string_equal(files[i].name, name))
        {
            files[i].used = 0;
            files[i].size = 0;
            files[i].name[0] = '\0';

            return 0;
        }
    }

    return -1;
}

void fs_list(void)
{
    unsigned int i;
    int found = 0;

    vga_puts("\nFiles:\n");
    vga_puts("------------------------------\n");

    for (i = 0; i < FS_MAX_FILES; i++)
    {
        if (files[i].used)
        {
            vga_puts(files[i].name);
            vga_puts("\n");
            found = 1;
        }
    }

    if (!found)
    {
        vga_puts("(empty)\n");
    }

    vga_puts("\n");
}
