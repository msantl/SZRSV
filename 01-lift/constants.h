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
#define MAX_BUFF        256
#define FREQUENCY       1000000
#define TIMEOUT         500000000


/*
 * Message types
 */
enum {POTVRDA, TIPKE_KEY_PRESSED_UP, TIPKE_KEY_PRESSED_DOWN,
    TIPKE_PALI_LAMPICU_UP, TIPKE_PALI_LAMPICU_DOWN,
    TIPKE_GASI_LAMPICU_UP, TIPKE_GASI_LAMPICU_DOWN};

#endif
