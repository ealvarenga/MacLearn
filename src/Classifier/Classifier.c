#include <stdlib.h>		/* For free() */

#include "MacLearn/Classifier/Classifier.h"

/************************
* "Private" Functions	*
************************/
static int Classifier_Stub (void)
{
	errno = ENOSYS;
	return ML_ERR_NOTIMPLEMENTED;
}

static void Classifier_Free (Classifier * classifier)
{
	/* As this is the base class' free, it'll be the last to be called, so free the structure pointer */
	if (classifier != NULL)
		free (classifier);
}

PUBLIC void Classifier_Init (Classifier * classifier)
{
	/* Initialize function pointers */
	classifier->batchLearn = (int(*)(Classifier * , BatchDataSet *, unsigned long, unsigned long *, unsigned long *)) Classifier_Stub;
	classifier->stepLearn = (int(*)(Classifier * , EntryData *, unsigned long *, unsigned long *)) Classifier_Stub;
	classifier->predict = (int(*)(Classifier * , EntryData *, unsigned long *)) Classifier_Stub;
	classifier->test = (int(*)(Classifier * , DataSet *, unsigned long *, unsigned long *)) Classifier_Stub;
	classifier->save = (int(*)(Classifier * , char *)) Classifier_Stub;
	classifier->free = Classifier_Free;
}

/************************
* "Public" Functions	*
************************/
