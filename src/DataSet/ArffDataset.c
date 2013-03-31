#define EXTEND_CSVDATASET		
#include <stdlib.h>
#include <string.h>

#include "MacLearn/DataSet/ArffDataset.h"

/* Create a local var to save references to "super class" functions */
static CSVDataSet super;
static unsigned char superInitialized = 0;

/************************
* "Private" Functions	*
************************/
static int ArffDataSet_LoadHeader (ArffDataSet * arffDataSet)
{
	char auxTxt[11];
	off_t dataStartPos;
	off_t lastAttributePos = 0;
	unsigned long featsCount;				/* Just to reduce clutter in the code */
	unsigned long classesCount;				/* Just to reduce clutter in the code */
	FILE * datasetFile;						/* Just to reduce clutter in the code */
	int ret;

	/* Initialize some vars */
	featsCount = 0;
	classesCount = 0;
	datasetFile = arffDataSet->file;

	/* NOTE: Can only handle attributes of type NUMBER */
	while (fgets (auxTxt, sizeof(auxTxt), datasetFile))
	{
		/* If read an "attribute" tag, increment the featsCount and continue */
		if (strncasecmp (auxTxt, "@ATTRIBUTE", 10) == 0)
		{
			featsCount++;
			lastAttributePos = ftello (datasetFile);
			continue;
		}
		/* If read an @data tag, the header is over */
		if (strncasecmp (auxTxt, "@DATA", 5) == 0)
			break;
	}

	/* Check if any error happened (or an unexpected file format) */
	if (feof(datasetFile) || ferror(datasetFile) || featsCount == 0)
	{
		/* Something went wrong. Return the error */
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* Decrement featsCount by 1, because the last "@ATTRIBUTE" is actually the "class" attribute */
	featsCount--;

	/* Check if the new line after @data is already consumed. Consume it if needed */
	if (auxTxt[strlen(auxTxt) - 1] != '\n')
		while (fgetc(datasetFile) != '\n');

	/* Save the position of the begining of the data */
	dataStartPos = ftello (datasetFile);

	/* Go back to the last attribute position to count the number of classes */
	fseeko (datasetFile, lastAttributePos, SEEK_SET);
	
	/* Find the '{' to begin counting classes */
	while (fgetc(datasetFile) != '{');

	classesCount = 0;
	/* Count the number of classes */
	while (1)
	{
		ret = fgetc(datasetFile);
		if (ret == ',' || ret == '}')
			classesCount++;
		if (ret == '}')
			break;
	}

	/* Go back to the position of the begining of data */
	fseeko (datasetFile, dataStartPos, SEEK_SET);

	/* Update the structure variables */
	arffDataSet->featsCount = featsCount;
	arffDataSet->classesCount = classesCount;

	/* Return OK */
	return ML_OK;
}

/************************
* "Protected" Functions	*
************************/
void ArffDataSet_Init (ArffDataSet * arffDataset)
{
	/* If the local "super" isn't initialized, init it to get pointers to "super" functions */
	if (!superInitialized)
	{
		CSVDataSet_Init(&super);
		superInitialized = 1;
	}

	/* Call the initializer for the "superclass" */
	CSVDataSet_Init((CSVDataSet *) arffDataset);

	/* Initialize the function pointers. */
	arffDataset->loadHeader = (int (*) (struct CSVDataSet *))ArffDataSet_LoadHeader;
	//csvDataset->batchDataset.save = (int (*)(BatchDataSet *, char *)) CSVDataSet_Save;

	/* No need to override free - nothing special to be done here */		
}

/************************
* "Public" Functions	*
************************/
ArffDataSet * ArffDataSet_New (BatchReadMode readMode, char * srcPath)
{
	ArffDataSet * arffDataset;

	/* malloc memory to store the structure */
	arffDataset = (ArffDataSet *) malloc (sizeof(ArffDataSet));
	if (arffDataset == NULL)
		return NULL;

	/* Zero memory */
	memset (arffDataset, 0, sizeof(ArffDataSet)); 
	
	/* Initialize the structure data and pointers */
	ArffDataSet_Init(arffDataset);

	/* Initialize instance data */
	arffDataset->hasLabels = 1;				/* Arff files always have feature labels */
	arffDataset->delimiter = ',';			/* Arff files always use commas as delimiter */
	arffDataset->readMode = readMode;

	/* Load data from srcPath */
	if (arffDataset->load((BatchDataSet *) arffDataset, srcPath) != ML_OK)
	{
		arffDataset->free((DataSet *) arffDataset);
		return NULL;
	}

	/* Return the new instance */
	return arffDataset;
}

int ArffDataSet_Save (DataSet * dataset, char * outPath)
{
	EntryData * entry;
	FILE * file;
	unsigned long i;

	/* Open the output file */
	file = fopen (outPath, "wb");
	if (file == NULL)
	{
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* Write ARFF Headers */
	/* TODO: Require a name for the relation? */
	fprintf (file, "@relation GENERATED_RELATION\n");

	/* Write feature names */
	/* TODO: Use stored column labels, if available */
	for (i=0;i<dataset->featsCount;i++)
		fprintf(file, "@ATTRIBUTE FEATURE_%lu  NUMERIC\n", i + 1);
	
	/* Write classes */
	fprintf(file, "@ATTRIBUTE class {1");
	for (i=1;i<dataset->classesCount;i++)
		fprintf(file, ",%lu", i+1);
	fprintf(file, "}\n\n");

	/* Mark the beginning of data on the arff file */
	fprintf(file, "@DATA\n");

	/* Write all entries */
	while (dataset->nextEntry(dataset, &entry) == ML_OK)
	{
		/* Write feature values */
		for (i=0;i<dataset->featsCount;i++)
			fprintf(file, "%lf,", entry->features[i]);

		/* Write class value */
		fprintf(file, "%d\n", entry->class);
	}

	/* Close the file */
	fclose (file);

	/* Return OK */
	return ML_OK;
}
