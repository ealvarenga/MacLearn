#include <stdio.h>
#include "MacLearn/DataSet/CSVDataset.h"
#include "MacLearn/DataSet/ArffDataset.h"

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

int main (int argc, char ** argv)
{
	CSVDataSet * csvDataset;			/* Structures that will hold the CSV data sets */

	/* Load the training data set */
	puts ("Loading training data set");
	csvDataset = CSVDataSet_New(FALSE, ',', BD_RM_FULL, "datasets/digits_train.data");
	if (csvDataset == NULL)
	{
		puts ("Error opening training file");
		return -1;
	}

	/* Save the Data set as an Arff file */
	puts ("Writing Arff file");
	ArffDataSet_Save((DataSet *) csvDataset, "datasets/digits_train.arff");

	/* Free memory */
	csvDataset->free((DataSet *) csvDataset);

	/* Load the cross validation data set */
	puts ("Loading test data set");
	csvDataset = CSVDataSet_New(TRUE, ';', BD_RM_FULL, "datasets/digits_test.csv");
	if (csvDataset == NULL)
	{
		puts ("Error opening cross validation file");
		return -1;
	}

	/* Save the Data set as an Arff file */
	puts ("Writing Arff file");
	csvDataset->reset((BatchDataSet *) csvDataset);
	ArffDataSet_Save((DataSet *) csvDataset, "datasets/digits_test.arff");

	/* Free memory */
	csvDataset->free((DataSet *) csvDataset);

	/* End program with return code 0 */
	return 0;
}