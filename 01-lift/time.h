/*
 * MAC OS x doesn't have clock_gettime
 */
#ifndef __TIME_H
#define __TIME_H

#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

void ClockGetTime(struct timespec*);

#endif
