#include "ramdisk.h"

static unsigned char ramdisk[RAMDISK_SIZE];

void ramdisk_init(void)
{
    unsigned int i;

    for (i = 0; i < RAMDISK_SIZE; i++)
    {
        ramdisk[i] = 0;
    }
}

void ramdisk_read_block(unsigned int block, void *buffer)
{
    unsigned int i;
    unsigned char *destination;
    unsigned int offset;

    if (block >= RAMDISK_BLOCK_COUNT || buffer == 0)
    {
        return;
    }

    destination = (unsigned char *)buffer;
    offset = block * RAMDISK_BLOCK_SIZE;

    for (i = 0; i < RAMDISK_BLOCK_SIZE; i++)
    {
        destination[i] = ramdisk[offset + i];
    }
}

void ramdisk_write_block(unsigned int block, const void *buffer)
{
    unsigned int i;
    const unsigned char *source;
    unsigned int offset;

    if (block >= RAMDISK_BLOCK_COUNT || buffer == 0)
    {
        return;
    }

    source = (const unsigned char *)buffer;
    offset = block * RAMDISK_BLOCK_SIZE;

    for (i = 0; i < RAMDISK_BLOCK_SIZE; i++)
    {
        ramdisk[offset + i] = source[i];
    }
}
