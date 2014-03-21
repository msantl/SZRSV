/*
 * Functions that implement 'list' data structure
 */
#ifndef __LIST_H
#define __LIST_H

#include <time.h>

#include "message.h"

/*
 * List element data structure
 */
struct list_t {
    struct message_t m;

    struct list_t *next;
};

/*
 * InitListHead     - initializes list head
 */
void InitListHead(struct list_t **);

/*
 * ListInsert       - inserts into list
 */
void ListInsert(struct list_t **, struct message_t);

/*
 * ListRemoveById   - removes element by its id
 */
void ListRemoveById(struct list_t **, int);

/*
 * ListRemoveByTimeout  - removes elements by their timeout
 */
void ListRemoveByTimeout(struct list_t **, struct timespec);

#endif
