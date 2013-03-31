/*
	"Extends" Dataset (in a sense).
	This layer abstracts implementations of data sets for "batch" learning, 
	in a sense that the quantity and value of records are known and accessible
	at any order. It's not required that all data resides in memory at the same
	time, which allows for reading data sets larger than the system memory.
*/

#ifndef __BATCHDATASET_H__
#define __BATCHDATASET_H__

#ifdef __cplusplus
extern "C" {
#endif
	
	#include <stdio.h>			/* For FILE */
	#include "MacLearn/MacLearn.h"
	#include "Dataset.h"		/* For DataSet definitions */

	/* Modes for reading a dataset */
	typedef enum{
		BD_RM_FULL = 1,
		BD_RM_INCREMENTAL
	}BatchReadMode;

	/* The structure representation of a Batch Dataset */
	typedef struct BatchDataSet
	{
		DataSet;										/* Holds all dataset vars and functions */
		/* Declare batch specific vars */
		PUBLIC unsigned long entriesCount;				/* Size (in records) of the Dataset */
		PUBLIC BatchReadMode readMode;					/* Dataset read mode - BD_RM_FULL to read all records to memory, BD_RM_INCREMENTAL to read one record at a time */
		PROTECTED EntryData * entries;					/* Buffer where the records are stored. Shouldn't be accessed directly by external calls. Use NextEntry() */
		PROTECTED off_t * readOrder;					/* Buffer to store the order in which the records are read. */
		PROTECTED unsigned long currentPos;				/* Holds the current position on the "readOrder" vector. */
		PROTECTED FILE * file;							/* Pointer to the file that stores the data - used only on INCREMENTAL datasets */
		/* Declare specific functions*/
		PUBLIC int (*reset)(struct BatchDataSet * batchDataset);					/* Reset the dataset back to the first record */
		PUBLIC int (*shuffle)(struct BatchDataSet * batchDataset);					/* Shuffle the records in the dataset */
		PUBLIC int (*sort)(struct BatchDataSet * batchDataset);						/* Sort the records back to the original order */
		PROTECTED int (*load)(struct BatchDataSet * batchDataset, char * srcPath);	/* Load a dataset from "srcPath" - should be called only once, during instance creation */
	}BatchDataSet;

#ifdef EXTEND_BATCHDATASET
	/************************
	* "Protected" Functions	*
	************************/

	/*	Initializes the struct's variables and function pointers. 
	NOTE: This function does NOT allocate memory for a BatchDataSet struct. */
	PROTECTED void BatchDataSet_Init (BatchDataSet * batchDataset);
#endif

#ifdef __cplusplus
}
#endif

#endif