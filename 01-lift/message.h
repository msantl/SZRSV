/*
 * Message types data structures
 */

#ifndef __MESSAGE_H
#define __MESSAGE_H

#include "time.h"

struct message_t {
    int id;
    struct timespec timeout;

    /* TODO: add message type and stuff */
};

#endif
