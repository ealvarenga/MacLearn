/*
"Extends" CSVDataset (in a sense).
This module implements the functions needed to read an ARFF file into a data set abstraction.
*/

#ifndef __ARFFDATASET_H__
#define __ARFFDATASET_H__

#ifdef __cplusplus
extern "C" {
#endif

	#include "MacLearn/MacLearn.h"
	#include "CSVDataset.h"		/* For CSVDataSet definitions */

	/* The structure representation of an Arff Dataset */
	typedef struct ArffDataSet
	{
		CSVDataSet;					/* Holds all csv dataset vars and functions */
		/* Don't need any specific vars or functions */
	}ArffDataSet;

#ifdef EXTEND_ARFFDATASET
	/************************
	* "Protected" Functions	*
	************************/

	/*	Initializes the struct's variables and function pointers. 
	NOTE: This function does NOT allocate memory for a ArffDataSet struct. */
	PROTECTED void ArffDataSet_Init (ArffDataSet * arffDataset);
#endif

	/*	Returns a new Arff dataset instance, or NULL on error */
	PUBLIC ArffDataSet * ArffDataSet_New (BatchReadMode readMode, char * srcPath);

	/* Write a dataset to an ARFF formatted file */
	PUBLIC int ArffDataSet_Save (DataSet * dataset, char * outPath);

#ifdef __cplusplus
}
#endif

#endif