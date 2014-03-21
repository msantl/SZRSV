/*
 * Message types data structures
 */

#ifndef __MESSAGE_H
#define __MESSAGE_H

#include <time.h>

typedef struct _message_t {
    int id;
    struct timespec timeout;

    /* TODO: add message type and stuff */

} message_t;

#endif
