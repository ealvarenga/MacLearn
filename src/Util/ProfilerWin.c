#include <windows.h>
#include "MacLearn/Util/ProfilerWin.h"

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	LARGE_INTEGER freq = {0};
	LARGE_INTEGER curVal = {0};
	double aux;

	/* Get the frequency of the meter */
	QueryPerformanceFrequency(&freq);

	/* Get the current value of the meter */
	QueryPerformanceCounter(&curVal);

	/* Remap the value into timespec structure */
	aux = ((double) curVal.QuadPart) / freq.QuadPart;
	tp->tv_sec = (time_t) aux;
	tp->tv_nsec = (long) ((__int64) (aux * 1000000000) % 1000000000);

	return 0;
}