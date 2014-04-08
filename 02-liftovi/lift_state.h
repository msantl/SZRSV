#ifndef __LIFT_STATE
#define __LIFT_STATE

#include "constants.h"
#include "time.h"

struct lift_state_t {
    int floor;
    int im_floor;
    int direction;
    int door;

    int state;

    int stani[FLOORS];
    struct timespec cekaj_do;
};

void from_string(char *, struct lift_state_t *);

void to_string(struct lift_state_t, char *);

#endif
