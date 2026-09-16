#ifndef RAMDISK_H
#define RAMDISK_H

#define RAMDISK_SIZE       (1024 * 1024)
#define RAMDISK_BLOCK_SIZE 4096
#define RAMDISK_BLOCK_COUNT (RAMDISK_SIZE / RAMDISK_BLOCK_SIZE)

void ramdisk_init(void);
void ramdisk_read_block(unsigned int block, void *buffer);
void ramdisk_write_block(unsigned int block, const void *buffer);

#endif
