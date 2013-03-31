/*
This module abstracts the implementation of a classifier algorithm.
It contains the functions and structures needed to learn from a data set and to predict the class of unannotated entries.
See this as an "abstract class". A Classifier per se is useless, but it provides the structure needed by
functions and modules that implements different classifiers.
*/

#ifndef __CLASSIFIER_H__
#define __CLASSIFIER_H__

#ifdef __cplusplus
extern "C" {
#endif
	
	#include "MacLearn/MacLearn.h"
	#include "MacLearn/DataSet/Dataset.h"
	#include "MacLearn/DataSet/BatchDataset.h"
	#include "MacLearn/Inducer/FeatInducer.h"

	/* Classifier methods */
	typedef struct Classifier
	{
		/* Variables */
		PRIVATE unsigned long featsCount;			/* Stores the number of features a valid input must have */
		PRIVATE unsigned long classesCount;			/* Stores the total number of classes recognized by this classifier */
		PRIVATE double alpha;						/* Learning rate. Higher values makes a faster but less accurate learner. (must be > 0 and it's usually <= 1) */
		PRIVATE int inducersCount;					/* Number of inducers used */
		PRIVATE FeatInducer ** inducers;			/* Feature Inducers used to generate new features in order to learn from non-linear data sets */

		/* Functions */
		/*	Learns from a BatchDataset. Will perform up to "maxIterations" on the data set (it stops early if it hits 100% correctly predicted outputs).
			If trainErrors is not null, it stores the number of errors on the last iteration of the data set.
			If confMatrix is not null, it returns the "confusion matrix" of the last iteration of the data set.
		*/
		PUBLIC int (*batchLearn)(struct Classifier * classifier, BatchDataSet * dataset, 
								 unsigned long maxIterations, unsigned long * trainErrors, 
								 unsigned long * confMatrix);		/* Read above */
		/*	Do a single step on the learning algorythm, using the provided entry. 
			If predictedClass is not NULL, stores the class the classifier predicted for this entry. 
		*/
		PUBLIC int (*stepLearn)(struct Classifier * classifier, EntryData * entry, unsigned long * predictedClass, unsigned long * confMatrix);		
		/*	Runs through a DataSet (until NextEntry returns that the DataSet ended) prediciting and verifying the results to test the accuracy of the Classifier 
			If trainErrors is not null, it stores the number of errors on the last iteration of the data set.
			If confMatrix is not null, it returns the "confusion matrix" of the last iteration of the data set.
		*/
		PUBLIC int (*test)(struct Classifier * classifier, DataSet * dataset, unsigned long * testErrors, unsigned long * confMatrix);		/* Read above */
		/*	Predict the class of a single entry. The main differences between this function and "stepLearn" are that this one 
		doesn't look into entry->class and does NOT update the classifier internal structures. 
		*/
		PUBLIC int (*predict)(struct Classifier * classifier, EntryData * entry, unsigned long * predictedClass);
		/* Save the learned classifier in dstPath. File layout depends on the type of the classifier */
		PUBLIC int (*save)(struct Classifier * classifier, char * dstPath);
		/* Set a group of Inducers to be used with this classifier */
		PUBLIC int (*setInducers)(FeatInducer * inducers, int inducersCount);
		/* Free all pointers allocated by this classifier */
		PUBLIC void (*free)(struct Classifier * classifier);					
	}Classifier;

#ifdef EXTEND_CLASSIFIER
	/************************
	* "Protected" Functions	*
	************************/

	/*	Initializes the struct's variables and function pointers. 
	NOTE: This function does NOT allocate memory for a Classifier struct. */
	PROTECTED void Classifier_Init (Classifier * classifier);
#endif

#ifdef __cplusplus
}
#endif

#endif