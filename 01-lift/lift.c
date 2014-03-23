#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include "time.h"

#include "message.h"
#include "list.h"
#include "udp.h"
#include "parse.h"
#include "constants.h"

#define THREAD_CNT  4

/* GLOBAL VARIABLES */
pthread_t thread[THREAD_CNT];
pthread_mutex_t m_datagram_list, m_get_id, m_action;

struct sockaddr upr_server;
socklen_t upr_server_l;

int upr_socket;

struct list_t *datagram_list;
int ID, RUNNING;

/*
 * LIFT specific
 * TODO:
 *  lampice u liftu
 *  stanje lifta
 *  ...
 */

int GetNewMessageID(void) {
    int ret;
    pthread_mutex_lock(&m_get_id);
    ret = ID++;
    pthread_mutex_unlock(&m_get_id);
    return ret;
}

void send_ack(int ack, int sockfd, struct sockaddr *server, socklen_t server_l) {
    struct message_t resp;
    resp.id = GetNewMessageID();
    resp.type = POTVRDA;

    sprintf(resp.data, "%d", ack);

    ClockGetTime(&resp.timeout);
    ClockAddTimeout(&resp.timeout, TIMEOUT);

#ifdef DEBUG
    printf("Sending ACK for message %d\n", ack);
#endif
    sendto(sockfd, &resp, sizeof(resp), 0, server, server_l);

    return;
}

void *lift(void *arg) {
    /* do lift things */
    int current_action = -1;
    struct timespec current_action_timeout, now;

    struct message_t msg;

    while (RUNNING) {
        ClockGetTime(&now);

        pthread_mutex_lock(&m_action);
        if (~current_action &&
            (current_action_timeout.tv_sec < now.tv_sec ||
            (current_action_timeout.tv_sec == now.tv_sec &&
             current_action_timeout.tv_nsec < now.tv_nsec))) {

            /* promjena stanja sustava */
#ifdef DEBUG
            printf("Finished with action %d\n", current_action);
#endif
            /* slanje statusa UPR-u */

            msg.id = GetNewMessageID();
            msg.type = LIFT_ACTION_FINISHED;

            sprintf(msg.data, "%d", current_action);

            ClockGetTime(&msg.timeout);
            ClockAddTimeout(&msg.timeout, TIMEOUT);

            pthread_mutex_lock(&m_datagram_list);

            sendto(upr_socket, &msg, sizeof(msg), 0, &upr_server, upr_server_l);
            ListInsert(&datagram_list, msg);

            pthread_mutex_unlock(&m_datagram_list);

        }
        pthread_mutex_unlock(&m_action);

        usleep(LIFT_FREQUENCY);
    }

    return NULL;
}

void *udp_listener(void *arg) {
    char lift_hostname[MAX_BUFF], lift_port[MAX_BUFF];
    int server_socket;

    struct message_t msg;
    struct sockaddr client;
    socklen_t client_l = sizeof(client);

    ParseHostnameAndPort("LIFT1", (char **) &lift_hostname, (char **) &lift_port);
#ifdef DEBUG
    printf("Listening as LIFT1 on %s:%s\n", lift_hostname, lift_port);
#endif
    server_socket = InitUDPServer(lift_port);
#ifdef DEBUG
    printf("Server started successfully\n");
#endif

    while (RUNNING) {
        /* waiting for next datagram */
        recvfrom(server_socket, &msg, sizeof(msg), 0, &client, &client_l);

        switch(msg.type) {
            case POTVRDA:
                /* received ack */
                pthread_mutex_lock(&m_datagram_list);

                int ack; sscanf(msg.data, "%d", &ack);

                printf("Received ACK for message %d\n", ack);
                ListRemoveById(&datagram_list, ack);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            default:
                warnx("Unknown message!");
                break;
        }
    }

    CloseUDPServer(server_socket);
#ifdef DEBUG
    printf("Server closed successfully\n");
#endif
    return NULL;
}

void *key_listener(void *arg) {
    char cmd[MAX_BUFF];
    int floor;

    struct message_t msg;

    while (RUNNING) {
        memset(cmd, 0, sizeof(cmd));

        /* format: floor|STOP */
        scanf("%s", cmd);

        if (sscanf(cmd, "%d", &floor) == 1) {
            msg.type = LIFT_KEY_PRESSED;
        } else if (strcmp(cmd, "STOP") == 0) {
            msg.type = LIFT_STOP;
        } else {
            warnx("Unknown command (use 0-9 for floors and STOP)!");
            continue;
        }

        msg.id = GetNewMessageID();
        sprintf(msg.data, "%d", floor);

        ClockGetTime(&msg.timeout);
        ClockAddTimeout(&msg.timeout, TIMEOUT);

        pthread_mutex_lock(&m_datagram_list);

        sendto(upr_socket, &msg, sizeof(msg), 0, &upr_server, upr_server_l);
        ListInsert(&datagram_list, msg);

        pthread_mutex_unlock(&m_datagram_list);
    }

    return NULL;
}

void *check_list(void *arg) {

    while (RUNNING) {
        pthread_mutex_lock(&m_datagram_list);

        struct timespec now;
        ClockGetTime(&now);

        ListRemoveByTimeout(&datagram_list, now);

        pthread_mutex_unlock(&m_datagram_list);
        usleep(LIST_FREQUENCY);
    }

    return NULL;
}

void kraj(int sig) {
    RUNNING = 0;
    ListDelete(&datagram_list);
#ifdef DEBUG
    printf("Deleted all elements from datagram list\n");
#endif

    CloseUDPClient(upr_socket);
#ifdef DEBUG
    printf("Client closed successfully\n");
#endif

    errx(USER_FAILURE, "User wants to quit!");
}

int main(int argc, char **argv) {
    int i;

    InitListHead(&datagram_list);
    ID = RUNNING = 1;

    char upr_hostname[MAX_BUFF], upr_port[MAX_BUFF];
    upr_server_l = sizeof(upr_server);

    ParseHostnameAndPort("UPR", (char **) &upr_hostname, (char **) &upr_port);
#ifdef DEBUG
    printf("Reaching UPR on %s:%s\n", upr_hostname, upr_port);
#endif

    upr_socket = InitUDPClient(upr_hostname, upr_port, &upr_server);
#ifdef DEBUG
    printf("Client started successfully\n");
#endif

    sigset(SIGINT, kraj);

    /* Inicijalizacija mutexa */
    pthread_mutex_init(&m_get_id, NULL);
    pthread_mutex_init(&m_datagram_list, NULL);
    pthread_mutex_init(&m_action, NULL);

    /* Glavna dretva lifta */
    if (pthread_create(&thread[0], NULL, lift, NULL)) {
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

    /* Pritisak tipke unutar lifta  */
    if (pthread_create(&thread[3], NULL, key_listener, NULL)) {
        errx(THREAD_FAILURE, "Could not create new thread :(");
    }

    for (i = 0; i < THREAD_CNT; ++i) {
        pthread_join(thread[i], NULL);
    }

    return 0;
}
