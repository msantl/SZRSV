/*
 * Message types data structures
 */

#ifndef __MESSAGE_H
#define __MESSAGE_H

#include "time.h"
#include "constants.h"

struct message_t {
    int id;
    struct timespec timeout;

    int type;
    int lift;
    char data[MAX_BUFF];
};

#endif
