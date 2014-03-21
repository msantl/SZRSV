
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

struct sockaddr upr_server;
socklen_t upr_server_l;

int upr_socket;

struct list_t *datagram_list;
int RUNNING;
int ID = 0;

void send_ack(int ack, int sockfd, struct sockaddr *server, socklen_t server_l) {
    struct message_t resp;
    resp.id = -1;
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

        if (*cmd == 'a') {
            msg.type = TIPKE_KEY_PRESSED_UP;
        } else if (*cmd == 'z') {
            msg.type = TIPKE_KEY_PRESSED_DOWN;
        } else {
            warnx("Unkown command (use 'a' for UP and 'z' for down)!");
            continue;
        }

        msg.id = ID++;
        sprintf(msg.data, "%d", floor);

        ClockGetTime(&msg.timeout);
        ClockAddTimeout(&msg.timeout, TIMEOUT);

#ifdef DEBUG
        printf("Sending (%d, %d, \"%s\", %ld.%ld)\n",
                msg.id, msg.type, msg.data,
                msg.timeout.tv_sec, msg.timeout.tv_nsec);
#endif

        pthread_mutex_lock(&m_datagram_list);

        sendto(upr_socket, &msg, sizeof(msg), 0, &upr_server, upr_server_l);
        ListInsert(&datagram_list, msg);

#ifdef DEBUG
        printf("Datagram sent\n");
#endif

        pthread_mutex_unlock(&m_datagram_list);
    }

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

    while (RUNNING) {
        /* waiting for next datagram */
        recvfrom(server_socket, &msg, sizeof(msg), 0, &client, &client_l);
#ifdef DEBUG
        printf("Received (%d, %d, \"%s\", %ld.%ld)\n",
                msg.id, msg.type, msg.data,
                msg.timeout.tv_sec, msg.timeout.tv_nsec);
#endif

        switch(msg.type) {
            case POTVRDA:
                /* received ack */
                pthread_mutex_lock(&m_datagram_list);

                int ack; sscanf(msg.data, "%d", &ack);

                printf("Received ACK for message %d\n", ack);
                ListRemoveById(&datagram_list, ack);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case TIPKE_PALI_LAMPICU_UP:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);
                /* TODO */

                break;
            case TIPKE_PALI_LAMPICU_DOWN:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);
                /* TODO */

                break;
            case TIPKE_GASI_LAMPICU_UP:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);
                /* TODO */

                break;
            case TIPKE_GASI_LAMPICU_DOWN:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);
                /* TODO */

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
#ifdef DEBUG
        /*
        printf("Checking datagram list for expired datagrams\n");
        */
#endif
        ListRemoveByTimeout(&datagram_list, now);

        pthread_mutex_unlock(&m_datagram_list);
        usleep(FREQUENCY);
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
    RUNNING = 1;

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

