#ifndef __TASK_H
#define __TASK_H

#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_TASK_NAME   128
#define MAX_TASK        1024

typedef struct {
    int         id;
    int         frequency, duration;
    int         priority;
    char        name[MAX_TASK_NAME];
    pthread_t   *thread;
    sem_t       *semaphore;

} task_t;

int task_cmp_by_frequency(const void *, const void *);

void task_sort_by_frequency(task_t *, int);

void task_init_semaphore(task_t *, int);

void task_init_priority(task_t *, int, int);

void task_init_thread(task_t *, int, int, void *(*start_routine)(void *));

void task_join_and_destroy(task_t *, int);

#endif
