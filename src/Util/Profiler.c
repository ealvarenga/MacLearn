#include <stdio.h>
#include <time.h>
#include <string.h>
#include "MacLearn/Util/Profiler.h"

/* If running on Windows, include ProfilerWin.h to fill missing functions and definitions */
#ifdef WIN32
	#include "MacLearn/Util/ProfilerWin.h"
#endif

/* Define a maximum value for the label of a Profiler. Although this creates a fixed limitation (which can be easily expanded if needed), it avoids aditional calls to malloc/free */
#define MAX_LABEL_SIZE			64		/* Labels shouldn't even be that long, but if they are over 63 chars, they'll be truncated */
#define MAX_PROFILER_ENTRIES	20		/* Maximum number of active profiling timers */

/* Define a structure to store Profiler data */
typedef struct  
{
	char label[MAX_LABEL_SIZE];		/* Entry Label */
	unsigned long executionCount;
	struct timespec totalTime;
	struct timespec lastStartTime;
}ProfilerEntry;

/* TODO: This should be changed to a hashmap. It's more elegant and doesn't require a fixed maximum number of entries */
static ProfilerEntry entries[MAX_PROFILER_ENTRIES];
/* Next free entry to be used */
static int nextEntry = 0;

static struct timespec TimeSpec_Diff(struct timespec * start, struct timespec * end)
{
	struct timespec aux;

	/* Subtract seconds and nanoseconds (end-start) */
	aux.tv_sec = end->tv_sec - start->tv_sec;
	aux.tv_nsec = end->tv_nsec - start->tv_nsec;

	/* If nanoseconds fell into negative values, fix this by removing one second and adding 10^9 nanoseconds */
	if (aux.tv_nsec<0)
	{
		aux.tv_sec -= 1;
		aux.tv_nsec += 1000000000;
	}

	return aux;
}

static struct timespec TimeSpec_Add(struct timespec * a, struct timespec * b)
{
	struct timespec aux;

	/* Add seconds and nanoseconds (a+b) */
	aux.tv_sec = a->tv_sec + b->tv_sec;
	aux.tv_nsec = a->tv_nsec + b->tv_nsec;

	/* If there are more than 10^9 nanoseconds, fix this by adding 1 to seconds and subtracting the excessive nanoseconds */
	if (aux.tv_nsec >= 1000000000)
	{
		aux.tv_sec += 1;
		aux.tv_nsec -= 1000000000;
	}

	return aux;
}

void Profiler_Reset_i (void)
{
	/* Reset all data to zero */
	memset (entries, 0, sizeof(entries));
	nextEntry = 0;
}

void Profiler_Start_i (char * entryLabel)
{
	int i;

	/* Search all active */
	for (i=0;i<nextEntry;i++)
		if (strncmp(entryLabel, entries[i].label, MAX_LABEL_SIZE) == 0)
			break;

	/* If all positions are in use, return without doing anything */
	if (i == MAX_PROFILER_ENTRIES)
		return;

	/* If using a new entry, increment the next entry position and clear all data */
	if (i == nextEntry)
	{
		/* Zero current data */
		memset (&entries[i],0,sizeof(ProfilerEntry));
		/* Save the label */
		strncpy(entries[i].label, entryLabel, MAX_LABEL_SIZE);
		/* Increment the next free position */
		nextEntry++;
	}


	clock_gettime(CLOCK_REALTIME, &entries[i].lastStartTime);

	/* all done */
}

void Profiler_Stop_i (char * entryLabel)
{
	int i;
	struct timespec aux;

	/* Search all active */
	for (i=0;i<nextEntry;i++)
		if (strncmp(entryLabel, entries[i].label, MAX_LABEL_SIZE) == 0)
			break;

	/* If not found, return without doing anything */
	if (i == nextEntry)
		return;

	/* Get current time to calc total elapsed time */
	clock_gettime(CLOCK_REALTIME, &aux);

	/* Calc de elapsed time */
	aux = TimeSpec_Diff(&entries[i].lastStartTime, &aux);

	/* Update total time */
	entries[i].totalTime = TimeSpec_Add(&entries[i].totalTime, &aux);

	/* Update execution count */
	entries[i].executionCount ++;
}

void Profiler_PrintTable_i (FILE * out)
{
	int i;
	double individualTime;

	/* Print a header */
	fprintf (out, "%-20s%10s%20s%20s\n","Label", "ExecCount", "TimePerExec", "TotalTime");
	
	/* Print entries data */  
	for (i=0;i<nextEntry;i++)
	{
		/* Skip entries that haven't completed at least one cycle */
		if (entries[i].executionCount == 0)
			continue;

		/* Calc totalTime in uS */
		individualTime = ((double) entries[i].totalTime.tv_sec)/entries[i].executionCount + ((double) entries[i].totalTime.tv_nsec)/entries[i].executionCount/1000000000;
		fprintf (out, "%-20s%10lu% 20lf% 13ld.%06ld\n", entries[i].label, entries[i].executionCount, individualTime, entries[i].totalTime.tv_sec, entries[i].totalTime.tv_nsec/1000);
	}

}
