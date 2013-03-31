/* Provides the functions and definitions for clock_gettime that are missing in Windows */
#ifndef __PROFILERWIN_H__
#define __PROFILERWIN_H__

#include <time.h>	/* For time_t */

struct timespec {
	time_t tv_sec; /* seconds */
	long tv_nsec; /* nanoseconds */
};

/* Define a type for clockid_t */
typedef int clockid_t;

#define CLOCK_REALTIME			0		/* The value is ignored in this implementation */

int clock_gettime(clockid_t clk_id, struct timespec *tp);


#endif