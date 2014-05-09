#include "task.h"

#include <stdlib.h>

int task_cmp_by_frequency(const void *a, const void *b) {
    task_t *t1 = (task_t *)a;
    task_t *t2 = (task_t *)b;

    return (t1->frequency - t2->frequency);
}

void task_sort_by_frequency(task_t *tasks, int size) {
    qsort(tasks, size, sizeof(task_t), task_cmp_by_frequency);

    for (i = 0; i < size; ++i) {
        tasks[i].id = i;
    }

    return;
}

void task_init_semaphore(task_t *tasks, int size) {
    int i;

    for (i = 0; i < size; ++i) {
        tasks[i].semaphore = (sem_t *)malloc(sizeof(sem_t));

        sem_init(tasks[i].semaphore, 0, 0);
    }

    return;
}

void task_init_priority(task_t *tasks, int size, int max_priority) {
    int i;

    for (i = 0; i < size; ++i) {
        tasks[i].priority = max_priority - i;
    }

    return;
}

void task_init_thread(task_t *tasks, int size, int policy, void *(*start_routine)(void *)) {
    int i;

    for (i = 0; i < size; ++i) {
        pthread_attr_t attr;
        struct sched_param param;

        param.sched_priority = tasks[i].priority;

        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, policy);
        pthread_attr_setschedparam(&attr, &param);

        tasks[i].thread = (pthread_t *)malloc(sizeof(pthread_t));

        pthread_create(tasks[i].thread, &attr, start_routine, (void *)&tasks[i]);
    }

    return;
}

void task_join_and_destroy(task_t *tasks, int size) {
    int i;

    for (i = 0; i < size; ++i) {
        pthread_join(tasks[i].thread, NULL);
    }

    for (i = 0; i < size; ++i) {
        free(tasks[i].thread);
        free(tasks[i].semaphore);
    }

    return;
}
