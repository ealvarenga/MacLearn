#include <stdlib.h>		/* For free() */

#include "MacLearn/DataSet/Dataset.h"

/************************
* "Private" Functions	*
************************/
static int DataSet_NextEntry (DataSet * dataset, EntryData ** entry)
{
	/* This function should never be called. See this as an "abstract class". */
	errno = ENOSYS;
	return ML_ERR_NOTIMPLEMENTED;
}

/************************
* "Protected" Functions	*
************************/
void DataSet_Free (DataSet * dataset)
{
	/* As this is the base class' free, it'll be the last to be called, so free the structure pointer */
	if (dataset != NULL)
		free (dataset);
}

void DataSet_Init (DataSet * dataset)
{
	/* Initialize the function pointers */
	dataset->free = DataSet_Free;
	dataset->nextEntry = DataSet_NextEntry;
}

/************************
* "Public" Functions	*
************************/
