/*
This module implements a Multi Class Perceptron.
Not to be mistaken for a Multi Layer Perceptron or Neural Network. This module implements a single perceptron that
relies on feature inducing techniques to ensure linear separability.
*/
#ifndef __PERCEPTRON_H__
#define __PERCEPTRON_H__


#ifdef __cplusplus
extern "C" {
#endif

	#include "MacLearn/MacLearn.h"
	#include "MacLearn/Classifier/Classifier.h"
	#include "MacLearn/DataSet/Dataset.h"

	/* Perceptron structure */
	typedef struct Perceptron
	{
		Classifier;
		/* Specific Vars */
		PRIVATE unsigned char largeMargin;		/* Determines if should use "Large Margin" mechanisms when training the Perceptron */
		PRIVATE unsigned long WColumns;			/* Holds the number of columns in the weights matrix */
		PRIVATE double * W;						/* Holds the "weights" matrix */
		/* Doesn't need any specific function */
	}Perceptron;

#ifdef EXTEND_PERCEPTRON
	/************************
	* "Protected" Functions	*
	************************/

	/*	Initializes the struct's variables and function pointers. 
	NOTE: This function does NOT allocate memory for a Perceptron struct. */
	PROTECTED void Perceptron_Init (Perceptron * pcpt);
#endif

	/* Returns a new perceptron instance, or NULL on error. */
	PUBLIC Perceptron * Perceptron_New (DataSet * dataset, unsigned char largeMargin, double alpha, int inducersCount, FeatInducer ** inducers);

	/* Load a perceptron from a file and returns a new instance (or NULL) */
	PUBLIC Perceptron * Perceptron_Load(char * srcPath);


#ifdef __cplusplus
}
#endif


#endif