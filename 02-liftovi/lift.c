#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "time.h"

#include "lift_state.h"
#include "message.h"
#include "list.h"
#include "udp.h"
#include "parse.h"
#include "constants.h"

#define THREAD_CNT  4

/* GLOBAL VARIABLES */
char MY_NAME[MAX_BUFF];
int MY_ID;

pthread_t thread[THREAD_CNT];
pthread_mutex_t m_datagram_list, m_get_id, m_action, m_status;

struct sockaddr upr_server;
socklen_t upr_server_l;

int upr_socket;

struct list_t *datagram_list;
int ID, RUNNING;

/*
 * LIFT specific
 */
struct lift_state_t current;
struct timespec current_action_timeout;
int current_action, current_stop;

char *lift_name[ELEVATORS] = {"LIFT1", "LIFT2", "LIFT3"};
int status[FLOORS];

void GetMyName(int *argc, char ***argv) {
    char c;

    MY_ID = 0;
    while ((c = getopt(*argc, *argv, "n:")) != -1) {
        switch (c) {
            case 'n':
                sscanf(optarg, "%d", &MY_ID);
                break;
            default:
                warnx("Unknown option!");
                break;
        }
    }

    if (MY_ID >= 0 && MY_ID < ELEVATORS) {
        strcpy(MY_NAME, lift_name[MY_ID]);
    } else {
        warnx("Invalid elevator id!");
    }

    return;
}

