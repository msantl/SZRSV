#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include "time.h"

#include "set.h"
#include "message.h"
#include "list.h"
#include "udp.h"
#include "parse.h"
#include "constants.h"

#define THREAD_CNT  3

/* GLOBAL VARIABLES */
pthread_t thread[THREAD_CNT];
pthread_mutex_t m_datagram_list, m_get_id, m_request_list, m_action;
pthread_cond_t c_action;

struct sockaddr lift_server, tipke_server;
socklen_t tipke_server_l, lift_server_l;

int lift_socket, tipke_socket;

struct set_t *request_list;
struct list_t *datagram_list;

int ID, RUNNING;

/*
 * UPR specific
 *  upravljanje liftom
 *  upravljanje tipkama
 */
int current_floor, current_im_floor;
int current_state, current_direction;

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
    struct message_t msg;

    while(RUNNING) {
        usleep(UPR_FREQUENCY);
        if (!request_list) continue;

        /* request status */
        msg.id = GetNewMessageID();
        msg.type = LIFT_ACTION_START;

        memset(msg.data, 0, sizeof(msg.data));
        sprintf(msg.data, "%d", A_LIFT_STATUS);

        ClockGetTime(&msg.timeout);
        ClockAddTimeout(&msg.timeout, TIMEOUT);

        pthread_mutex_lock(&m_datagram_list);

        sendto(lift_socket, &msg, sizeof(msg), 0, &lift_server, lift_server_l);
        ListInsert(&datagram_list, msg);

        pthread_mutex_unlock(&m_datagram_list);

        /* cekaj na odgovor sa statusom */
        pthread_mutex_lock(&m_action);
#ifdef DEBUG
        printf("Waiting status!\n");
#endif
        pthread_cond_wait(&c_action, &m_action);
#ifdef DEBUG
        printf("Received status!\n");
#endif

        /* update set structure */
        pthread_mutex_lock(&m_request_list);
        SetSort(&request_list, current_floor, current_direction);
        pthread_mutex_unlock(&m_request_list);

#ifdef DEBUG
        pthread_mutex_lock(&m_request_list);
        printf("Requests: ");
        for (struct set_t *curr = request_list; curr; curr = curr->next) {
            printf("(%d %d) ", curr->floor, curr->direction);
        }
        printf("\n");
        pthread_mutex_unlock(&m_request_list);
#endif

        /* upravljacki dio */
        switch(current_state) {
            case S_OTVARA_VRATA:
                break;
            case S_ZATVARA_VRATA:
                break;
            case S_STOJI_OTVOREN:
                /* A_LIFT_ZATVORI */
#ifdef DEBUG
                printf("Saljem liftu da zatvori vrata!\n");
#endif
                msg.id = GetNewMessageID();
                msg.type = LIFT_ACTION_START;

                memset(msg.data, 0, sizeof(msg.data));
                sprintf(msg.data, "%d", A_LIFT_ZATVORI);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(lift_socket, &msg, sizeof(msg), 0, &lift_server, lift_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                /* LIFT_GASI_LAMPICU */
#ifdef DEBUG
                printf("Saljem liftu da ugasi lampicu!\n");
#endif
                msg.id = GetNewMessageID();
                msg.type = LIFT_GASI_LAMPICU;

                memset(msg.data, 0, sizeof(msg.data));
                sprintf(msg.data, "%d", current_floor);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(lift_socket, &msg, sizeof(msg), 0, &lift_server, lift_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                /* TIPKE_GASI_LAMPICU current_direction current_floor */
#ifdef DEBUG
                printf("Saljem tipkama da ugase lampicu!\n");
#endif
                msg.id = GetNewMessageID();

                if (current_direction == D_UP) {
                    msg.type = TIPKE_GASI_LAMPICU_UP;
                } else if (current_direction == D_DOWN) {
                    msg.type = TIPKE_GASI_LAMPICU_DOWN;
                } else {
                    warnx("What direction!?");
                    break;
                }

                memset(msg.data, 0, sizeof(msg.data));
                sprintf(msg.data, "%d", current_floor);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(tipke_socket, &msg, sizeof(msg), 0, &tipke_server, tipke_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                /* zahtjev je posluzen */
                pthread_mutex_lock(&m_request_list);
                if (request_list->floor == current_floor &&
                    request_list->direction == current_direction) {

                    /* obrisi zahtjev iz liste */
                    SetRemove(&request_list, request_list->floor, request_list->direction);
                }
                pthread_mutex_unlock(&m_request_list);

                break;
            case S_STOJI_ZATVOREN:
                /* A_LIFT_GORE ili A_LIFT_DOLE */
                msg.id = GetNewMessageID();
                msg.type = LIFT_ACTION_START;

                pthread_mutex_lock(&m_request_list);
                memset(msg.data, 0, sizeof(msg.data));

                if (request_list->floor == current_floor) {
                    sprintf(msg.data, "%d", A_LIFT_OTVORI);
#ifdef DEBUG
                    printf("Saljem liftu da otvori vrata!\n");
#endif
                } else if (request_list->floor < current_floor) {
                    sprintf(msg.data, "%d", A_LIFT_DOLE);
#ifdef DEBUG
                    printf("Saljem liftu da ide dolje!\n");
#endif
                } else if (request_list->floor > current_floor) {
                    sprintf(msg.data, "%d", A_LIFT_GORE);
#ifdef DEBUG
                    printf("Saljem liftu da ide gore!\n");
#endif
                }

                pthread_mutex_unlock(&m_request_list);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(lift_socket, &msg, sizeof(msg), 0, &lift_server, lift_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case S_IDE_GORE:
                /* A_LIFT_STANI_NA_KATU */
                msg.id = GetNewMessageID();
                msg.type = LIFT_ACTION_START;

                pthread_mutex_lock(&m_request_list);
                memset(msg.data, 0, sizeof(msg.data));

                if (request_list->floor == current_floor + 1 &&
                    current_im_floor > 0 &&
                    request_list->direction != D_DOWN) {

                    sprintf(msg.data, "%d", A_LIFT_STANI_NA_KATU);
#ifdef DEBUG
                    printf("Saljem liftu da stane na sljedecem katu!\n");
#endif
                } else {
                    pthread_mutex_unlock(&m_request_list);
                    break;
                }

                pthread_mutex_unlock(&m_request_list);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(lift_socket, &msg, sizeof(msg), 0, &lift_server, lift_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case S_IDE_DOLE:
                /* A_LIFT_STANI_NA_KATU */
                msg.id = GetNewMessageID();
                msg.type = LIFT_ACTION_START;

                pthread_mutex_lock(&m_request_list);
                memset(msg.data, 0, sizeof(msg.data));

                if (request_list->floor == current_floor &&
                    current_im_floor > 0 &&
                    request_list->direction != D_UP) {

                    sprintf(msg.data, "%d", A_LIFT_STANI_NA_KATU);
#ifdef DEBUG
                    printf("Saljem liftu da stane na sljedecem katu!\n");
#endif
                } else {
                    pthread_mutex_unlock(&m_request_list);
                    break;
                }

                pthread_mutex_unlock(&m_request_list);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(lift_socket, &msg, sizeof(msg), 0, &lift_server, lift_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            default:
                warnx("Don't know what to do with this state!");
                break;
        }

        pthread_mutex_unlock(&m_action);
    }

    return NULL;
}

void *udp_listener(void *arg) {
    char upr_hostname[MAX_BUFF], upr_port[MAX_BUFF];
    int server_socket;

    int ack, floor;

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

                sscanf(msg.data, "%d", &ack);

#ifdef DEBUG
                printf("Received ACK for message %d\n", ack);
#endif
                ListRemoveById(&datagram_list, ack);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case TIPKE_KEY_PRESSED_UP:
                send_ack(msg.id, tipke_socket, &tipke_server, tipke_server_l);

                sscanf(msg.data, "%d", &floor);

                /* insert new request */
                pthread_mutex_lock(&m_request_list);
                SetInsert(&request_list, floor, D_UP);
                pthread_mutex_unlock(&m_request_list);
#ifdef DEBUG
                printf("Requested %d. floor going UP.\n", floor);
#endif

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

                sscanf(msg.data, "%d", &floor);

                /* insert new request */
                pthread_mutex_lock(&m_request_list);
                SetInsert(&request_list, floor, D_DOWN);
                pthread_mutex_unlock(&m_request_list);
#ifdef DEBUG
                printf("Requested %d. floor going DOWN.\n", floor);
#endif

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
#ifdef DEBUG
                printf("Elevator is going to stop!\n");
#endif

                msg.id = GetNewMessageID();
                msg.type = LIFT_ACTION_START;

                memset(msg.data, 0, sizeof(msg.data));
                sprintf(msg.data, "%d", A_LIFT_STANI);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(lift_socket, &msg, sizeof(msg), 0, &lift_server, lift_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case LIFT_KEY_PRESSED:
                send_ack(msg.id, lift_socket, &lift_server, lift_server_l);

                sscanf(msg.data, "%d", &floor);

                /* insert new request */
                pthread_mutex_lock(&m_action);
                pthread_mutex_lock(&m_request_list);
                if (current_floor < floor) {
                    SetInsert(&request_list, floor, D_UP);
                } else {
                    SetInsert(&request_list, floor, D_DOWN);
                }
                pthread_mutex_unlock(&m_request_list);
                pthread_mutex_unlock(&m_action);
#ifdef DEBUG
                printf("Requested %d. floor inside elevator.\n", floor);
#endif
                msg.id = GetNewMessageID();
                msg.type = LIFT_PALI_LAMPICU;

                /* msg.data stays the same */

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(lift_socket, &msg, sizeof(msg), 0, &lift_server, lift_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case LIFT_STATUS_REPORT:
                send_ack(msg.id, lift_socket, &lift_server, lift_server_l);

                pthread_mutex_lock(&m_action);

                sscanf(msg.data, "%d-%d-%d-%d",
                        &current_floor,
                        &current_im_floor,
                        &current_state,
                        &current_direction);

                pthread_cond_signal(&c_action);
                pthread_mutex_unlock(&m_action);

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

    pthread_mutex_lock(&m_request_list);
    SetDelete(&request_list);
    pthread_mutex_unlock(&m_request_list);
#ifdef DEBUG
    printf("Deleted all elements from request list\n");
#endif

    pthread_mutex_lock(&m_datagram_list);
    ListDelete(&datagram_list);
    pthread_mutex_unlock(&m_datagram_list);
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
    pthread_mutex_init(&m_action, NULL);
    pthread_mutex_init(&m_datagram_list, NULL);
    pthread_mutex_init(&m_request_list, NULL);

    /* Inicijalizacija varijabli stanja */
    pthread_cond_init(&c_action, NULL);

    /* Pinganje lifta */
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
