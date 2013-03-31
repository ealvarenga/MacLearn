/*
"Extends" FeatInducer (in a sense).
This module implements the functions needed to read a file containing boolean rules to generate new features.

The file must have one record per line, formatted as follows:
<lvl>#F<feature><op><refVal>[&G<genFeat>]
where:
<lvl> is the level of the feature on the generating tree
<feature> is the feature number that must be tested. Remember that 0 stands for the bias (always 1) and the first real feature is 1.
<op> is the boolean operation to be tested, i.e. =, !=, >, <, >= or <=.
<refVal> is the value to be used on the test (this is a "double" value)
[%G<genFeat>] is optional. <genFeat> is the number of a generated feature to be considered along with the rule (genFeat is a zero based array). An "and" is performed over first rule and genFeat.

Ex:

0#F1>10
1#F2==3.2&G0
1#F3<=0.00&G2

*/

#ifndef __BOOLEANINDUCER_H__
#define __BOOLEANINDUCER_H__

#ifdef __cplusplus
extern "C" {
#endif

	#include "MacLearn/MacLearn.h"
	#include "FeatInducer.h"		/* For FeatInducer definitions */

	#define BOOLEANINDUCER_ID		"BoolInducer"			/* Used to identify inducer file data */

	/* Types of valid boolean operations */
	typedef enum
	{
		BI_OP_EQ,		/* Equal "==" */
		BI_OP_LT,		/* Less than "<" */
		BI_OP_GT,		/* Greater than ">" */
		BI_OP_NE,		/* Not Equal "!=" */
		BI_OP_LE,		/* Less than or equal to "<=" */
		BI_OP_GE		/* Greater than or equal to ">=" */
	}BoolOperator;


	/* This structures stores a rule to generate a feature based on a boolean operation */
	typedef struct 
	{
		int lvl;
		unsigned long feat1;
		unsigned long feat2;
		double referenceValue;
		BoolOperator op;
	}BooleanRule;

	/* The structure representation of a BooleanInducer */
	typedef struct BooleanInducer
	{
		FeatInducer;				/* Holds all feature inducer vars and functions */
		/* Declare boolean specific vars */
		PRIVATE BooleanRule * booleanRules;		/* Rules to generate new features using boolean operations */
		/* Declare specific functions*/
	}BooleanInducer;

#ifdef EXTEND_BOOLEANINDUCER
	/************************
	* "Protected" Functions	*
	************************/

	/*	Initializes the struct's variables and function pointers. 
	NOTE: This function does NOT allocate memory for a BooleanInducer struct. */
	PROTECTED void BooleanInducer_Init (BooleanInducer * boolInducer);
#endif

	/* Return a new BooleanInducer instance, or NULL on error */
	PUBLIC BooleanInducer * BooleanInducer_New (char * path, unsigned long maxLvl);

	/* Read the data from an already open stream and return a new instance of BooleanInducer */
	PUBLIC BooleanInducer * BooleanInducer_ReadData (FILE * in);

#ifdef __cplusplus
}
#endif

#endif