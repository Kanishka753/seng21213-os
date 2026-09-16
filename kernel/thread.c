#include "../include/thread.h"
#include "../include/process.h"
#include "vga.h"

extern void context_switch(uint32_t *old_esp, uint32_t new_esp);

thread_t thread_table[MAX_THREADS];
int current_thread = 0;

static uint8_t thread_stacks[MAX_THREADS][THREAD_STACK_SIZE];
static int next_tid = 0;

static void thread_bootstrap(void)
{
    thread_t *t = &thread_table[current_thread];

    t->state = THREAD_RUNNING;

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

    /*
     * Reserve thread 0 for the current kernel/shell context.
     *
     * The kernel is already running before worker threads are created.
     * Therefore, thread 0 does not need a new stack.
     */
    thread_table[0].tid = 0;
    thread_table[0].state = THREAD_RUNNING;
    thread_table[0].entry = NULL;
    thread_table[0].arg = NULL;

    current_thread = 0;
    next_tid = 1;
}

int thread_create(void (*entry)(void *), void *arg)
{
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_UNUSED) {
            thread_t *t = &thread_table[i];

            t->tid = next_tid++;
            t->state = THREAD_READY;
            t->entry = entry;
            t->arg  = arg;

            t->stack_base = (uint32_t)&thread_stacks[i][0];

            uint32_t stack_top =
                t->stack_base + THREAD_STACK_SIZE;

            /*
 	     * Prepare initial stack for context_switch().
 	     *
 	     * context_switch() executes:
 	     *     popad
             *     ret
 	     *
 	     * Therefore the stack must contain:
    	     *     32 bytes for popad
   	     *     thread_bootstrap address for ret
 	     */
	    stack_top -= 4;
	    *((uint32_t *)stack_top) =
    	        (uint32_t)thread_bootstrap;

            stack_top -= 32;

	    /*
 	     * Clear the saved registers.
 	     */
	    for (int j = 0; j < 8; j++) {
    	        ((uint32_t *)stack_top)[j] = 0;
            }

	    t->esp = stack_top;

            return t->tid;
        }
    }

    return -1;
}

void thread_yield(void)
{
    int old_thread = current_thread;
    int next_thread = -1;

    /*
     * Find the next READY thread using round-robin order.
     */
    for (int i = 1; i <= MAX_THREADS; i++) {
        int index = (old_thread + i) % MAX_THREADS;

        if (thread_table[index].state == THREAD_READY) {
            next_thread = index;
            break;
        }
    }

    /*
     * No other READY thread found.
     */
    if (next_thread == -1) {
        thread_table[old_thread].state = THREAD_RUNNING;
        return;
    }

    thread_table[old_thread].state = THREAD_READY;
    thread_table[next_thread].state = THREAD_RUNNING;

    current_thread = next_thread;

    context_switch(
        &thread_table[old_thread].esp,
        thread_table[next_thread].esp
    );
}

void thread_exit(void)
{
    thread_table[current_thread].state = THREAD_FINISHED;
}
void thread_block(void)
{
    int tid = current_thread;

    thread_table[tid].state = THREAD_BLOCKED;

    thread_yield();
}

void thread_unblock(int tid)
{
    if (tid < 0 || tid >= MAX_THREADS) {
        return;
    }

    if (thread_table[tid].state == THREAD_BLOCKED) {
        thread_table[tid].state = THREAD_READY;
    }
}
