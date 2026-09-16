#include "../include/pmm.h"

#define BITMAP_SIZE (PMM_MAX_MEMORY / PAGE_SIZE / 8U)

static uint8_t frame_bitmap[BITMAP_SIZE];
static uint32_t total_frames_count;
static uint32_t used_frames_count;

static void set_frame(uint32_t frame)
{
    frame_bitmap[frame / 8U] |=
        (uint8_t)(1U << (frame % 8U));
}

static void clear_frame(uint32_t frame)
{
    frame_bitmap[frame / 8U] &=
        (uint8_t)~(1U << (frame % 8U));
}

static bool is_frame_used(uint32_t frame)
{
    return (frame_bitmap[frame / 8U] &
            (uint8_t)(1U << (frame % 8U))) != 0;
}

void pmm_init(uint32_t memory_size)
{
    uint32_t frame;
    uint32_t reserved_frames;

    if (memory_size > PMM_MAX_MEMORY) {
        memory_size = PMM_MAX_MEMORY;
    }

    total_frames_count = memory_size / PAGE_SIZE;
    used_frames_count = 0;

    for (frame = 0; frame < BITMAP_SIZE * 8U; frame++) {
        frame_bitmap[frame / 8U] = 0;
    }

    reserved_frames = (1024U * 1024U) / PAGE_SIZE;

    for (frame = 0; frame < reserved_frames; frame++) {
        set_frame(frame);
        used_frames_count++;
    }
}

uint32_t pmm_alloc_frame(void)
{
    uint32_t frame;

    for (frame = 0; frame < total_frames_count; frame++) {
        if (!is_frame_used(frame)) {
            set_frame(frame);
            used_frames_count++;
            return frame * PAGE_SIZE;
        }
    }

    return 0;
}

void pmm_free_frame(uint32_t physical_address)
{
    uint32_t frame;

    if (physical_address == 0 ||
        (physical_address % PAGE_SIZE) != 0) {
        return;
    }

    frame = physical_address / PAGE_SIZE;

    if (frame >= total_frames_count) {
        return;
    }

    if (is_frame_used(frame)) {
        clear_frame(frame);
        used_frames_count--;
    }
}

uint32_t pmm_total_frames(void)
{
    return total_frames_count;
}

uint32_t pmm_used_frames(void)
{
    return used_frames_count;
}

uint32_t pmm_free_frames(void)
{
    return total_frames_count - used_frames_count;
}

bool pmm_test(void)
{
    uint32_t frames[100];
    uint32_t i;

    for (i = 0; i < 100; i++) {
        frames[i] = pmm_alloc_frame();

        if (frames[i] == 0) {
            return false;
        }
    }

    for (i = 0; i < 100; i++) {
        pmm_free_frame(frames[i]);
    }

    return true;
}
