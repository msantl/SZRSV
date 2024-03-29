#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "list.h"
#include "constants.h"

void InitListHead(struct list_t **head) {
    *head = NULL;
    return;
}

void ListInsert(struct list_t **head, struct message_t m) {
    struct list_t *elem = (struct list_t *) malloc(sizeof(struct list_t));

    (*elem).m = m;

    if (!head) {
        (*elem).next = NULL;
    } else {
        (*elem).next = *head;
    }

    *head = elem;
    return;
}

void ListRemoveById(struct list_t **head, int id) {
    struct list_t *curr, *last = NULL;

    for (curr = *head; curr; curr = (*curr).next) {
        if ((*curr).m.id == id) {
            if (last) {
                (*last).next = (*curr).next;
            } else {
                *head = (*curr).next;
            }

            free(curr);
            break;
        }

        last = curr;
    }

    return;
}

void ListRemoveByTimeout(struct list_t **head, struct timespec timeout) {
    struct list_t *curr, *last = NULL, *temp;

    for (curr = *head; curr;) {
        if ((*curr).m.timeout.tv_sec < timeout.tv_sec ||
           ((*curr).m.timeout.tv_sec == timeout.tv_sec &&
            (*curr).m.timeout.tv_nsec < timeout.tv_nsec)) {

#ifdef DEBUG
            warnx("Datagram %d expired", (*curr).m.id);
#endif

            if (last) {
                (*last).next = (*curr).next;
            } else {
                *head = (*curr).next;
            }

            temp = curr;
            curr = (*curr).next;
            free(temp);
        } else {
            last = curr;
            curr = (*curr).next;
        }
    }

    return;
}

void ListDelete(struct list_t **head) {
    struct list_t *curr, *temp;

    for (curr = *head; curr;) {
        temp = curr;
        curr = (*curr).next;

        free(temp);
    }

    *head = NULL;
    return;
}
