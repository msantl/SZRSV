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
pthread_mutex_t m_datagram_list, m_get_id;

struct sockaddr lift_server, tipke_server;
socklen_t tipke_server_l, lift_server_l;

int lift_socket, tipke_socket;

struct list_t *datagram_list;
int ID, RUNNING;

/*
 * UPR specific
 * TODO:
 *  upravljanje liftom
 *  upravljanje tipkama
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

void *upr(void *arg) {

    /* do upr things */
    while (RUNNING) {
        /* provjeri stanje lifta i posalji naredbe za kretanje */
    }

    return NULL;
}

void *udp_listener(void *arg) {
    char upr_hostname[MAX_BUFF], upr_port[MAX_BUFF];
    int server_socket;

    struct message_t msg;
    struct sockaddr client;
    socklen_t client_l = sizeof(client);

    ParseHostnameAndPort("UPR", (char **) &upr_hostname, (char **) &upr_port);
#ifdef DEBUG
    printf("Listening as UPR on %s:%s\n", upr_hostname, upr_port);
#endif
    server_socket = InitUDPServer(upr_port);
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
            case TIPKE_KEY_PRESSED_UP:
                send_ack(msg.id, tipke_socket, &tipke_server, tipke_server_l);
                /* TODO: zabiljezi akciju */

                msg.id = GetNewMessageID();
                msg.type = TIPKE_PALI_LAMPICU_UP;
                /* msg.data stays the same */

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(tipke_socket, &msg, sizeof(msg), 0, &tipke_server, tipke_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case TIPKE_KEY_PRESSED_DOWN:
                send_ack(msg.id, tipke_socket, &tipke_server, tipke_server_l);
                /* TODO: zabiljezi akciju */

                msg.id = GetNewMessageID();
                msg.type = TIPKE_PALI_LAMPICU_DOWN;
                /* msg.data stays the same */

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(tipke_socket, &msg, sizeof(msg), 0, &tipke_server, tipke_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case LIFT_STOP:
                send_ack(msg.id, lift_socket, &lift_server, lift_server_l);
                /* TODO */

                break;
            case LIFT_KEY_PRESSED:
                send_ack(msg.id, lift_socket, &lift_server, lift_server_l);
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

    CloseUDPClient(tipke_socket);
#ifdef DEBUG
    printf("Client TIPKE closed successfully\n");
#endif

    CloseUDPClient(lift_socket);
#ifdef DEBUG
    printf("Client LIFT1 closed successfully\n");
#endif

    errx(USER_FAILURE, "User wants to quit!");
}

int main(int argc, char **argv) {
    int i;

    InitListHead(&datagram_list);
    ID = RUNNING = 1;

    char lift_hostname[MAX_BUFF], lift_port[MAX_BUFF];
    char tipke_hostname[MAX_BUFF], tipke_port[MAX_BUFF];

    lift_server_l = sizeof(lift_server);
    tipke_server_l = sizeof(tipke_server);

    ParseHostnameAndPort("LIFT1", (char **) &lift_hostname, (char **) &lift_port);
#ifdef DEBUG
    printf("Reaching LIFT1 on %s:%s\n", lift_hostname, lift_port);
#endif
    ParseHostnameAndPort("TIPKE", (char **) &tipke_hostname, (char **) &tipke_port);
#ifdef DEBUG
    printf("Reaching TIPKE on %s:%s\n", tipke_hostname, tipke_port);
#endif

    lift_socket = InitUDPClient(lift_hostname, lift_port, &lift_server);
#ifdef DEBUG
    printf("Client for LIFT1 started successfully\n");
#endif

    tipke_socket = InitUDPClient(tipke_hostname, tipke_port, &tipke_server);
#ifdef DEBUG
    printf("Client for TIPKE started successfully\n");
#endif

    sigset(SIGINT, kraj);

    /* Inicijalizacija mutexa */
    pthread_mutex_init(&m_get_id, NULL);
    pthread_mutex_init(&m_datagram_list, NULL);

    /* Glavna dretva lifta */
    if (pthread_create(&thread[0], NULL, upr, NULL)) {
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
