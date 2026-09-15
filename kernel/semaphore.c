#include "../include/semaphore.h"
#include "../include/thread.h"

void sem_init(semaphore_t *sem, int value)
{
    int i;

    sem->value = value;
    sem->wait_count = 0;

    for (i = 0; i < SEM_MAX_WAITERS; i++) {
        sem->waiters[i] = -1;
    }
}

void sem_wait(semaphore_t *sem)
{
    int tid;

    while (1) {
        __asm__ __volatile__("cli");

        if (sem->value > 0) {
            sem->value--;

            __asm__ __volatile__("sti");
            return;
        }

        tid = current_thread;

        if (sem->wait_count < SEM_MAX_WAITERS) {
            sem->waiters[sem->wait_count] = tid;
            sem->wait_count++;
        }

        __asm__ __volatile__("sti");

        thread_block();
    }
}

void sem_signal(semaphore_t *sem)
{
    int tid;
    int i;

    __asm__ __volatile__("cli");

    sem->value++;

    if (sem->wait_count > 0) {
        tid = sem->waiters[0];

        for (i = 0; i < sem->wait_count - 1; i++) {
            sem->waiters[i] = sem->waiters[i + 1];
        }

        sem->waiters[sem->wait_count - 1] = -1;
        sem->wait_count--;

        __asm__ __volatile__("sti");

        thread_unblock(tid);
        return;
    }

    __asm__ __volatile__("sti");
}
