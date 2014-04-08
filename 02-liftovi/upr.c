#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include "time.h"

#include "lift_state.h"
#include "message.h"
#include "list.h"
#include "udp.h"
#include "parse.h"
#include "constants.h"

#define THREAD_CNT  3

/* GLOBAL VARIABLES */
pthread_t thread[THREAD_CNT];
pthread_mutex_t m_datagram_list, m_get_id, m_action;

struct sockaddr lift_server[ELEVATORS], tipke_server;
socklen_t tipke_server_l, lift_server_l[ELEVATORS];

int lift_socket[ELEVATORS], tipke_socket;

char *lift_name[ELEVATORS] = {"LIFT1", "LIFT2", "LIFT3"};
struct list_t *datagram_list;

int ID, RUNNING;

/*
 * UPR specific
 */
int naredba[ELEVATORS][FLOORS][4][2][2];
int tipke_u_liftu[ELEVATORS][FLOORS];
int tipke_na_katu[FLOORS][2];

struct lift_state_t current[ELEVATORS];

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
    int i, j, lift_id;
    int current_request_up, current_request_down;

    struct message_t msg;
    struct timespec now;

    while(RUNNING) {
        /* request tipke status */
        msg.id = GetNewMessageID();
        msg.type = TIPKE_STATUS_REQUEST;

        ClockGetTime(&msg.timeout);
        ClockAddTimeout(&msg.timeout, TIMEOUT);

        pthread_mutex_lock(&m_datagram_list);

        sendto(tipke_socket, &msg, sizeof(msg), 0, &tipke_server, tipke_server_l);
        ListInsert(&datagram_list, msg);

        pthread_mutex_unlock(&m_datagram_list);

        /* request lift status */
        for (i = 0; i < ELEVATORS; ++i) {
            msg.id = GetNewMessageID();
            msg.type = LIFT_ACTION_START;

            memset(msg.data, 0, sizeof(msg.data));
            sprintf(msg.data, "%d", A_LIFT_STATUS);

            ClockGetTime(&msg.timeout);
            ClockAddTimeout(&msg.timeout, TIMEOUT);

            pthread_mutex_lock(&m_datagram_list);

            sendto(lift_socket[i], &msg, sizeof(msg), 0, &lift_server[i], lift_server_l[i]);
            ListInsert(&datagram_list, msg);

            pthread_mutex_unlock(&m_datagram_list);
        }

        usleep(UPR_FREQUENCY);

        /* upravljacka logika */
        pthread_mutex_lock(&m_action);

        /* tipke u liftovima */
        for (i = 0; i < ELEVATORS; ++i) {
            for (j = 0; j < FLOORS; ++j) {
                if (tipke_u_liftu[i][j] == 1 && current[i].stani[j] == 0) {
                    current[i].stani[j] = 1;
                }
            }
        }

        /* tipke na katovima */
        for (j = 0; j < FLOORS; ++j) {
            lift_id = -1;
            if (tipke_na_katu[j][D_UP]) {
                /* nadji najblizi lift za (j, UP) */
                for (i = 0; i < ELEVATORS; ++i) {
                    if (current[i].floor < j && current[i].direction == D_UP) {
                        lift_id = i;
                    }
                }
            } else if (tipke_na_katu[j][D_DOWN]) {
                /* nadji najblizi lift u za (j, DOWN) */
                for (i = 0; i < ELEVATORS; ++i) {
                    if (current[i].floor > j && current[i].direction == D_DOWN) {
                        lift_id = i;
                    }
                }
            }

            /* ako smo pronasli lift koji moze obraditi zahtjev */
            if (lift_id != -1) {
                if (current[lift_id].stani[j] == 0) {
                    current[lift_id].stani[j] = 1;
                }
            }
        }

        /* azuriraj strukturu naredbe */
        for (i = 0; i < ELEVATORS; ++i) {
            if (current[i].state == S_STOJI_OTVOREN && current[i].stani[current[i].floor] == 1) {
                /* produzi vrijeme s otvorenim vratima */
                ClockGetTime(&current[i].cekaj_do);
                ClockAddTimeout(&current[i].cekaj_do, ACTION_TIME);

            } else if (current[i].state == S_STOJI_ZATVOREN && current[i].stani[current[i].floor] == 1) {
                naredba[i][current[i].floor][0][current[i].direction][V_ZATVORENA] = A_LIFT_OTVORI;

            } else if (current[i].direction == D_UP) {
                for (j = current[i].floor + 1; j < FLOORS; ++j) {
                    if (current[i].stani[j] == 1) {
                        naredba[i][j-1][2][D_UP][V_ZATVORENA] = A_LIFT_STANI_NA_KATU;
                        naredba[i][j][0][D_UP][V_ZATVORENA] = A_LIFT_OTVORI;
                    }
                }
            } else if (current[i].direction == D_DOWN) {
                for (j = current[i].floor - 1; j >= 0; --j) {
                    if (current[i].stani[j] == 1) {
                        naredba[i][j][2][D_UP][V_ZATVORENA] = A_LIFT_STANI_NA_KATU;
                        naredba[i][j][0][D_UP][V_ZATVORENA] = A_LIFT_OTVORI;
                    }
                }
            }
        }

        /* obradi liftove s otvorenim vratima */
        current_request_up = current_request_down = 0;

        for (i = 0; i < ELEVATORS; ++i) {
            if (current[i].state == -1) continue;

            if (current[i].state == S_STOJI_OTVOREN) {
                current[i].stani[current[i].floor] = 0;
                tipke_u_liftu[i][current[i].floor] = 0;

                /* posalji naredbe za gasenje tipke lift[i].floor u liftu i */
#ifdef DEBUG
                printf("Saljem liftu %d da ugasi lampicu\n", i);
#endif
                msg.id = GetNewMessageID();
                msg.type = LIFT_GASI_LAMPICU;

                memset(msg.data, 0, sizeof msg.data);
                sprintf(msg.data, "%d", current[i].floor);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(lift_socket[i], &msg, sizeof(msg), 0, &lift_server[i], lift_server_l[i]);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                /* posalji naredbe za gasenje tipke lift[i].direction na katu lift[i].floor */
#ifdef DEBUG
                printf("Saljem tipkama da ugasi lampicu\n");
#endif
                msg.id = GetNewMessageID();

                if (current[i].direction == D_UP) {
                    msg.type = TIPKE_GASI_LAMPICU_UP;
                } else if (current[i].direction == D_DOWN) {
                    msg.type = TIPKE_GASI_LAMPICU_DOWN;
                } else {
                    warnx("What direction!?");
                    break;
                }

                memset(msg.data, 0, sizeof msg.data);
                sprintf(msg.data, "%d", current[i].floor);

                ClockGetTime(&msg.timeout);
                ClockAddTimeout(&msg.timeout, TIMEOUT);

                pthread_mutex_lock(&m_datagram_list);

                sendto(tipke_socket, &msg, sizeof(msg), 0, &tipke_server, tipke_server_l);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                /* postavi neaktivnost na ACTION_TIME */
                ClockGetTime(&current[i].cekaj_do);
                ClockAddTimeout(&current[i].cekaj_do, ACTION_TIME);

                /* zatvori vrata */
                naredba[i][current[i].floor][0][current[i].direction][V_OTVORENA] = A_LIFT_ZATVORI;

            } else if (current[i].state == S_STOJI_ZATVOREN && current[i].direction == D_UP) {
                current[i].door = V_ZATVORENA;

                for (j = 0; j < FLOORS; ++j) {
                    if (j > current[i].floor &&
                        (tipke_na_katu[j][D_UP] == 1 ||
                         tipke_u_liftu[i][j] == 1)
                    ) {
                        current_request_up = 1;
                    } else if (j < current[i].floor &&
                        (tipke_na_katu[j][D_DOWN] == 1 ||
                         tipke_na_katu[j][D_UP] == 1 ||
                         tipke_u_liftu[i][j] == 1)
                    ) {
                        current_request_down = 1;
                    }
                }

                if (current_request_up) {
                    /* ako ima zahtjeva u mom smjeru */
                    naredba[i][current[i].floor][0][current[i].direction][V_ZATVORENA] = A_LIFT_GORE;
                } else if (current_request_down) {
                    /* ako ima zahtjeva u drugom smjeru */
                    naredba[i][current[i].floor][0][current[i].direction][V_ZATVORENA] = A_LIFT_DOLE;
                } else {
                    /* inace stoji */
                    current[i].direction = D_STOJI;
                }
            } else if (current[i].state == S_STOJI_ZATVOREN && current[i].direction == D_DOWN) {
                current[i].door = V_ZATVORENA;

                for (j = 0; j < FLOORS; ++j) {
                    if (j < current[i].floor &&
                        (tipke_na_katu[j][D_DOWN] == 1 ||
                         tipke_u_liftu[i][j] == 1)
                    ) {
                        current_request_down = 1;
                    } else if (j > current[i].floor &&
                        (tipke_na_katu[j][D_DOWN] == 1 ||
                         tipke_na_katu[j][D_UP] == 1 ||
                         tipke_u_liftu[i][j] == 1)
                    ) {
                        current_request_up = 1;
                    }
                }

                if (current_request_down) {
                    /* ako ima zahtjeva u mom smjeru */
                    naredba[i][current[i].floor][0][current[i].direction][V_ZATVORENA] = A_LIFT_DOLE;
                } else if (current_request_up) {
                    /* ako ima zahtjeva u drugom smjeru */
                    naredba[i][current[i].floor][0][current[i].direction][V_ZATVORENA] = A_LIFT_GORE;
                } else {
                    /* inace stoji */
                    current[i].direction = D_STOJI;
                }

            } else if (current[i].state == S_STOJI_ZATVOREN) {
                /* standing by */
                // TODO
                // nadji najblizi zahtjev i uputi lift tamo
                // naredba[i][current[i].floor][0][current[i].direction][V_ZATVORENA] = A_LIFT_GORE;
            }
        }

        ClockGetTime(&now);

        /* salji naredbe */
        for (i = 0; i < ELEVATORS; ++i) {
            if (naredba[i][current[i].floor][current[i].im_floor][current[i].direction][current[i].door] != 0 &&
                (current[i].cekaj_do.tv_sec <= now.tv_sec ||
                (current[i].cekaj_do.tv_sec == now.tv_sec &&
                 current[i].cekaj_do.tv_nsec <= now.tv_nsec))) {

                /* posalji naredbu */

                naredba[i][current[i].floor][current[i].im_floor][current[i].direction][current[i].door] = 0;
                current[i].cekaj_do = now;
            }
        }

        pthread_mutex_unlock(&m_action);
        /* kraj petlje */
    }

    return NULL;
}

