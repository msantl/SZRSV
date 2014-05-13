#include <stdio.h>

#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <err.h>

#include <sys/types.h>

#include "parser.h"
#include "task.h"

#define POLICY  SCHED_FIFO

/* global variables */
int g_aktivna_dretva, g_running;

void print_usage(void) {
    puts("+------------------------------------------+");
    puts("| Third lab assignment - Real Time Systems |");
    puts("|------------------------------------------|");
    puts("| Usage:                                   |");
    puts("| ./sched <task_config>                    |");
    puts("+------------------------------------------+");

    errx(1, "Wrong usage!");
    return;
}

void init_process(void) {
    struct sched_param param;

    param.sched_priority =
        (sched_get_priority_min(POLICY) +
         sched_get_priority_max(POLICY)) >> 1;

    sched_setscheduler(getpid(), POLICY, &param);
    return;
}

void* thread_work(void *arg) {
    task_t *task = (task_t *)arg;
    struct timespec t_start, t_end;
    clockid_t clock_id;

    pthread_getcpuclockid(*task->thread, &clock_id);

    while (g_running) {

        /* wait for your turn*/
        sem_wait(task->semaphore);

        printf("U K.O. ulazi %s\n", task->name);

        for (clock_gettime(clock_id, &t_start);;) {
            clock_gettime(clock_id, &t_end);

            if ((t_end.tv_sec - t_start.tv_sec == task->duration &&
                t_end.tv_nsec >= t_start.tv_nsec) ||
                t_end.tv_sec - t_start.tv_sec > task->duration
            ) {
                break;
            }

            /* radno cekanje */
            g_aktivna_dretva = task->id;
            __asm__ __volatile__ ("" ::: "memory");
        }

        g_aktivna_dretva = -1;

        printf("Iz K.O. izlazi %s\n", task->name);
    }

    return NULL;
}

void thread_release(union sigval e) {
    sem_t *semaphore = (sem_t *)e.sival_ptr;

    sem_post(semaphore);

    return;
}

void thread_print_info(union sigval e) {
    printf("\tAktivna dretva: %d\n", g_aktivna_dretva);
    return;
}

void init_timers(timer_t *timers, task_t *tasks, int size) {
    int i;

    struct itimerspec spec;
    struct sigevent tick;

    for (i = 0; i < size; ++i) {
        /* activate each task */
        tick.sigev_notify               = SIGEV_THREAD;
        tick.sigev_signo                = 0;
        tick.sigev_value.sival_ptr      = tasks[i].semaphore;
        tick.sigev_notify_function      = thread_release;
        tick.sigev_notify_attributes    = NULL;

        spec.it_value.tv_sec        = 1;
        spec.it_value.tv_nsec       = 0;
        spec.it_interval.tv_sec     = tasks[i].frequency;
        spec.it_interval.tv_nsec    = 0;

        timer_create(CLOCK_REALTIME, &tick, &timers[i]);
        timer_settime(timers[i], 0, &spec, NULL);
    }

    /* print info each 0.5 second */
    tick.sigev_notify               = SIGEV_THREAD;
    tick.sigev_signo                = 0;
    tick.sigev_value.sival_int      = 0;
    tick.sigev_notify_function      = thread_print_info;
    tick.sigev_notify_attributes    = NULL;

    spec.it_value.tv_sec        = 1;
    spec.it_value.tv_nsec       = 0;
    spec.it_interval.tv_sec     = 0;
    spec.it_interval.tv_nsec    = 500000000;

    timer_create(CLOCK_REALTIME, &tick, &timers[size]);
    timer_settime(timers[size], 0, &spec, NULL);

    return;
}

void end_scheduler(int sig) {
    g_running = 0;
    printf("Last call for processor!\n");
    return;
}

int main(int argc, char **argv) {
    int n_tasks;
    task_t tasks[MAX_TASK];
    timer_t timers[MAX_TASK + 1];

    if (argc != 2) {
        print_usage();
    } else {
        g_running = 1;
    }

    /* listen to C-c */
    sigset(SIGINT, end_scheduler);

    /* prepare this process */
    init_process();

    /* load task information */
    parse_input_file(argv[1], tasks, &n_tasks);

    /* sort them by frequency */
    task_sort_by_frequency(tasks, n_tasks);

    /* print information about configuration */
    task_print_information(tasks, n_tasks);

    /* assign each task their semaphore */
    task_init_semaphore(tasks, n_tasks);

    /* assign each task their priority */
    task_init_priority(tasks, n_tasks, sched_get_priority_max(POLICY));

    /* assign each task their thread */
    task_init_thread(tasks, n_tasks, POLICY, thread_work);

    /* start the timers */
    init_timers(timers, tasks, n_tasks);

    /* the end */
    task_join_and_destroy(tasks, n_tasks);

    return 0;
}
