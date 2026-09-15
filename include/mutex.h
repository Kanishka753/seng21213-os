#ifndef MUTEX_H
#define MUTEX_H

#include "types.h"

typedef struct {
    volatile int locked;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif
