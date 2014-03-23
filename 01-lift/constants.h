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

/*
 * Other constatns
 */
#define UP              0
#define DOWN            1
#define FLOORS          10
#define MAX_BUFF        256
#define LIFT_FREQUENCY  1000
#define LIST_FREQUENCY  1000000
#define TIMEOUT         500000000


/*
 * Message types
 */
enum {POTVRDA,
    TIPKE_KEY_PRESSED_UP, TIPKE_KEY_PRESSED_DOWN,
    TIPKE_PALI_LAMPICU_UP, TIPKE_PALI_LAMPICU_DOWN,
    TIPKE_GASI_LAMPICU_UP, TIPKE_GASI_LAMPICU_DOWN,
    LIFT_STOP, LIFT_KEY_PRESSED,
    LIFT_ACTION_FINISHED};

/*
 * Lift actions
 */
enum {A_LIFT_GORE, A_LIFT_DOLE,
    A_LIFT_STANI_NA_KATU, A_LIFT_STANI,
    A_OTVORI, A_ZATVORI,
    A_LIFT_STATUS};

/*
 * Lift states
 */
enum {S_STOJI_OTVOREN, S_STOJI_ZATVOREN,
    S_OTVARA_VRATA, S_ZATVARA_VRATA,
    S_IDE_DOLJE, S_IDE_GORE,
    S_STOJI};

#endif
