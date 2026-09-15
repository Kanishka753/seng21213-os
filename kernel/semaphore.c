#include "../include/semaphore.h"

void sem_init(semaphore_t *sem, int value)
{
    sem->value = value;
}

void sem_wait(semaphore_t *sem)
{
    while (1) {
        __asm__ __volatile__("cli");

        if (sem->value > 0) {
            sem->value--;
            __asm__ __volatile__("sti");
            return;
        }

        __asm__ __volatile__("sti");
    }
}

void sem_signal(semaphore_t *sem)
{
    __asm__ __volatile__("cli");
    sem->value++;
    __asm__ __volatile__("sti");
}
