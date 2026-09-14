#include "../include/process.h"
#include "vga.h"

/*
 * Process table
 */
pcb_t proc_table[MAX_PROCS];

/*
 * Currently running process.
 * Process 0 will be the idle process.
 */
int current_proc = 0;

/*
 * Process stacks.
 *
 * Each process gets a private 4 KB stack.
 */
static uint8_t process_stacks[MAX_PROCS][STACK_SIZE];

/*
 * Next PID.
 */
static uint32_t next_pid = 0;

/*
 * Process bootstrap.
 *
 * When a newly created process is first scheduled,
 * execution starts here.
 */
static void process_bootstrap(void)
{
    pcb_t *p = &proc_table[current_proc];

    /*
     * A newly created process does not return through
     * IRETD, so interrupts must be enabled here.
     */
    __asm__ __volatile__("sti");

    if (p->entry != NULL) {
        p->entry();
    }

    proc_exit();

    while (true) {
        __asm__ __volatile__("hlt");
    }
}

/*
 * Copy a string into the process name.
 */
static void process_copy_name(char *dst, const char *src)
{
    int i = 0;

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    while (src[i] != '\0' && i < 31) {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

/*
 * Create a new process.
 */
pcb_t *proc_create(const char *name, void (*entry)(void))
{
    for (int i = 0; i < MAX_PROCS; i++) {

        if (proc_table[i].state == PROC_UNUSED) {

            pcb_t *p = &proc_table[i];

            p->pid = next_pid++;
            p->state = PROC_READY;
            p->entry = entry;
            p->ticks = 0;

            process_copy_name(p->name, name);

            /*
             * Stack grows downward.
             */
            p->stack_base = (uint32_t)&process_stacks[i][0];

            uint32_t stack_top =
                p->stack_base + STACK_SIZE;

            /*
             * We create an initial stack layout
             * compatible with context_switch().
             *
             * context_switch() performs:
             *
             *     popad
             *     ret
             *
             * Therefore the stack must contain:
             *
             *     saved registers
             *     return address
             */

            stack_top -= 4;
            *((uint32_t *)stack_top) =
                (uint32_t)process_bootstrap;

            /*
             * Space for POPAD.
             *
             * POPAD restores 8 registers = 32 bytes.
             */
            stack_top -= 32;

            p->esp = stack_top;

            return p;
        }
    }

    return NULL;
}

/*
 * Terminate the current process.
 */
void proc_exit(void)
{
    proc_table[current_proc].state = PROC_ZOMBIE;

    /*
     * The scheduler will select another READY process
     * on the next timer tick.
     */
}
