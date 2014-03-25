/*
 * Functions that implement 'set' data structure
 */
#ifndef __set_H
#define __set_H

/*
 * set element data structure
 */
struct set_t {
    int floor, direction;

    struct set_t *next;
};

/*
 * InitSetHead     - initializes set head
 */
void InitSetHead(struct set_t **);

/*
 * SetInsert       - inserts into set
 */
void SetInsert(struct set_t **, int, int);

/*
 * SetRemove    - removes element
 */
void SetRemove(struct set_t **, int, int);

/*
 * SetDelete   - deletes all elements from set
 */
void SetDelete(struct set_t **);

/*
 * SetSort      - sorts elements of set
 */
void SetSort(struct set_t **, int, int);

#endif
