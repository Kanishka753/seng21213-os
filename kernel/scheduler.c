#include "../include/scheduler.h"
#include "../include/process.h"

extern void context_switch(uint32_t *old_esp, uint32_t new_esp);

void scheduler_init(void)
{
    current_proc = 0;

    /*
     * Process 0 is the idle process.
     * It must already exist before scheduling starts.
     */
    if (proc_table[0].state != PROC_UNUSED) {
        proc_table[0].state = PROC_RUNNING;
    }
}

void scheduler_tick(void)
{
    proc_table[current_proc].ticks++;

    /*
     * Find the next READY process using round-robin order.
     */
    int next = (current_proc + 1) % MAX_PROCS;
    int searched = 0;

    while (searched < MAX_PROCS) {
        if (proc_table[next].state == PROC_READY) {
            break;
        }

        next = (next + 1) % MAX_PROCS;
        searched++;
    }

    /*
     * No READY process found.
     */
    if (searched == MAX_PROCS) {
        __asm__ __volatile__(
            "outb %0, %1"
            :
            : "a"((uint8_t)0x20),
              "Nd"((uint16_t)0x20)
        );
        return;
    }

    int old = current_proc;

    /*
     * Update process states.
     */
    proc_table[old].state = PROC_READY;
    proc_table[next].state = PROC_RUNNING;
    current_proc = next;

    /*
     * EOI MUST be sent before context switching.
     */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x20),
          "Nd"((uint16_t)0x20)
    );

    /*
     * Save the current process context and switch
     * to the next process.
     */
    context_switch(&proc_table[old].esp,
                   proc_table[next].esp);
}
