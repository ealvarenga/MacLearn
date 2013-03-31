#define EXTEND_BATCHDATASET
#define EXTEND_CSVDATASET		/* To get "PROTECTED" function prototypes */
#include <stdlib.h>

#include "MacLearn/DataSet/CSVDataset.h"

/* Create a local var to save references to "super class" functions */
static BatchDataSet super;
static unsigned char superInitialized = 0;

/************************
* "Private" Functions	*
************************/
static int CSVDataSet_Load (CSVDataSet * csvDataset, char * srcPath)
{
	int ret;

	/* Open the dataset file */
	csvDataset->file = fopen (srcPath, "rb");
	if (csvDataset->file == NULL)
	{
		errno = ENOENT;
		return ML_ERR_FILENOTFOUND;
	}

	/* Load the header of the file */
	ret = csvDataset->loadHeader(csvDataset);
	if (ret != ML_OK)
		return ret;

	/* Load (or prepare) the data of the file */
	ret = csvDataset->loadData(csvDataset);
	if (ret != ML_OK)
		return ret;

	/* If readMode == FULL, and the file is still open, close the file pointer */
	if (csvDataset->readMode == BD_RM_FULL && csvDataset->file != NULL)
	{
		fclose (csvDataset->file);
		csvDataset->file = NULL;
	}

	/* Return OK */
	return ML_OK;
}

