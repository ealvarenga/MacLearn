/*
	This module abstracts the notion of a data set.
	It contains the functions and structures needed to read any data set, no matter what kind or size.
	See this as an "abstract class". A DataSet per se is useless, but it provides the structure needed by
	functions and modules that need to read a data set.

	TODO: Save "column" names to use when converting datasets
	TODO: Currently only numeric data sets are supported.
*/

#ifndef __DATASET_H__
#define __DATASET_H__

#ifdef __cplusplus
extern "C" {
#endif
	#include <string.h>		/* For memset */

	#include "MacLearn/MacLearn.h"

	/* Struct that represents one record of a Data Set */
	typedef struct
	{
		double * features;
		int class;
	}EntryData;

	/* Data Set representation */
	typedef struct DataSet
	{
		/* Variables */
		PUBLIC unsigned long featsCount;											/* Number of features in a record */
		PUBLIC unsigned long classesCount;											/* Total number of classes in the Data Set */
		/* Functions */
		/*	Returns a pointer to the next entry to be used. The pointer must not be freed by the calling function.  
			This functions returns a pointer instead of copying the data to achieve better performance. It's unsafe to
			assume anything about the returned pointer's position in memory or its neibourghs. */
		PUBLIC int (*nextEntry)(struct DataSet * dataset, EntryData ** entry);		/* Read above. */
		PUBLIC void (*free)(struct DataSet * dataset);					/* Free all pointers allocated by this data set */
	}DataSet;

#ifdef EXTEND_DATASET
	/************************
	* "Protected" Functions	*
	************************/

	/*	Initializes the struct's variables and function pointers. 
		NOTE: This function does NOT allocate memory for a DataSet struct. */
	PROTECTED void DataSet_Init (DataSet * dataset);
#endif

#ifdef __cplusplus
}
#endif

#endif