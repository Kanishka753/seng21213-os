#include "../include/mutex.h"
#include "../include/thread.h"

void mutex_init(mutex_t *mutex)
{
    int i;

    mutex->locked = 0;
    mutex->wait_count = 0;

    for (i = 0; i < MUTEX_MAX_WAITERS; i++) {
        mutex->waiters[i] = -1;
    }
}

void mutex_lock(mutex_t *mutex)
{
    int tid;

    while (1) {
        __asm__ __volatile__("cli");

        if (mutex->locked == 0) {
            mutex->locked = 1;

            __asm__ __volatile__("sti");
            return;
        }

        tid = current_thread;

        if (mutex->wait_count < MUTEX_MAX_WAITERS) {
            mutex->waiters[mutex->wait_count] = tid;
            mutex->wait_count++;
        }

        __asm__ __volatile__("sti");

        thread_block();
    }
}

void mutex_unlock(mutex_t *mutex)
{
    int tid;
    int i;

    __asm__ __volatile__("cli");

    if (mutex->wait_count > 0) {
        tid = mutex->waiters[0];

        for (i = 0; i < mutex->wait_count - 1; i++) {
            mutex->waiters[i] = mutex->waiters[i + 1];
        }

        mutex->waiters[mutex->wait_count - 1] = -1;
        mutex->wait_count--;

        mutex->locked = 0;

        __asm__ __volatile__("sti");

        thread_unblock(tid);
        return;
    }

    mutex->locked = 0;

    __asm__ __volatile__("sti");
}