static int CSVDataSet_LoadHeader (CSVDataSet * csvDataset)
{
	off_t dataStartPos = 0;			/* If there's no header, the begining of the data is at the begining of the file */
	int ret;
	unsigned long featsCount;		/* Just to keep code simpler. Should be "optimized" out by the compiler anyway */
	unsigned long currentClass;
	unsigned long maxClass = 0;

	/* TODO: Read the feature labels, when available */
	/* Count the number of features */
	featsCount = 0;
	while (1)
	{
		/* reads one symbol at a time, looking for a "delimiter" or a line break. EOF should NOT be reached in this step */	
		ret = fgetc(csvDataset->file);
		if (ret == '\n')
		{
			/* Reached the end of the line. Stop this process */
			break;
		}
		else if (ret == csvDataset->delimiter)
		{
			/* End of one feature, increment the count */
			featsCount++;
		}
		else if (ret < 0)
		{
			/* Error ocurred, return */
			errno = EIO;
			return ML_ERR_FILE;
		}
		/* Any other character is ignored */
	}
	/* Save the number of features on the structure */
	csvDataset->featsCount = featsCount;

	/* Check if any error happened (or an unexpected file format) */
	if (feof(csvDataset->file) || ferror(csvDataset->file) || csvDataset->featsCount == 0)
	{
		/* Something went wrong. Return the error */
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* If the file has a label line, the data starts at current position (it starts at zero otherwise) */
	if (csvDataset->hasLabels)
		dataStartPos = ftello (csvDataset->file);

	/* Set the file at the begining of the data */
	fseeko(csvDataset->file, dataStartPos, SEEK_SET);

	/* Find out the number of classes - NOTE: This will consider the higher class value as total ammount of classes */
	while (!feof(csvDataset->file))
	{
		/* Resets the featsCount variable to know when we've read all feats of this line */
		featsCount = 0;
		/* Position the file at the class value */
		while (!feof(csvDataset->file) && !ferror(csvDataset->file))
		{
			/* Find the next delimiter char */
			if (fgetc(csvDataset->file) == csvDataset->delimiter)
			{
				featsCount++;
				/* If all feats have been read, break this loop */
				if (featsCount == csvDataset->featsCount)
					break;
			}
		}

		/* If an error ocurred, return error */
		if (ferror(csvDataset->file))
		{
			errno = EIO;
			return ML_ERR_FILE;
		}

		/* If found the EOF, break */
		if (feof(csvDataset->file))
			break;

		/* Read the class value */
		if (fscanf (csvDataset->file,"%lu",&currentClass) == EOF)
		{
			errno = EIO;
			return ML_ERR_FILE;
		}

		/* If this class is higher than the current max, update it */
		if (currentClass > maxClass)
			maxClass = currentClass;

		/* Read up to the next \n or EOF */
		do {
			ret = fgetc(csvDataset->file);
		} while (ret != '\n' && ret >= 0);

	}
	/* Update the struct with the classes count */
	csvDataset->classesCount = maxClass;

	/* Reset the file to the start of the Data */
	fseeko(csvDataset->file, dataStartPos, SEEK_SET);

	/* Return OK */
	return ML_OK;
}

static int CSVDataSet_NextEntry_Full (CSVDataSet * csvDataset, EntryData ** entry)
{
	off_t nextPos;

	/* Check that the dataset hasn't been fully read yet */
	if (csvDataset->currentPos >= csvDataset->entriesCount)
		return ML_WARN_EOF;

	/* On "FULL" datasets, all we need is to return a pointer to the memory location of the next position on the readOrder array */ 
	nextPos = csvDataset->readOrder[csvDataset->currentPos++];
	*entry = &csvDataset->entries[nextPos];

	/* Return OK */
	return ML_OK;
}

static int CSVDataSet_NextEntry_Incremental (CSVDataSet * csvDataset, EntryData ** entry)
{
	int ret;
	
	/* Check that the dataset hasn't been fully read yet */
	if (csvDataset->currentPos >= csvDataset->entriesCount)
		return ML_WARN_EOF;

	/* Position the file based on the readOrder array */
	fseeko (csvDataset->file, csvDataset->readOrder[csvDataset->currentPos++], SEEK_SET);

	/* Read one line from the file */
	ret = CSVDataSet_ReadLine(csvDataset, csvDataset->entries);

	/* If anything went wrong, return the error */
	if (ret != ML_OK)
		return ret;

	/* Set the returning pointer */
	*entry = csvDataset->entries;

	/* Return ok */
	return ML_OK;
}

static int CSVDataSet_LoadData_Full (CSVDataSet * csvDataset)
{
	unsigned long i;
	double * data;
	EntryData * auxEntries;
	int ret;

	/* Initialize the readOrder vector - For fully loaded datasets, the readOrder will contain the indexes of the entries array */
	for (i=0;i<csvDataset->entriesCount;i++)
		csvDataset->readOrder[i] = i;

	/* Store the dataset in a contiguous block. Useful for "recasting" this as a Matrix if needed */
	data = (double *) malloc (sizeof(double) * (csvDataset->entriesCount * csvDataset->featsCount));
	if (data == NULL)
	{
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Malloc an array to store the entries */
	auxEntries = (EntryData *) malloc (sizeof(EntryData) * csvDataset->entriesCount);
	if (auxEntries == NULL)
	{
		free (data);
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Do NOT initialize the allocated memory, since it'll be written with valid data directly */

	/* Read all records */
	for (i=0;i<csvDataset->entriesCount;i++)
	{
		/* Set this entry's pointer to the right address within the data buffer */
		auxEntries[i].features = data + (csvDataset->featsCount * i);

		/* Read one dataset line */
		ret = CSVDataSet_ReadLine (csvDataset, &auxEntries[i]);
		if (ret != ML_OK)
		{
			free (data);
			free (auxEntries);
			return ret;
		}
	}

	/* Save the entries array on the dataset struct */
	csvDataset->entries = auxEntries;

	/* Update the "nextEntry" pointer to point to the function that handles "full" datasets */
	csvDataset->nextEntry = (int(*)(DataSet *, EntryData **)) CSVDataSet_NextEntry_Full;

	/* Return OK */
	return ML_OK;
}

static int CSVDataSet_LoadData_Incremental (CSVDataSet * csvDataset)
{
	unsigned long i;
	/* Initialize the readOrder array. On incrementally read datasets we must store the start position of each record */
	/* Find "newlines" to find the start position of each record */
	i = 0;
	csvDataset->readOrder[i++] = ftello(csvDataset->file);
	while (i < csvDataset->entriesCount && !feof(csvDataset->file) && !ferror(csvDataset->file))
	{
		if (fgetc(csvDataset->file) == '\n')
			csvDataset->readOrder[i++] = ftello(csvDataset->file);
	}

	/* An "incremental" dataset must have one entry to avoid multiple allocations when reading the dataset */
	csvDataset->entries = (EntryData *) malloc (sizeof(EntryData));
	if (csvDataset->entries == NULL)
	{
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* This entry must have enough space to store one record */
	csvDataset->entries[0].features = (double *) malloc (sizeof(double) * csvDataset->featsCount);
	if (csvDataset->entries[0].features == NULL)
	{
		free (csvDataset->entries);
		csvDataset->entries = NULL;
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Update the "nextEntry" pointer to point to the function that handles "full" datasets */
	csvDataset->nextEntry = (int(*)(DataSet *, EntryData **)) CSVDataSet_NextEntry_Incremental;

	/* Return OK */
	return ML_OK;
}

static int CSVDataSet_LoadData (CSVDataSet * csvDataset)
{
	off_t dataStartPos;
	int ret;

	/* Save the position of the begining of the data */
	dataStartPos = ftello (csvDataset->file);

	/* Count "newlines" to find the number of records */
	csvDataset->entriesCount = 0;
	while (!feof(csvDataset->file) && !ferror(csvDataset->file))
	{
		if (fgetc(csvDataset->file) == '\n')
			csvDataset->entriesCount++;
	}

	/* Reset the file to the begining of the data */
	fseeko (csvDataset->file, dataStartPos, SEEK_SET);

	/* Malloc an area to store the readOrder vector */
	csvDataset->readOrder = (off_t *) malloc (sizeof(off_t) * csvDataset->entriesCount);
	if (csvDataset->readOrder == NULL)
	{
		fclose(csvDataset->file);
		csvDataset->file = NULL;
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Process the rest of the file according to the readMode */
	if (csvDataset->readMode == BD_RM_FULL)
		ret = CSVDataSet_LoadData_Full (csvDataset);
	else if	(csvDataset->readMode == BD_RM_INCREMENTAL)
		ret = CSVDataSet_LoadData_Incremental (csvDataset);
	else
	{
		free (csvDataset->readOrder);
		csvDataset->readOrder = NULL;
		errno = EINVAL;
		return ML_ERR_PARAM;
	}

	/* If something went wrong, free the readOrder array and return the error code */
	if (ret != ML_OK)
	{
		free (csvDataset->readOrder);
		csvDataset->readOrder = NULL;
		return ret;
	}

	/* Return OK */
	return ML_OK;
}

static void CSVDataSet_Free (CSVDataSet * csvDataset)
{
	/* Close the file pointer if it's still open */
	if (csvDataset->file != NULL)
	{
		fclose (csvDataset->file);
		csvDataset->file = NULL;
	}

	/* Free the readOrder array */
	if (csvDataset->readOrder != NULL)
	{
		free (csvDataset->readOrder);
		csvDataset->readOrder = NULL;
	}

	/* TODO: Free local copy of FeatsTransform */
	/*
	if (dataset->online.featsTransform != NULL)
	{
		Dataset_FreeTransform(dataset->online.featsTransform);
		free (dataset->online.featsTransform);
		dataset->online.featsTransform = NULL;
	}
	*/

	/* Free entries */
	if (csvDataset->entries != NULL)
	{
		/* entries[0] points to the whole memory block. Free it */
		if (csvDataset->entries[0].features != NULL)
			free (csvDataset->entries[0].features);

		/* Free the array */
		free (csvDataset->entries);
		csvDataset->entries = NULL;
	}

	/* Free readOrder */
	if (csvDataset->readOrder != NULL)
	{
		free (csvDataset->readOrder);
		csvDataset->readOrder = NULL;
	}

	/* Call the "superclass" free function */
	super.free((DataSet *) csvDataset);
}

/************************
* "Protected" Functions	*
************************/

int CSVDataSet_ReadLine (CSVDataSet * csvDataset, EntryData * entry)
{
	unsigned long i;
	unsigned long featsCount;

	/* TODO: If using featsTransform, the featsCount might be different */
	/* Determine the number of features on the file */
	featsCount = csvDataset->featsCount;

	/* Read this record's feats */
	for (i=0;i<featsCount;i++)
	{
		if (fscanf (csvDataset->file, "%lf", &entry->features[i]) == EOF)
			return ML_WARN_EOF;
		/* Skip the delimiter */
		fgetc (csvDataset->file);
	}

	/* Read this record's class */
	if (fscanf (csvDataset->file, "%d", &entry->class) == EOF)
		return ML_WARN_EOF;
	/* Skip the newLine */
	fgetc (csvDataset->file);

	/* If found the EOF, return a warning */
	if (feof (csvDataset->file))
		return ML_WARN_EOF;

	/* If there was an error, return the error */
	if (ferror (csvDataset->file))
	{
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* TODO: If there's a transformation set, process the features */
	/*
	if (csvDataset->batchDataset.featsTransform != NULL)
		FeatsTransform_Process (...);
	*/

	/* Return OK */
	return ML_OK;
}

void CSVDataSet_Init (CSVDataSet * csvDataset)
{
	/* If the local "super" isn't initialized, init it */
	if (!superInitialized)
	{
		BatchDataSet_Init(&super);
		superInitialized = 1;
	}

	/* Call the initializer for the "superclass" */
	BatchDataSet_Init((BatchDataSet *) csvDataset);

	/* Initialize the function pointers. */
	csvDataset->load = (int (*)(BatchDataSet *, char *)) CSVDataSet_Load;
	csvDataset->loadHeader = CSVDataSet_LoadHeader;
	csvDataset->loadData = CSVDataSet_LoadData;

	/* Overrides the default free method */
	csvDataset->free = (void(*)(DataSet *))CSVDataSet_Free;
}

/************************
* "Public" Functions	*
************************/
CSVDataSet * CSVDataSet_New (unsigned char hasLabels, char delimiter, BatchReadMode readMode, char * srcPath)
{
	CSVDataSet * csvDataset;

	/* malloc memory to store the structure */
	csvDataset = (CSVDataSet *) malloc (sizeof(CSVDataSet));
	if (csvDataset == NULL)
		return NULL;

	/* Zero memory */
	memset (csvDataset, 0, sizeof(CSVDataSet)); 

	/* Initialize the structure data and pointers */
	CSVDataSet_Init(csvDataset);

	/* Initialize instance data */
	csvDataset->hasLabels = hasLabels;
	csvDataset->delimiter = delimiter;
	csvDataset->readMode = readMode;

	/* Load data from srcPath */
	if (csvDataset->load((BatchDataSet *) csvDataset, srcPath) != ML_OK)
	{
		csvDataset->free((DataSet *) csvDataset);
		return NULL;
	}

	/* Return the new instance */
	return csvDataset;
}

int CSVDataSet_Save (DataSet * dataset, unsigned char hasLabels, char delimiter, char * outPath)
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

	/* If headers are requested, write a line containing column labels */
	if (hasLabels)
	{
		for (i=0;i<dataset->featsCount;i++)
		{
			/* TODO: Use stored column labels, if available */
			fprintf (file, "FEATURE_%lu%c",i+1,delimiter);
		}
		/* Write a label for the class column and a line break */
		fprintf (file, "CLASS\n");
	}

	/* Write all entries */
	while (dataset->nextEntry(dataset, &entry) == ML_OK)
	{
		/* Write feature values */
		for (i=0;i<dataset->featsCount;i++)
			fprintf(file, "%lf%c", entry->features[i], delimiter);

		/* Write class value */
		fprintf(file, "%d\n", entry->class);
	}

	/* Close the file */
	fclose (file);

	/* Return OK */
	return ML_OK;
}
