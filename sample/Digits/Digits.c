#include <stdio.h>
#include "MacLearn/DataSet/CSVDataset.h"
#include "MacLearn/Classifier/Perceptron.h"
#include "MacLearn/Util/MatrixUtil.h"
#include "MacLearn/Inducer/BooleanInducer.h"
#include "MacLearn/Classifier/Committee.h"

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define PERCEPTRON_COUNT			8

int main (int argc, char ** argv)
{
	CSVDataSet * trainDS, *testDS;			/* Structures that will hold the data sets */
	Perceptron * pcpts[PERCEPTRON_COUNT];	/* The Perceptrons that will be trained to form the committee */
	unsigned long confMatrix[10*10];		/* Confusion matrix created during training/testing */
	unsigned long errorCount;				/* Variable to store the number of errors found during training/test */
	Committee * comt;						/* The committee of perceptrons */
	FeatInducer * inducer;					/* Feature inducer used by the perceptrons */
	int i;
	char fileName[256];
	EntryData * entry;
	unsigned long prediction;
	FILE * outFile;

	/* Load the training data set */
	puts ("Loading training data set");
	trainDS = CSVDataSet_New(FALSE, ',', BD_RM_FULL, "datasets/norm_digits_train.data");
	if (trainDS == NULL)
	{
		puts ("Error opening training file");
		return -1;
	}

	puts ("Reading feature inducer");
	inducer = (FeatInducer *) BooleanInducer_New("datasets/norm_digits_train.boolFeats", 7);
	if (inducer == NULL)
	{
		puts ("Error loading inducers");
		return -1;
	}
	/* Create the six perceptrons, with different input parameters */
	pcpts[0] = Perceptron_New((DataSet *) trainDS, 0, 1.0, 1, (FeatInducer **) &inducer);		/* With induced feats */
	pcpts[1] = Perceptron_New((DataSet *) trainDS, 0, 1.0, 0, NULL);							/* Without induced feats */
	pcpts[2] = Perceptron_New((DataSet *) trainDS, 0, 1.0, 1, (FeatInducer **) &inducer);		/* With induced feats */
	pcpts[3] = Perceptron_New((DataSet *) trainDS, 0, 1.0, 0, NULL);							/* Without induced feats */
	pcpts[4] = Perceptron_New((DataSet *) trainDS, 1, 1.0, 1, (FeatInducer **) &inducer);		/* Large margin with induced feats */
	pcpts[5] = Perceptron_New((DataSet *) trainDS, 1, 1.0, 0, NULL);							/* Large Margin without induced feats */
	pcpts[6] = Perceptron_New((DataSet *) trainDS, 0, 0.5, 1, (FeatInducer **) &inducer);		/* Small steps with induced feats */
	pcpts[7] = Perceptron_New((DataSet *) trainDS, 0, 0.5, 0, NULL);							/* Small steps without induced feats */

	/* Test if all perceptrons were created sucessfully */
	for (i=0;i<PERCEPTRON_COUNT;i++)
	{
		if (pcpts[i] == NULL)
		{
			printf ("Error creating percpetron number %d\n", i);
			return -1;
		}
	}

	/* Train the Perceptrons */
	for (i=0;i<PERCEPTRON_COUNT;i++)
		pcpts[i]->batchLearn((Classifier *) pcpts[i], (BatchDataSet *) trainDS, 50, &errorCount, confMatrix);

	/* Free trainning data */
	trainDS->free((DataSet *) trainDS);

	/* Load the test data set */
	puts ("Loading test data set");
	testDS = CSVDataSet_New(FALSE, ',', BD_RM_FULL, "datasets/norm_digits_test.data");
	if (testDS == NULL)
	{
		puts ("Error opening test file");
		return -1;
	}

	/* Create the committee of peceptrons */
	comt = Committee_New(PERCEPTRON_COUNT, (Classifier **) pcpts);
	if (comt == NULL)
	{
		puts ("Error creating committee");
		return -1;
	}

	/* Generate an output to send to kaggle */
	puts ("Creating Perceptrons Committee output file");
	outFile = fopen ("datasets/norm_digits_output.csv", "wt");
	if (outFile == NULL)
	{
		puts ("Error creating output file");
		return -1;
	}

	while (testDS->nextEntry((DataSet *) testDS, &entry) == ML_OK)
	{
		/* Find the prediction */
		comt->predict((Classifier *)comt, entry, &prediction);

		/* write to file - convert class 10 to 0, as is expected by kaggle */
		fprintf (outFile, "%lu\n", prediction == 10 ? 0 : prediction);
	}

	/* Save perceptrons for later use */
	for (i=0;i<PERCEPTRON_COUNT;i++)
	{
		snprintf(fileName, sizeof(fileName), "digits_%d.pcpt", i+1);
		pcpts[i]->save((Classifier *) pcpts[i], fileName);
	}

	/* Free Committee, cv data and perceptrons */
	comt->free ((Classifier *) comt);
	testDS->free((DataSet *) testDS);
	for (i=0;i<PERCEPTRON_COUNT;i++)
		pcpts[i]->free ((Classifier *) pcpts[i]);

	/* End program with return code 0 */
	return 0;
}
