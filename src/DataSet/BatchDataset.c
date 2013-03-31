#define EXTEND_DATASET
#include "MacLearn/DataSet/BatchDataset.h"
#include "MacLearn/Util/MatrixUtil.h"			/* For ShuffleVector */

/* Create a local var to save references to "super class" functions */
static DataSet super;
static unsigned char superInitialized = 0;

/************************
* "Private" Functions	*
************************/
static int BatchDataSet_Stub (BatchDataSet * dataset, char * path)
{
	/* This function should never be called. See this as an "abstract class". */
	errno = ENOSYS;
	return ML_ERR_NOTIMPLEMENTED;
}

static int BatchDataSet_ulongCompare (const void * a, const void * b)
{
	/* if a > b, returns a negative number */
	return (*(unsigned long *)a - *(unsigned long *)b);
}

static int BatchDataSet_Sort (BatchDataSet * batchDataset)
{
	unsigned long i;

	/* Treat data sets differently, depending on the read mode */
	if (batchDataset->readMode == BD_RM_FULL)
	{
		/* For "FULL" data sets we just need to assign the index number to each position */
		for (i=0;i<batchDataset->entriesCount;i++)
			batchDataset->readOrder[i] = i;
	}
	else
	{
		/* For "INCREMENTAL" datasets, the fastest way is to do a quicksort on the readOrder array */
		qsort(batchDataset->readOrder, batchDataset->entriesCount, sizeof(batchDataset->readOrder[0]), BatchDataSet_ulongCompare);
	}

	/* Resets the reading position after sorting */
	batchDataset->reset(batchDataset);

	/* Return OK */
	return ML_OK;
}

static int BatchDataSet_Shuffle (BatchDataSet * batchDataset)
{
	/* No matter what read mode we're using, shuffling the read order will act the same as shuffling the actual records */
	shuffleVector(batchDataset->readOrder, sizeof(batchDataset->readOrder[0]), batchDataset->entriesCount);

	/* Resets the reading position after shuffling */
	batchDataset->reset(batchDataset);

	/* Return OK */
	return ML_OK;
}

static int BatchDataSet_Reset (BatchDataSet * batchDataset)
{
	/* Resets the current position to the first record */
	batchDataset->currentPos = 0;

	/* Return OK */
	return ML_OK;
}

/************************
* "Public" Functions	*
************************/
void BatchDataSet_Init (BatchDataSet * batchDataset)
{
	/* If the local "super" isn't initialized, init it */
	if (!superInitialized)
	{
		DataSet_Init(&super);
		superInitialized = 1;
	}

	/* Call the initializer for the "superclass" */
	DataSet_Init((DataSet *) batchDataset);

	/* Initialize the function pointers. */
	/* Load has no meaning in this level */
	batchDataset->load = BatchDataSet_Stub;

	/* Provide default implementations for Sort, Shuffle and Reset */
	batchDataset->sort = BatchDataSet_Sort;
	batchDataset->shuffle = BatchDataSet_Shuffle;
	batchDataset->reset = BatchDataSet_Reset;

	/* No need to override the default free method, since this class doesn't need any cleanup procedures */
}
