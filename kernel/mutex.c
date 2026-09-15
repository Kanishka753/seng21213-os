#include "../include/mutex.h"

void mutex_init(mutex_t *mutex)
{
    mutex->locked = 0;
}

void mutex_lock(mutex_t *mutex)
{
    while (1) {
        __asm__ __volatile__("cli");

        if (mutex->locked == 0) {
            mutex->locked = 1;
            __asm__ __volatile__("sti");
            return;
        }

        __asm__ __volatile__("sti");
    }
}

void mutex_unlock(mutex_t *mutex)
{
    __asm__ __volatile__("cli");
    mutex->locked = 0;
    __asm__ __volatile__("sti");
}
