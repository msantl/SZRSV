
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include "time.h"

#include "message.h"
#include "list.h"
#include "udp.h"
#include "parse.h"
#include "constants.h"

#define THREAD_CNT  3

/* GLOBAL VARIABLES */
pthread_t thread[THREAD_CNT];
pthread_mutex_t m_datagram_list;

struct list_t *datagram_list;
int RUNNING;

void *tipke(void *arg) {
    char cmd[MAX_BUFF];
    char upr_hostname[MAX_BUFF], upr_port[MAX_BUFF];
    int upr_socket;

    struct message_t msg;
    struct sockaddr upr_server;
    socklen_t server_l = sizeof(upr_server);

    ParseHostnameAndPort("UPR", (char **) &upr_hostname, (char **) &upr_port);
#ifdef DEBUG
    printf("Reaching UPR on %s:%s\n", upr_hostname, upr_port);
#endif
    upr_socket = InitUDPClient(upr_hostname, upr_port, &upr_server);
#ifdef DEBUG
    printf("Client started successfully\n");
#endif

    /* do tipke things */
    while (RUNNING) {
        scanf("%s", cmd);
    }

    CloseUDPClient(upr_socket);
#ifdef DEBUG
    printf("Client closed successfully\n");
#endif
    return NULL;
}

void *udp_listener(void *arg) {
    char tipke_hostname[MAX_BUFF], tipke_port[MAX_BUFF];
    int server_socket;

    struct message_t msg;
    struct sockaddr client;
    socklen_t client_l = sizeof(client);

    ParseHostnameAndPort("TIPKE", (char **) &tipke_hostname, (char **) &tipke_port);
#ifdef DEBUG
    printf("Listening as TIPKE on %s:%s\n", tipke_hostname, tipke_port);
#endif
    server_socket = InitUDPServer(tipke_port);
#ifdef DEBUG
    printf("Server started successfully\n");
#endif

    /* waiting for next datagram */


    CloseUDPServer(server_socket);
#ifdef DEBUG
    printf("Server closed successfully\n");
#endif
    return NULL;
}

void *check_list(void *arg) {

    while (RUNNING) {
        pthread_mutex_lock(&m_datagram_list);

        struct timespec now;
        ClockGetTime(&now);
#ifdef DEBUG
        printf("Checking datagram list for expired datagrams\n");
#endif
        ListRemoveByTimeout(&datagram_list, now);

        pthread_mutex_unlock(&m_datagram_list);
        usleep(FREQUENCY);
    }

    return NULL;
}

void kraj(int sig) {
    RUNNING = 0;
    warnx("User wants to quit!");
    return;
}

int main(int argc, char **argv) {
    int i;

    InitListHead(&datagram_list);
    RUNNING = 1;

    sigset(SIGINT, kraj);

    /* Glavna dretva lifta */
    if (pthread_create(&thread[0], NULL, tipke, NULL)) {
        errx(THREAD_FAILURE, "Could not create new thread :(");
    }

    /* Primanje paketa */
    if (pthread_create(&thread[1], NULL, udp_listener, NULL)) {
        errx(THREAD_FAILURE, "Could not create new thread :(");
    }

    /* Provjera liste potvrdjenih paketa */
    if (pthread_create(&thread[2], NULL, check_list, NULL)) {
        errx(THREAD_FAILURE, "Could not create new thread :(");
    }

    for (i = 0; i < THREAD_CNT; ++i) {
        pthread_join(thread[i], NULL);
    }

    return 0;
}

