#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "set.h"
#include "constants.h"

void InitSetHead(struct set_t **head) {
    *head = NULL;
    return;
}

void SetInsert(struct set_t **head, int floor, int direction) {
    struct set_t *elem;

    if (*head == NULL) {
        elem = (struct set_t *) malloc(sizeof(struct set_t));

        (*elem).floor = floor;
        (*elem).direction = direction;

        (*elem).next = NULL;

        *head = elem;
    } else if ((*head)->floor == floor && (*head)->direction == direction) {
        warnx("Could not insert into set!");
    } else if ((*head)->next == NULL) {
        elem = (struct set_t *) malloc(sizeof(struct set_t));

        (*elem).floor = floor;
        (*elem).direction = direction;

        (*elem).next = NULL;

        (*head)->next = elem;
    } else {
        SetInsert(&(*head)->next, floor, direction);
    }

    return;
}

void SetRemove(struct set_t **head, int floor, int direction) {
    struct set_t *prev = NULL, *curr = *head;

    if (curr == NULL) {
        /* set is empty */
    } else if (curr->floor == floor && curr->direction == direction) {
        /* removing first element */
        *head = (*head)->next;
        free(curr);
    } else {
        while (curr != NULL) {
            if (curr->floor == floor && curr->direction == direction) {
                prev->next = curr->next;
                free(curr);
                break;
            }
            prev = curr;
            curr = curr->next;
        }
    }

    return;
}

void SetDelete(struct set_t **head) {
    struct set_t *curr, *temp;

    for (curr = *head; curr;) {
        temp = curr;
        curr = (*curr).next;

        free(temp);
    }

    *head = NULL;
    return;
}

void SetSort(struct set_t **head, int floor, int direction) {
    struct set_t *ret = NULL;
    struct set_t *curr = *head, *best;

    int curr_dist, best_dist;

    if (direction == D_STOJI) {
        /* FIFO */
        return;
    }

    while (*head != NULL) {
        curr = best = *head;
        best_dist = 0x7fffffff;

        while (curr != NULL) {

            if (direction == D_UP) {
                if (curr->direction == D_UP && floor <= curr->floor) {
                    curr_dist = curr->floor - floor;
                } else {
                    curr_dist = 2 * FLOORS - floor - curr->floor;
                }
            } else {
                if (curr->direction == D_DOWN && floor >= curr->floor) {
                    curr_dist = floor - curr->floor;
                } else {
                    curr_dist = floor + curr->floor;
                }
            }

            if (curr_dist < best_dist) {
                best_dist = curr_dist;
                best = curr;
            }

            curr = curr->next;
        }

        SetInsert(&ret, best->floor, best->direction);
        SetRemove(head, best->floor, best->direction);
    }

    *head = ret;

    return;
}
