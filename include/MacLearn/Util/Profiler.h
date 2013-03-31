/*
This module provides a simple way to measure the time taken to perform individual tasks

Since this is (usually) a debug time utility, the PROFILING macro must be defined for profilers to work.
If such macro isn't defined, all Profiler_* macros are evaluated to "nothing" by the pre-compiler, thus avoiding 
wasting time during execution time.
*/


#ifndef __PROFILER_H__
#define __PROFILER_H__

/* If compiled without profilers, define empty macros to avoid wasting time calling empty functions */
#ifndef PROFILING
	/* Free's all internally stored profilers, reseting their values and freeing memory */
	#define Profiler_Reset() 
	/* Start the counter of a profiler with "entryLabel". If the profiler was already running, updates the start time (effectively ignoring the previous call) */
	#define Profiler_Start(entryLabel)
	/* Stop the profiler "entryLabel". The running time since Profiler_Start will be added to this profiler counter, and the number of executions will be incremented */
	#define Profiler_Stop(entryLabel)
	/* Prints the current table of profilers on the "out" File *. This function does not free profiler's memory, call Profiler_Reset() for that */
	#define Profiler_PrintTable(out)
#else
	/*	Profiler's must be used. Declare the macros pointing to valid profiler functions. These functions must be implemented on a separate module, which can be context
		specific (for instance, you can have two different implementations, for Linux and Windows).
	*/

	/* Free's all internally stored profilers, reseting their values and freeing memory */
	#define Profiler_Reset()				Profiler_Reset_i();
	/* Start the counter of a profiler with "entryLabel". If the profiler was already running, updates the start time (effectively ignoring the previous call) */
	#define Profiler_Start(entryLabel)		Profiler_Start_i(entryLabel);
	/* Stop the profiler "entryLabel". The running time since Profiler_Start will be added to this profiler counter, and the number of executions will be incremented */
	#define Profiler_Stop(entryLabel)		Profiler_Stop_i(entryLabel);
	/* Prints the current table of profilers on the "out" File *. This function does not free profiler's memory, call Profiler_Reset() for that */
	#define Profiler_PrintTable(out)		Profiler_PrintTable_i(out);

	/* All functions return void, since caller can't expect and test a return when PROFILING is undefined. 
		Any internal error should be reported on stderr.
	*/
	void Profiler_Reset_i (void);
	void Profiler_Start_i (char * entryLabel);
	void Profiler_Stop_i (char * entryLabel);
	void Profiler_PrintTable_i (FILE * out);

#endif

#endif