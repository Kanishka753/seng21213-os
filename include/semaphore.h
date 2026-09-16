#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "types.h"

#define SEM_MAX_WAITERS 8

typedef struct {
    volatile int value;
    int waiters[SEM_MAX_WAITERS];
    int wait_count;
} semaphore_t;

void sem_init(semaphore_t *sem, int value);
void sem_wait(semaphore_t *sem);
void sem_signal(semaphore_t *sem);

#endif
