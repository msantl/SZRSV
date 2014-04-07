/*
 * Constants used in this project
 */
#ifndef __CONSTANTS_H
#define __CONSTANTS_H

/*
 * Configuration file
 */
#define CONFIGURATION   "lift.cfg"

/*
 * Error codes
 */
#define UDP_FAILURE     1
#define PARSE_FAILURE   2
#define LIST_FAILURE    3
#define THREAD_FAILURE  4
#define USER_FAILURE    5
#define LIFT_FAILURE    6

/*
 * Other constatns
 */
#define FLOORS          10
#define MAX_BUFF        256
#define UPR_FREQUENCY   1000000
#define LIFT_FREQUENCY  5000000
#define LIST_FREQUENCY  1000000
#define ACTION_TIME     300000000
#define TIMEOUT         500000000

/*
 * Message types
 */
enum {POTVRDA,
    TIPKE_KEY_PRESSED_UP, TIPKE_KEY_PRESSED_DOWN,
    TIPKE_PALI_LAMPICU_UP, TIPKE_PALI_LAMPICU_DOWN,
    TIPKE_GASI_LAMPICU_UP, TIPKE_GASI_LAMPICU_DOWN,
    TIPKE_STATUS_REQUEST, TIPKE_STATUS_REPORT,
    LIFT_STOP, LIFT_KEY_PRESSED,
    LIFT_ACTION_FINISH, LIFT_ACTION_START,
    LIFT_PALI_LAMPICU, LIFT_GASI_LAMPICU,
    LIFT_STATUS_REPORT};

/*
 * Lift actions
 */
enum {A_UNDEF, A_OBRADENO,
    A_LIFT_GORE, A_LIFT_DOLE,
    A_LIFT_STANI_NA_KATU, A_LIFT_STANI,
    A_LIFT_OTVORI, A_LIFT_ZATVORI,
    A_LIFT_STATUS};

/*
 * Lift states
 */
enum {S_STOJI_OTVOREN, S_STOJI_ZATVOREN,
    S_OTVARA_VRATA, S_ZATVARA_VRATA,
    S_IDE_DOLE, S_IDE_GORE,
    S_STOJI};

/*
 * Direction
 */
enum {D_UP, D_DOWN, D_STOJI};

#endif