void *udp_listener(void *arg) {
    char upr_hostname[MAX_BUFF], upr_port[MAX_BUFF];
    int server_socket;

    int ack, floor;
    int lift_id;

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

        lift_id = msg.lift;

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
                pthread_mutex_lock(&m_action);
                tipke_na_katu[floor][D_UP] = 1;
                pthread_mutex_unlock(&m_action);
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
                pthread_mutex_lock(&m_action);
                tipke_na_katu[floor][D_DOWN] = 1;
                pthread_mutex_unlock(&m_action);
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
                send_ack(msg.id, lift_socket[lift_id], &lift_server[lift_id], lift_server_l[lift_id]);
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

                sendto(lift_socket[lift_id], &msg, sizeof(msg), 0, &lift_server[lift_id], lift_server_l[lift_id]);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case LIFT_KEY_PRESSED:
                send_ack(msg.id, lift_socket[lift_id], &lift_server[lift_id], lift_server_l[lift_id]);

                sscanf(msg.data, "%d", &floor);

                pthread_mutex_lock(&m_action);
                tipke_u_liftu[lift_id][floor] = 1;
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

                sendto(lift_socket[lift_id], &msg, sizeof(msg), 0, &lift_server[lift_id], lift_server_l[lift_id]);
                ListInsert(&datagram_list, msg);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case LIFT_STATUS_REPORT:
                send_ack(msg.id, lift_socket[lift_id], &lift_server[lift_id], lift_server_l[lift_id]);

                pthread_mutex_lock(&m_action);

                from_string(msg.data, &current[lift_id]);

                pthread_mutex_unlock(&m_action);
#ifdef DEBUG
                printf("Lift is alive and well!\n");
#endif
                break;
            case TIPKE_STATUS_REPORT:
                send_ack(msg.id, tipke_socket, &tipke_server, tipke_server_l);
#ifdef DEBUG
                printf("Tipke are alive and well!\n");
#endif
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
    int i;
    RUNNING = 0;

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

    for (i = 0; i < ELEVATORS; ++i) {
        CloseUDPClient(lift_socket[i]);
#ifdef DEBUG
        printf("Client LIFT1 closed successfully\n");
#endif
    }

    errx(USER_FAILURE, "User wants to quit!");
}

