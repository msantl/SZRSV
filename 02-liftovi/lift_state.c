#include <stdio.h>

#include "lift_state.h"

void from_string(char *src, struct lift_state_t *dst) {
    sscanf(src, "%d-%d-%d-%d-%d",
            &(dst->floor),
            &(dst->im_floor),
            &(dst->direction),
            &(dst->door),
            &(dst->state));
    return;
}

void to_string(struct lift_state_t src, char *dst) {
    sprintf(dst, "%d-%d-%d-%d-%d",
            src.floor,
            src.im_floor,
            src.direction,
            src.door,
            src.state);
    return;
}
