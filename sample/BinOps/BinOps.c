#include <stdio.h>

#include "MacLearn/DataSet/CSVDataset.h"
#include "MacLearn/Classifier/Perceptron.h"
#include "MacLearn/Inducer/BooleanInducer.h"

int main (int argc, char ** argv)
{
	CSVDataSet * andDS, * orDS, * xorDS;
	BooleanInducer * xorBoolInducer;
	unsigned long andErrorCount, orErrorCount, xorErrorCount;
	Perceptron * andPcpt, * orPcpt, * xorPcpt;

	/* Load datasets */
	puts ("Loading datasets...");
	andDS = CSVDataSet_New(FALSE, ';', BD_RM_FULL, "datasets/AND.csv");
	orDS = CSVDataSet_New(FALSE, ';', BD_RM_FULL, "datasets/OR.csv");
	xorDS = CSVDataSet_New(FALSE, ';', BD_RM_FULL, "datasets/XOR.csv");

	/* Check that all went ok */
	if (andDS == NULL || orDS == NULL || xorDS == NULL)
	{
		perror ("Error reading datasets");
		return -1;
	}

	/* "AND" and "OR" are linearly separable, but "XOR" isn't. Load a feature inducing to create separability */
	xorBoolInducer = BooleanInducer_New("datasets/XOR.boolInducer", 1);
	if (xorBoolInducer == NULL)
	{
		perror ("Error reading boolean inducer");
		return -1;
	}

	/* Create instances of Perceptrons for all datasets */
	andPcpt = Perceptron_New((DataSet *) andDS, FALSE, 1.0, 0, NULL);
	orPcpt = Perceptron_New((DataSet *) orDS, FALSE, 1.0, 0, NULL);
	xorPcpt = Perceptron_New((DataSet *) xorDS, FALSE, 1.0, 1, (FeatInducer **) &xorBoolInducer);

	/* Check that all went ok */
	if (andPcpt == NULL || orPcpt == NULL || xorPcpt == NULL)
	{
		perror ("Error creating perceptrons");
		return -1;
	}

	/* Train the perceptrons. 10 passes is enough, the lib stops early if it finds a 100% accuracy solution */
	andPcpt->batchLearn ((Classifier *) andPcpt, (BatchDataSet *) andDS, 10, &andErrorCount, NULL);
	orPcpt->batchLearn ((Classifier *) orPcpt, (BatchDataSet *) orDS, 10, &orErrorCount, NULL);
	xorPcpt->batchLearn ((Classifier *) xorPcpt, (BatchDataSet *) xorDS, 10, &xorErrorCount, NULL);

	/* Check that all went ok */
	if (andErrorCount != 0 || orErrorCount != 0 || xorErrorCount != 0)
	{
		puts ("Error training perceptrons.");
		return -1;
	}

	/* Save all three perceptrons */
	andPcpt->save ((Classifier *) andPcpt, "datasets/AND.pcpt");
	orPcpt->save ((Classifier *) orPcpt, "datasets/OR.pcpt");
	xorPcpt->save ((Classifier *) xorPcpt, "datasets/XOR.pcpt");

	/* Free memory used by datasets and perceptrons */
	andDS->free ((DataSet *) andDS);
	orDS->free ((DataSet *) orDS);
	xorDS->free ((DataSet *) xorDS);
	andPcpt->free ((Classifier *) andPcpt);
	orPcpt->free ((Classifier *) orPcpt);
	xorPcpt->free ((Classifier *) xorPcpt);

	/* Return ok */
	return 0;
}