void PrintStatus(void) {
    int i;
    pthread_mutex_lock(&m_status);

    printf("Current position %d.%d\n",
            current.floor,
            current.im_floor);

    if (current.door == V_OTVORENA) {
        printf("The door is open.\n");
    } else {
        printf("The door is closed.\n");
    }

    if (current.direction == D_UP) {
        printf("Going up!\n");
    } else if (current.direction == D_DOWN) {
        printf("Going down!\n");
    } else {
        printf("Standing by.\n");
    }

    for (i = 0; i < FLOORS; ++i) {
        printf("Floor %d: \t%c\n", i, status[i] ? '*' : ' ');
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
    resp.lift = MY_ID;

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
    int next_state;

    while (RUNNING) {
        usleep(LIFT_FREQUENCY);

        pthread_mutex_lock(&m_action);
        /* if we have a finished action */
        if (current_action == A_OBRADENO) {

            next_state = current.state;

            /* state has changed */
            switch (current.state) {
                case S_STOJI_ZATVOREN:
                case S_STOJI_OTVOREN:
                    break;
                case S_STOJI:
                    errx(LIFT_FAILURE, "STOP");
                    break;
                case S_OTVARA_VRATA:
                    next_state = S_STOJI_OTVOREN;
                    current.door = V_OTVORENA;
                    break;
                case S_ZATVARA_VRATA:
                    next_state = S_STOJI_ZATVOREN;
                    current.door = V_ZATVORENA;
                    break;
                case S_IDE_GORE:
                    if (current.im_floor < 3) {
                        if (current.floor < FLOORS - 1) {
                            current.im_floor += 1;
                        } else {
                            warnx("Can't go higher!");
                        }
                    } else {
                        current.im_floor = 0;
                        current.floor += 1;
                        if (current_stop == 1) {
                            next_state = S_STOJI_ZATVOREN;
                            current_stop = 0;
                        }
                    }
                    break;
                case S_IDE_DOLE:
                    if (current.im_floor == 0) {
                        if (current.floor > 0) {
                            current.im_floor = 3;
                            current.floor -= 1;
                        } else {
                            warnx("Can't go lower!");
                        }
                    } else {
                        current.im_floor -= 1;
                        if (current.im_floor == 0) {
                            if (current_stop == 1) {
                                next_state = S_STOJI_ZATVOREN;
                                current_stop = 0;
                            }
                        }
                    }
                    break;
                default:
                    warnx("Unknown state!");
                    break;
            }

            /* update state */
            current.state = next_state;

            /* reset action */
            current_action = A_OBRADENO;

            /* print lift status */
            PrintStatus();

        } else if (current_action != A_UNDEF && current_action != A_OBRADENO) {
            /* ongoing action */

            switch(current_action) {
                case A_LIFT_GORE:
                    if (current.state != S_STOJI_ZATVOREN &&
                        current.state != S_IDE_GORE) {

                        warnx("Action and state missmatch!");
                    } else if (current.floor == FLOORS - 1) {

                        warnx("Can't go higher!");
                    } else if (current.state != S_IDE_GORE) {

                        current.direction = D_UP;
                        current.state = S_IDE_GORE;
                    }
                    break;
                case A_LIFT_DOLE:
                    if (current.state != S_STOJI_ZATVOREN &&
                        current.state != S_IDE_DOLE) {

                        warnx("Action and state missmatch!");
                    } else if (current.floor == 0) {

                        warnx("Can't go lower!");
                    } else if (current.state != S_IDE_DOLE) {

                        current.direction = D_DOWN;
                        current.state = S_IDE_DOLE;
                    }
                    break;
                case A_LIFT_STANI_NA_KATU:
                    if (current.state != S_IDE_DOLE &&
                        current.state != S_IDE_GORE) {

                        warnx("Action and state missmatch");
                    } else {
                        current_stop = 1;
                    }
                    break;
                case A_LIFT_STANI:
                    current.state = S_STOJI;

                    errx(LIFT_FAILURE, "STOP");
                    break;
                case A_LIFT_OTVORI:
                    if (current.state != S_STOJI_ZATVOREN) {

                        warnx("Action and state missmatch!");
                    } else {
                        current.state = S_OTVARA_VRATA;
                    }
                    break;
                case A_LIFT_ZATVORI:
                    if (current.state != S_STOJI_OTVOREN) {

                        warnx("Action and state missmatch!");
                    } else {
                        current.state = S_ZATVARA_VRATA;
                    }
                    break;
                default:
                    warnx("Unknown action!");
                    break;
            }

            current_action = A_OBRADENO;
        }
        pthread_mutex_unlock(&m_action);
    }

    return NULL;
}

void *udp_listener(void *arg) {
    char lift_hostname[MAX_BUFF], lift_port[MAX_BUFF];
    int server_socket;

    int ack, action, floor;

    struct message_t msg;
    struct sockaddr client;
    socklen_t client_l = sizeof(client);

    ParseHostnameAndPort(MY_NAME, (char **) &lift_hostname, (char **) &lift_port);
#ifdef DEBUG
    printf("Listening as %s on %s:%s\n", MY_NAME, lift_hostname, lift_port);
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

                sscanf(msg.data, "%d", &ack);
#ifdef DEBUG
                printf("Received ACK for message %d\n", ack);
#endif
                ListRemoveById(&datagram_list, ack);

                pthread_mutex_unlock(&m_datagram_list);

                break;
            case LIFT_PALI_LAMPICU:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);

                pthread_mutex_lock(&m_status);

                sscanf(msg.data, "%d", &floor);
                status[floor] = 1;

                pthread_mutex_unlock(&m_status);

                PrintStatus();

                break;
            case LIFT_GASI_LAMPICU:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);

                pthread_mutex_lock(&m_status);

                sscanf(msg.data, "%d", &floor);
                status[floor] = 0;

                pthread_mutex_unlock(&m_status);

                PrintStatus();

                break;
            case LIFT_ACTION_START:
                send_ack(msg.id, upr_socket, &upr_server, upr_server_l);

                sscanf(msg.data, "%d", &action);

                switch (action) {
                    case A_LIFT_STANI:
                    case A_LIFT_GORE:
                    case A_LIFT_DOLE:
                    case A_LIFT_OTVORI:
                    case A_LIFT_ZATVORI:
#ifdef DEBUG
                        printf("Primljena akcija %d!\n", action);
#endif
                        pthread_mutex_lock(&m_action);

                        if (current_action == A_UNDEF ||
                            current_action == A_OBRADENO) {
                            current_action = action;

                            ClockGetTime(&current_action_timeout);
                            ClockAddTimeout(&current_action_timeout, ACTION_TIME);
                        }

                        pthread_mutex_unlock(&m_action);

                        break;
                    case A_LIFT_STANI_NA_KATU:
#ifdef DEBUG
                        printf("Stajem na sljedecem katu!\n");
#endif
                        pthread_mutex_lock(&m_action);
                        current_stop = 1;
                        pthread_mutex_unlock(&m_action);

                        break;

                    case A_LIFT_STATUS:
                        /* posalji status */
                        msg.id = GetNewMessageID();
                        msg.type = LIFT_STATUS_REPORT;
                        msg.lift = MY_ID;

                        pthread_mutex_lock(&m_action);
                        /* current status */
                        memset(msg.data, 0, sizeof(msg.data));
                        to_string(current, msg.data);

                        pthread_mutex_unlock(&m_action);

                        ClockGetTime(&msg.timeout);
                        ClockAddTimeout(&msg.timeout, TIMEOUT);

                        pthread_mutex_lock(&m_datagram_list);

                        sendto(upr_socket, &msg, sizeof(msg), 0, &upr_server, upr_server_l);
                        ListInsert(&datagram_list, msg);

                        pthread_mutex_unlock(&m_datagram_list);

                        break;
                    default:
                        warnx("Unknown lift action!");
                        break;
                }

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
        msg.lift = MY_ID;

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

    /* Initial lift status */
    current_action = A_UNDEF;
    current_stop = 0;

    current = (struct lift_state_t) {
        .floor = 0,
        .im_floor = 0,
        .direction = D_STOJI,
        .door = V_ZATVORENA,
        .state = S_STOJI_ZATVOREN};

    GetMyName(&argc, &argv);
#ifdef DEBUG
    printf("My name is %s\n", MY_NAME);
#endif

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
    pthread_mutex_init(&m_status, NULL);

    /* Ispis stanja */
    PrintStatus();

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
