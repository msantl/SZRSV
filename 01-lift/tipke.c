
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
pthread_mutex_t m_datagram_list, m_get_id, m_status;

struct sockaddr upr_server;
socklen_t upr_server_l;

int upr_socket;

struct list_t *datagram_list;
int ID, RUNNING;

/*
 * TIPKE specific
 */
int status[FLOORS][2];

void PrintStatus(void) {
    int i;

    pthread_mutex_lock(&m_status);
    printf("Floor  : \tUP \tDOWN\n");

    for (i = 0; i < FLOORS; ++i) {
        printf("Floor %d: \t%c \t%c\n",
                i,
                status[i][0] ? '*' : ' ',
                status[i][1] ? '*' : ' ');
    }

    pthread_mutex_unlock(&m_status);

    return;
}

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

void *tipke(void *arg) {
    char cmd[MAX_BUFF];
    int floor;

    struct message_t msg;

    /* do tipke things */
    while (RUNNING) {
        memset(cmd, 0, sizeof(cmd));

        /* format: floor direction*/
        scanf("%d%s", &floor, cmd);

        if (*cmd == 'a' && floor != 9) {
            msg.type = TIPKE_KEY_PRESSED_UP;
        } else if (*cmd == 'z' && floor != 0) {
            msg.type = TIPKE_KEY_PRESSED_DOWN;
        } else {
            warnx("Unkown command (use 'a' for UP and 'z' for down)!");
            continue;
        }

        msg.id = GetNewMessageID();

        memset(msg.data, 0, sizeof(msg.data));
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

void *udp_listener(void *arg) {
    char tipke_hostname[MAX_BUFF], tipke_port[MAX_BUFF];
    int server_socket;
    int ack, floor;

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

    while (RUNNING) {
        /* waiting for next datagram */
        recvfrom(server_socket, &msg, sizeof(msg), 0, &client, &client_l);

        switch(msg.type) {
            case POTVRDA:
                /* received ack */
                pthread_mutex_lock(&m_datagram_list);

                sscanf(msg.data, "%d", &ack);

#ifdef DEBUG
                printf("Received ACK for message %d\n", ack);
#endif
                ListRemoveById(&datagram_list, ack);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case TIPKE_PALI_LAMPICU_UP:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);

                pthread_mutex_lock(&m_status);

                sscanf(msg.data, "%d", &floor);
                status[floor][D_UP] = 1;

                pthread_mutex_unlock(&m_status);

                PrintStatus();

                break;
            case TIPKE_PALI_LAMPICU_DOWN:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);

                pthread_mutex_lock(&m_status);

                sscanf(msg.data, "%d", &floor);
                status[floor][D_DOWN] = 1;

                pthread_mutex_unlock(&m_status);

                PrintStatus();

                break;
            case TIPKE_GASI_LAMPICU_UP:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);

                pthread_mutex_lock(&m_status);

                sscanf(msg.data, "%d", &floor);
                status[floor][D_UP] = 0;

                pthread_mutex_unlock(&m_status);

                PrintStatus();

                break;
            case TIPKE_GASI_LAMPICU_DOWN:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);

                pthread_mutex_lock(&m_status);

                sscanf(msg.data, "%d", &floor);
                status[floor][D_DOWN] = 0;

                pthread_mutex_unlock(&m_status);

                PrintStatus();

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

    pthread_mutex_lock(&m_datagram_list);
    ListDelete(&datagram_list);
    pthread_mutex_unlock(&m_datagram_list);
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
    memset(status, 0, sizeof(status));

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

    /* Initial print */
    PrintStatus();

    /* Inicijalizacija mutexa */
    pthread_mutex_init(&m_get_id, NULL);
    pthread_mutex_init(&m_datagram_list, NULL);
    pthread_mutex_init(&m_status, NULL);

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

