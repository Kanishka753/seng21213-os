#ifndef PMM_H
#define PMM_H

#include "types.h"

#define PAGE_SIZE 4096U
#define PMM_MAX_MEMORY (64U * 1024U * 1024U)

/*
 * BIOS E820 memory map entry.
 */
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;

/*
 * E820 map location provided by the bootloader.
 */
#define E820_MAP_ADDRESS 0x8000U
#define E820_MAX_ENTRIES 32U

void pmm_init(uint32_t memory_size);
void pmm_init_from_e820(const e820_entry_t *entries, uint32_t entry_count);

uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t physical_address);

uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);

bool pmm_test(void);

#endif
