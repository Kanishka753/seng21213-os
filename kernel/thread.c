#include "../include/thread.h"
#include "../include/process.h"
#include "vga.h"

thread_t thread_table[MAX_THREADS];
int current_thread = 0;

static uint8_t thread_stacks[MAX_THREADS][THREAD_STACK_SIZE];
static int next_tid = 0;

static void thread_bootstrap(void)
{
    thread_t *t = &thread_table[current_thread];

    __asm__ __volatile__("sti");

    if (t->entry != NULL) {
        t->entry(t->arg);
    }

    thread_exit();

    while (1) {
        __asm__ __volatile__("hlt");
    }
}

void thread_init(void)
{
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = -1;
        thread_table[i].state = THREAD_UNUSED;
        thread_table[i].entry = NULL;
        thread_table[i].arg = NULL;
        thread_table[i].esp = 0;
        thread_table[i].stack_base = 0;
    }

    current_thread = 0;
    next_tid = 0;
}

int thread_create(void (*entry)(void *), void *arg)
{
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_UNUSED) {
            thread_t *t = &thread_table[i];

            t->tid = next_tid++;
            t->state = THREAD_READY;
            t->entry = entry;
            t->arg = arg;

            t->stack_base = (uint32_t)&thread_stacks[i][0];

            uint32_t stack_top =
                t->stack_base + THREAD_STACK_SIZE;

            stack_top -= 4;
            *((uint32_t *)stack_top) =
                (uint32_t)thread_bootstrap;

            stack_top -= 32;

            t->esp = stack_top;

            return t->tid;
        }
    }

    return -1;
}

void thread_yield(void)
{
    /*
     * Initial implementation.
     * Thread scheduling will be connected to the
     * existing scheduler in the next step.
     */
}

void thread_exit(void)
{
    thread_table[current_thread].state = THREAD_FINISHED;
}