int main(int argc, char **argv) {
    int i;

    InitListHead(&datagram_list);
    ID = RUNNING = 1;

    char lift_hostname[MAX_BUFF], lift_port[MAX_BUFF];
    char tipke_hostname[MAX_BUFF], tipke_port[MAX_BUFF];

    tipke_server_l = sizeof(tipke_server);

    ParseHostnameAndPort("TIPKE", (char **) &tipke_hostname, (char **) &tipke_port);
#ifdef DEBUG
    printf("Reaching TIPKE on %s:%s\n", tipke_hostname, tipke_port);
#endif

    for (i = 0; i < ELEVATORS; ++i) {
        memset(lift_hostname, 0, sizeof lift_hostname);
        memset(lift_port, 0, sizeof lift_port);

        lift_server_l[i] = sizeof(lift_server[i]);

        ParseHostnameAndPort(lift_name[i], (char **) &lift_hostname, (char **) &lift_port);
#ifdef DEBUG
        printf("Reaching %s on %s:%s\n", lift_name[i], lift_hostname, lift_port);
#endif
        lift_socket[i] = InitUDPClient(lift_hostname, lift_port, &lift_server[i]);
#ifdef DEBUG
        printf("Client for %s started successfully\n", lift_name[i]);
#endif
    }

    tipke_socket = InitUDPClient(tipke_hostname, tipke_port, &tipke_server);
#ifdef DEBUG
    printf("Client for TIPKE started successfully\n");
#endif

    sigset(SIGINT, kraj);

    /* Inicijalizacija liftova */
    for (i = 0; i < ELEVATORS; ++i) {
        current[i].state = -1;
    }

    memset(naredba, 0, sizeof naredba);
    memset(tipke_u_liftu, 0, sizeof tipke_u_liftu);
    memset(tipke_na_katu, 0, sizeof tipke_na_katu);

    /* Inicijalizacija mutexa */
    pthread_mutex_init(&m_get_id, NULL);
    pthread_mutex_init(&m_action, NULL);
    pthread_mutex_init(&m_datagram_list, NULL);

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
