#ifndef PMM_H
#define PMM_H

#include "types.h"

#define PAGE_SIZE 4096U
#define PMM_MAX_MEMORY (64U * 1024U * 1024U)

void pmm_init(uint32_t memory_size);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t physical_address);

uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);

bool pmm_test(void);

#endif
