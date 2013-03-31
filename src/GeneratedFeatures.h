#ifndef __GENERATEDFEATURES_H__
#define __GENERATEDFEATURES_H__

#include "Dataset.h"

typedef enum
{
	GF_OP_EQ,		/* Equal "==" */
	GF_OP_LT,		/* Less than "<" */
	GF_OP_GT,		/* Greater than ">" */
	GF_OP_NE,		/* Not Equal "!=" */
	GF_OP_LE,		/* Less than or equal to "<=" */
	GF_OP_GE		/* Greater than or equal to ">=" */
}BoolOperator;

typedef struct  
{
	int lvl;
	unsigned long feat1;
	unsigned long feat2;
	FeatureNorm norm;
}GeneratedFeature;

typedef struct 
{
	int lvl;
	unsigned long feat1;
	unsigned long feat2;
	double referenceValue;
	BoolOperator op;
}GeneratedBoolFeature;

typedef struct  
{
	GeneratedFeature * genFeats;
	unsigned long genFeatsCount;
	GeneratedBoolFeature * genBoolFeats;
	unsigned long genBoolFeatsCount;
}GeneratedFeatureList;

int GeneratedFeatures_Load (char * path, char * boolPath, GeneratedFeatureList * genFeatsList, int maxLvl, int maxBoolLvl, unsigned long baseFeats);

#endif