#ifndef THREAD_H
#define THREAD_H

#include "types.h"

#define MAX_THREADS 8
#define THREAD_STACK_SIZE 4096

typedef enum {
    THREAD_UNUSED = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_FINISHED
} thread_state_t;

typedef struct {
    int tid;
    thread_state_t state;

    void (*entry)(void *);
    void *arg;

    uint32_t esp;
    uint32_t stack_base;
} thread_t;

extern thread_t thread_table[MAX_THREADS];
extern int current_thread;

void thread_init(void);
int thread_create(void (*entry)(void *), void *arg);
void thread_yield(void);
void thread_exit(void);

#endif
