#define EXTEND_CLASSIFIER
#include <stdlib.h>				/* For malloc/free */

#include "MacLearn/Classifier/Classifier.h"
#include "MacLearn/Classifier/Committee.h"
#include "MacLearn/DataSet/Dataset.h"
#include "MacLearn/DataSet/BatchDataset.h"

/* Create a local var to save references to "super class" functions */
static Classifier super;
static unsigned char superInitialized = 0;

/************************
* "Private" Functions	*
************************/
static __inline int Committee_InternalPredict (Committee * comt, EntryData * entry, unsigned long * prediction)
{
	int ret;
	unsigned long currPrediction;
	int i;
	int maxVotes;
	int secondPlace;		/* Number of votes the second place have */

	/* Zero all positions of the voting array */
	memset (comt->predictionVotes, 0, sizeof (int) * comt->classesCount);

	/* Initialize the number of votes the max voted class has so far */
	maxVotes = 0;
	secondPlace = 0;
	*prediction = -1;

	/* Run through all classifiers to decide the committee prediction */
	for (i=0;i<comt->classifiersCount;i++)
	{
		/* If there aren't enough voters to change the winner, break the loop */
		if (maxVotes >= (comt->classifiersCount - i) + secondPlace)
			break;

		/* Find the prediction of the i'th classifier */
		ret = comt->classifiers[i]->predict(comt->classifiers[i], entry, &currPrediction);
		if (ret != ML_OK)
			return ret;

		/* Increment the vote count of the predicted class */
		comt->predictionVotes[currPrediction - 1]++;

		/* Check if the current prediction should be updated */
		if (comt->predictionVotes[currPrediction - 1] > maxVotes)
		{
			/* Update maxVotes */
			maxVotes = comt->predictionVotes[currPrediction - 1];
			
			/* If the winning prediction changed, update second place and the prediction */
			if (*prediction != currPrediction)
			{
				secondPlace = maxVotes;
				*prediction = currPrediction;
			}
		}
	}

	/* We have a winner! (If it's a tie, the winner is the one that got to "maxVotes" first, meaning that the order of the classifier is a tie breaking mechanism) */
	return ML_OK;
}

static int Committee_BatchLearn(Committee * comt, BatchDataSet * dataset, unsigned long maxIterations, unsigned long * trainErrors, unsigned long * confMatrix)
{
	/* It's unlikely that this function should be used. Basically it trains all classifiers using the same batch parameters */
	int i;
	int ret;

	/* Run through all classifiers training them */
	for (i=0;i<comt->classifiersCount;i++)
	{
		/* Train each classifier. Stop on first error */
		ret = comt->classifiers[i]->batchLearn((Classifier *) comt->classifiers[i], dataset, maxIterations, trainErrors, confMatrix);
		if (ret != ML_OK)
			return ret;
	}

	/* All done, return OK */
	return ML_OK;
}

static int Committee_StepLearn(Committee * comt, EntryData * entry, unsigned long * predictedClass, unsigned long * confMatrix)
{
	/* It's unlikely that this function should be used. Basically it trains all classifiers using the same individual parameters */
	int i;
	int ret;

	/* Run through all classifiers training them */
	for (i=0;i<comt->classifiersCount;i++)
	{
		/* Train each classifier. Stop on first error */
		ret = comt->classifiers[i]->stepLearn((Classifier *) comt->classifiers[i], entry, predictedClass, confMatrix);
		if (ret != ML_OK)
			return ret;
	}

	/* All done, return OK */
	return ML_OK;
}

static int Committee_Predict(Committee * comt, EntryData * entry, unsigned long * predictedClass)
{
	/* Use the "inline" function to speed up the calls on Committee_Test */
	return Committee_InternalPredict (comt, entry, predictedClass);
}

#define REPORT_INTERVAL				10000

