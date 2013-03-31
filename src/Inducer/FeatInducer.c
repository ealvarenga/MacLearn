#include <stdlib.h>		/* For free() */

#include "MacLearn/Inducer/FeatInducer.h"
#include "MacLearn/Inducer/BooleanInducer.h"

static struct
{
	char inducerID[INDUCER_ID_LEN];
	FeatInducer * (*readFunction)(FILE *);
} knownInducers[] = {{BOOLEANINDUCER_ID, (FeatInducer * (*)(FILE *)) BooleanInducer_ReadData}};

/************************
* "Private" Functions	*
************************/
static int FeatInducer_Stub (void)
{
	errno = ENOSYS;
	return ML_ERR_NOTIMPLEMENTED;
}

static void FeatInducer_Free (FeatInducer * featInducer)
{
	/* As this is the base class' free, it'll be the last to be called, so free the structure pointer */
	if (featInducer != NULL)
		free (featInducer);
}

void FeatInducer_Init (FeatInducer * featInducer)
{
	/* Initialize function pointers */
	featInducer->generate = (int(*)(struct FeatInducer *, double *, unsigned long)) FeatInducer_Stub;
	featInducer->clone = (FeatInducer * (*)(struct FeatInducer *)) FeatInducer_Stub;
	featInducer->writeData = (int (*) (struct FeatInducer *, FILE *)) FeatInducer_Stub;
	featInducer->free = FeatInducer_Free;
}

/************************
* "Public" Functions	*
************************/

FeatInducer * FeatInducer_ReadData (FILE * in)
{
	/* NOTE: This function is "inconvenient" to say the least. Since I have to read a file without knowing what kind of inducer to expect, 
	the best solution I could think of was to add an ID value to the file and, based on this ID, decide which Read method I should call.
	The problem with this approach is that this "abstract class" must now know each and every existing implementation, so it can test the ID
	and call the appropriate function.
	The list of known implementations is at the beggining of this file.
	*/

	size_t i;
	char idOnFile[INDUCER_ID_LEN];

	/* Read the ID on the file */
	if (fread(idOnFile, sizeof(char), INDUCER_ID_LEN, in) != INDUCER_ID_LEN)
		return NULL;

	/* Return the file to the initial position so other functions can validate the ID */
	fseeko (in, -INDUCER_ID_LEN, SEEK_CUR);

	/* Search the identifier over all known IDs to decide which Read function to call */
	for (i=0;i<sizeof(knownInducers)/sizeof(knownInducers[0]);i++)
	{
		if (memcmp(knownInducers[i].inducerID, idOnFile, INDUCER_ID_LEN) == 0)
			break;
	}

	/* If the ID on file doesn't match any known ID, return NULL */
	if (i == sizeof(knownInducers)/sizeof(knownInducers[0]))
		return NULL;

	/* Call the Read Function and return the instance returned by it */
	return knownInducers[i].readFunction(in);
}