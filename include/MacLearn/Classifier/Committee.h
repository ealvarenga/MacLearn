/*
This module implements a Committee of Classifiers.
It also extends a Classifier, so you can create a committee of committees if needed.

A committee is a classifier that executes many classifiers and return the most "voted" class as its classification.

NOTE: To reduce the coding effort needed, a committee can't be saved and do not clone the classifiers internally. This means that a classifier that is part of a committee
can't be freed until the committee is freed.

TODO: Clone the classifiers internally to avoid possible errors
*/
#ifndef __COMMITTEE_H__
#define __COMMITTEE_H__


#ifdef __cplusplus
extern "C" {
#endif

	#include <stdio.h>

	#include "MacLearn/MacLearn.h"
	#include "MacLearn/Classifier/Classifier.h"
	#include "MacLearn/DataSet/Dataset.h"

	/* Committee structure */
	typedef struct Committee
	{
		Classifier;
		/* Specific Vars */
		PRIVATE Classifier ** classifiers;		/* The classifiers that form this committee */
		PRIVATE int classifiersCount;			/* Number of classifiers on this committee */
		PRIVATE int * predictionVotes;			/* Array that stores the number of votes each class has during a prediction */
		/* Doesn't need any specific function */
	}Committee;

#ifdef EXTEND_COMMITTEE
	/************************
	* "Protected" Functions	*
	************************/

	/*	Initializes the struct's variables and function pointers. 
	NOTE: This function does NOT allocate memory for a Committee struct. */
	PROTECTED void Committee_Init (Committee * comt);
#endif

	/* Returns a new committee instance, or NULL on error. */
	PUBLIC Committee * Committee_New (int classifiersCount, Classifier ** classifiers);


#ifdef __cplusplus
}
#endif


#endif