static int Committee_Test(Committee * comt, DataSet * dataset, unsigned long * testErrors, unsigned long * confMatrix)
{
	unsigned long auxErrorCount;
	unsigned long currItem;
	unsigned long report;
	EntryData * entry;
	int ret;
	unsigned long prediction;
	
	/* Check that the DataSet is compatible with this committee */
	if (comt->featsCount != dataset->featsCount || comt->classesCount != dataset->classesCount)
	{
		errno = EINVAL;
		return ML_ERR_PARAM;
	}

	/* Set the local errorCount to zero */
	auxErrorCount = 0;

	/* If the caller requested a confusion matrix, zero all the values - the values will later be incremented, not directly written */
	if (confMatrix != NULL)
		memset (confMatrix, 0, sizeof(unsigned long) * comt->classesCount * comt->classesCount);

	/* Init reporting variables */
	currItem = 0;
	report = REPORT_INTERVAL;

	/* Run until the dataset entries end */
	while (dataset->nextEntry((DataSet *) dataset, &entry) == ML_OK)
	{
		/* Predict the class - Stops on error */
		ret = Committee_InternalPredict(comt, entry, &prediction);
		if (ret != ML_OK)
			return ret;

		/* If the prediction is incorrect, increase error counter */
		if (prediction != entry->class)
			auxErrorCount++;

		/* If the caller requested a confusion matrix, update the data */
		if (confMatrix != NULL)
			confMatrix[(entry->class - 1) * comt->classesCount + (prediction - 1)]++;

		/* Update report variables */
		currItem++;
		report--;

		/* Give some feedback of current line being processed (update every 10k lines) */
		if (!report)
		{
			printf("Processed %lu entries so far\r", currItem);
			fflush(stdout);
			report = REPORT_INTERVAL;
		}
	}

	/* If the caller requested an error count, save it */
	if (testErrors != NULL)
		*testErrors = auxErrorCount;

	/* Return OK */
	return ML_OK;
}

static int Committee_Save(Committee * comt, char * dstPath)
{
	/* Won't be implemented at this time */
	errno = ENOSYS;
	return ML_ERR_NOTIMPLEMENTED;
}

static void Committee_Free (Committee * comt)
{
	/* TODO: Free each classifier once they are being cloned by the committee */
	/* Free the Classifiers array */
	if (comt->classifiers != NULL)
		free (comt->classifiers);

	/* Free the voting array */
	if (comt->predictionVotes != NULL)
		free (comt->predictionVotes);

	/* Call the "super class" free */
	super.free((Classifier *) comt);
}

/************************
* "Protected" Functions	*
************************/
int Committee_Init (Committee * comt)
{
	/* If the local "super" isn't initialized, init it */
	if (!superInitialized)
	{
		Classifier_Init(&super);
		superInitialized = 1;
	}

	/* Call the initializer for the "superclass" */
	Classifier_Init((Classifier *) comt);

	/* Initialize the function pointers. */
	comt->batchLearn = (int(*)(Classifier * , BatchDataSet *, unsigned long, unsigned long *, unsigned long *)) Committee_BatchLearn;
	comt->stepLearn = (int(*)(Classifier * , EntryData *, unsigned long *, unsigned long *)) Committee_StepLearn;
	comt->predict = (int(*)(Classifier * , EntryData *, unsigned long *)) Committee_Predict;
	comt->test = (int(*)(Classifier * , DataSet *, unsigned long *, unsigned long *)) Committee_Test;
	comt->save = (int(*)(Classifier * , char *)) Committee_Save;
	comt->free = (void(*)(Classifier *)) Committee_Free;

	/* Return OK */
	return ML_OK;
}

/************************
* "Public" Functions	*
************************/
Committee * Committee_New (int classifiersCount, Classifier ** classifiers)
{
	Committee * comt;
	int i;

	/* Check the first one independently, as it will be skipped in the for loop */
	if (classifiers[0] == NULL)
		return NULL;

	/* Check that all classifiers passed are valid and compatible */
	for (i=1;i<classifiersCount;i++)
	{
		if (classifiers[i] == NULL											||
			classifiers[i]->classesCount != classifiers[i-1]->classesCount	||
			classifiers[i]->featsCount != classifiers[i-1]->featsCount)
		{
			/* Classfiers are not equivalent, return error */
			return NULL;
		}
	}

	/* malloc memory to store the structure */
	comt = (Committee *) malloc (sizeof(Committee));
	if (comt == NULL)
		return NULL;

	/* Zero memory */
	memset (comt, 0, sizeof(Committee)); 

	/* Initialize the structure data and pointers */
	Committee_Init(comt);

	/* Initialize instance data */
	comt->classifiersCount = classifiersCount;

	/* Save the features and classes informations */
	comt->featsCount = classifiers[0]->featsCount;			/* Can use classifiers[0] because it was checked that all are the same */
	comt->classesCount = classifiers[0]->classesCount;		

	/* malloc memory to store the classifiers array */
	comt->classifiers = (Classifier **) malloc (sizeof(Classifier) * classifiersCount);
	if (comt->classifiers == NULL)
	{
		comt->free ((Classifier *) comt);
		return NULL;
	}

	/* Copy over all classifiers */
	memcpy (comt->classifiers, classifiers, sizeof(Classifier) * classifiersCount);

	/* malloc memory to store the prediction voting - useful to avoid many mallocs during execution */
	comt->predictionVotes = (int *) malloc (sizeof(int) * comt->classesCount);
	if (comt->predictionVotes == NULL)
	{
		comt->free ((Classifier *) comt);
		return NULL;
	}

	/* Return the new instance */
	return comt;
}
