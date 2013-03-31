/*
"Extends" BatchDataset (in a sense).
This module implements the functions needed to read a CSV file into a data set abstraction.
The file may or may not have a header with column names.
*/

#ifndef __CSVDATASET_H__
#define __CSVDATASET_H__

#ifdef __cplusplus
extern "C" {
#endif

	#include "MacLearn/MacLearn.h"
	#include "BatchDataset.h"		/* For BatchDataSet definitions */

	/* The structure representation of a CSV Dataset */
	typedef struct CSVDataSet
	{
		BatchDataSet;				/* Holds all batch dataset vars and functions */
		/* Declare batch specific vars */
		PUBLIC unsigned char hasLabels;				/* Determines if the original file had column labels */
		PUBLIC char delimiter;							/* Character that represents the separation between records. Default vaule is ',' */
		/* Declare specific functions*/
		PROTECTED int (*loadHeader) (struct CSVDataSet * csvDataset);	/* Load header info (Column names, column count and class count) from dataset - used internally, treat as "protected" */
		PROTECTED int (*loadData) (struct CSVDataSet * csvDataset);		/* Prepare/Load the data from a dataset file - used internally, treat as "protected" */
	}CSVDataSet;

#ifdef EXTEND_CSVDATASET
	/************************
	* "Protected" Functions	*
	************************/

	/* Reads on line of the CSV file into entry. The file must be positioned on the begining of the record line (no check is made to verify that this is true!) */
	PROTECTED int CSVDataSet_ReadLine (CSVDataSet * csvDataset, EntryData * entry);

	/*	Initializes the struct's variables and function pointers. 
	NOTE: This function does NOT allocate memory for a BatchDataSet struct. */
	PROTECTED void CSVDataSet_Init (CSVDataSet * csvDataset);
#endif

	/* Returns a new CSV data set instance, or NULL on error */
	PUBLIC CSVDataSet * CSVDataSet_New (unsigned char hasLabels, char delimiter, BatchReadMode readMode, char * srcPath);

	/* Write a dataset to a CSV formatted file */
	PUBLIC int CSVDataSet_Save (DataSet * dataset, unsigned char hasLabels, char delimiter, char * outPath);


#ifdef __cplusplus
}
#endif

#endif