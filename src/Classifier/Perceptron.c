#define EXTEND_CLASSIFIER
#include <stdlib.h>				/* For malloc/free */
#include <limits.h>				/* For DBL_MAX */
#ifndef WIN32
#include <unistd.h>				/* For unlink */
#endif

/* GSL includes */
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include "MacLearn/DataSet/Dataset.h"
#include "MacLearn/DataSet/BatchDataset.h"
#include "MacLearn/Classifier/Perceptron.h"
#include "MacLearn/Util/MatrixUtil.h"			/* For Matrix multiplication functions */
#include "MacLearn/Util/Profiler.h"

/* This is stupid, but it's just to compile under VC */
#ifndef DBL_MAX
#define DBL_MAX		1000000
#endif

/* Create a local var to save references to "super class" functions */
static Classifier super;
static unsigned char superInitialized = 0;

/* TODO: I've removed the normalization of induced features. It is a much needed mechanism for some kind of feature induction, 
but not for all of them, so it should be done in the InductionMechanism itself, not here */

/************************
* "Private" Functions	*
************************/
static __inline void SumMultVector (double * dst, double * src, double alpha, size_t len)
{
	size_t i;

	/* Processa todos os elementos */
	for (i=0;i<len;i++)
		dst[i] += (src[i] * alpha);

}

static __inline int Perceptron_InternalPredict (Perceptron * pcpt, EntryData * entry, double * feats, double * predictArray, unsigned char learn, unsigned long * confMatrix)
{
	gsl_matrix_view WMatrix;
	gsl_vector_view featsVect;
	gsl_vector_view predictVect;
	double max = -DBL_MAX;
	unsigned long i;
	int prediction=1;
	double auxValue;
	unsigned long baseIndex;

	/* Copy the entry data to the lineIn array, skipping the first position that holds the bias */
	memcpy (&feats[1], entry->features, sizeof(double) * pcpt->featsCount);

	/* Set the position of the first generated feature */
	baseIndex = pcpt->featsCount + 1;

	/* Call relevant feature inducing mechanims */
	Profiler_Start ("Feat Inducing");
	for (i=0;i<(unsigned long) pcpt->inducersCount;i++)
	{
		pcpt->inducers[i]->generate(pcpt->inducers[i], feats, baseIndex);
		baseIndex += pcpt->inducers[i]->generatedFeatsCount;
	}
	Profiler_Stop ("Feat Inducing");

	/* Initialize matrixes and vectors for the GSL */
	WMatrix = gsl_matrix_view_array(pcpt->W, pcpt->classesCount, pcpt->WColumns);
	featsVect = gsl_vector_view_array(feats, pcpt->WColumns);
	predictVect = gsl_vector_view_array(predictArray, pcpt->classesCount);

	/* Calc W * X to get the prediction array (X = feats) */
	Profiler_Start ("W*X Calc");
	if (gsl_blas_dgemv (CblasNoTrans, 1, &WMatrix.matrix, &featsVect.vector, 0, &predictVect.vector) != 0)
	{
		puts ("GSL ERROR!");
		exit(-1);
	}
	Profiler_Stop ("W*X Calc");

	/* Find the prediction with the highest value */
	Profiler_Start ("Find Prediction");
	for (i=0;i<pcpt->classesCount;i++)
	{
		/* Save current value */
		auxValue = predictArray[i];

		/* If learning using largeMargin perceptron and this isn't the correct class */
		/* NOTE: LargeMargin requires a confusion matrix to store the "error count" of individual classes */
		if (learn && confMatrix != NULL && pcpt->largeMargin && (i+1) != entry->class)
			auxValue += confMatrix[(entry->class - 1) * pcpt->classesCount + i];

		/* Store the class with the maximum value in "predict" */
		if (auxValue > max)
		{
			max = auxValue;
			prediction = i + 1;
		}
	}
	Profiler_Stop ("Find Prediction");

	/* If the caller requested a confusion matrix, update the data */
	if (confMatrix != NULL)
		confMatrix[(entry->class - 1) * pcpt->classesCount + (prediction - 1)]++;

	/* If the prediction is incorrect and we're learning, update W */
	if (learn && prediction != entry->class)
	{
		/* Sum values on the correct class weights, and subtract from the incorrectly predicted one */
		Profiler_Start("W update");
		SumMultVector(&pcpt->W[pcpt->WColumns * (entry->class - 1)],feats, pcpt->alpha, pcpt->WColumns);
		SumMultVector(&pcpt->W[pcpt->WColumns * (prediction - 1)],feats, pcpt->alpha * (-1), pcpt->WColumns);
		Profiler_Stop("W update");
	}

	/* Return the predicted class */
	return prediction;
}

/* Define the interval between status report */
#define REPORT_INTERVAL 10000
static int Perceptron_Run (Perceptron * pcpt, DataSet * dataset, unsigned long * errorCount, unsigned long * confMatrix, unsigned char learn)
{
	double * lineIn;
	double * predictArray;
	unsigned long auxErrorCount = 1;
	EntryData * entry;
	int prediction;
	unsigned long currItem;
	unsigned long report;

	/* Check that the DataSet is compatible with this perceptron */
	if (pcpt->featsCount != dataset->featsCount || pcpt->classesCount != dataset->classesCount)
	{
		errno = EINVAL;
		return ML_ERR_PARAM;
	}

	/* Get storage for a single entry line, considering bias and induced features */
	lineIn = (double *) malloc (sizeof(double) * pcpt->WColumns);
	if (lineIn == NULL)
	{
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Get storage for the prediction array, where the prediction value for each possible class will be stored */
	predictArray = (double *) malloc (sizeof(double) * pcpt->classesCount);
	if (predictArray == NULL)
	{
		free (lineIn);
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Initialize bias to 1 - the bias won't ever change, its weight is changed independently for each class */
	lineIn[0] = 1;

	/* Set the local errorCount to zero */
	auxErrorCount = 0;

	/* If the caller requested a confusion matrix, zero all the values - the values will later be incremented, not directly written */
	if (confMatrix != NULL)
		memset (confMatrix, 0, sizeof(unsigned long) * pcpt->classesCount * pcpt->classesCount);

	/* Init reporting variables */
	currItem = 0;
	report = REPORT_INTERVAL;

	/* Run until the dataset entries end */
	while (dataset->nextEntry(dataset, &entry) == ML_OK)
	{
		/* Predict the class based on current weights vector */
		prediction = Perceptron_InternalPredict(pcpt, entry, lineIn, predictArray, learn, confMatrix);

		/* If the prediction is incorrect, increase error counter */
		if (prediction != entry->class)
			auxErrorCount++;

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
	if (errorCount != NULL)
		*errorCount = auxErrorCount;

	/* Free allocated memory */
	free (lineIn);
	free (predictArray);

	/* Return OK */
	return ML_OK;
}


static int Perceptron_BatchLearn(Perceptron * pcpt, BatchDataSet * dataset, unsigned long maxIterations, unsigned long * trainErrors, unsigned long * confMatrix)
{
	int ret;
	unsigned long i;
	unsigned long localTrainErrors = 1;

	/* Shuffle the DataSet only once */
	dataset->shuffle(dataset);

	for (i=0;i<maxIterations && localTrainErrors != 0;i++)
	{
		printf ("Starting step %lu\n", i+1);

		/* Process the DataSet in learning mode */
		ret = Perceptron_Run(pcpt, (DataSet *) dataset, &localTrainErrors, confMatrix, 1);

		/* No matter the result, always reset the dataset before returning */
		dataset->reset(dataset);

		/* If something went wrong, return the error */
		if (ret != ML_OK)
			return ret;
		
		/* Give some feedback on how this step performed */
		printf ("Total errors on step %lu = %lu  (%.2lf%% accuracy)\n", i+1, localTrainErrors, (1 - ((double) localTrainErrors)/dataset->entriesCount)*100);
		Profiler_PrintTable(stdout);
	}

	/* Save the error count of the last run */
	if (trainErrors != NULL)
		*trainErrors = localTrainErrors;

	/* Return OK */
	return ML_OK;
}

/* TODO: This is awfully like the Perceptron_Run, the difference being the loop and errorCount. Merge the two functions to avoid duplicated code */
static __inline int Perceptron_SingleStep(Perceptron * pcpt, EntryData * entry, unsigned long * predictedClass, unsigned char learn, unsigned long * confMatrix)
{
	double * lineIn;
	double * predictArray;
	int prediction;

	/* Get storage for a single entry line, considering bias and induced features */
	lineIn = (double *) malloc (sizeof(double) * pcpt->WColumns);
	if (lineIn == NULL)
	{
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Get storage for the prediction array, where the prediction value for each possible class will be stored */
	predictArray = (double *) malloc (sizeof(double) * pcpt->classesCount);
	if (predictArray == NULL)
	{
		free (lineIn);
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Initialize bias to 1 - the bias won't ever change, its weight is changed independently for each class */
	lineIn[0] = 1;

	/* Predict the class based on current weights vector */
	prediction = Perceptron_InternalPredict(pcpt, entry, lineIn, predictArray, learn, confMatrix);

	/* Store the predicted value */
	*predictedClass = prediction;

	/* Free allocated memory */
	free (lineIn);
	free (predictArray);

	/* Return OK */
	return ML_OK;

}

static int Perceptron_StepLearn(Perceptron * pcpt, EntryData * entry, unsigned long * predictedClass, unsigned long * confMatrix)
{
	return Perceptron_SingleStep(pcpt, entry, predictedClass, 1, confMatrix);
}

static int Perceptron_Predict(Perceptron * pcpt, EntryData * entry, unsigned long * predictedClass)
{
	return Perceptron_SingleStep(pcpt, entry, predictedClass, 0, NULL);
}

static int Perceptron_Test(Perceptron * pcpt, DataSet * dataset, unsigned long * testErrors, unsigned long * confMatrix)
{
	/* Process the DataSet in testing (not learning) mode */
	return Perceptron_Run(pcpt, dataset, testErrors, confMatrix, 0);
}

static int Perceptron_Save(Perceptron * pcpt, char * dstPath)
{
	FILE * out;
	int i;

	/* Open the output file */
	out = fopen (dstPath, "wb");
	if (out == NULL)
	{
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* Write Perceptron structure data */
	fwrite (pcpt, sizeof(Perceptron), 1, out);

	/* Write W data */
	fwrite (pcpt->W, sizeof(double), pcpt->classesCount * pcpt->WColumns, out);

	/* Write Inducers data */
	for (i=0;i<pcpt->inducersCount;i++)
	{
		if (pcpt->inducers[i]->writeData(pcpt->inducers[i], out) != ML_OK)
			break;
	}

	/* If anything went wrong, delete the file and return error */
	if (i != pcpt->inducersCount || ferror(out))
	{
		fclose(out);
		unlink(dstPath);
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* Close the output file */
	fclose(out);

	/* Return OK */
	return ML_OK;
}

static void Perceptron_FreeInducers (Perceptron * pcpt)
{
	int i;
	unsigned long oldFeatsCount=0;

	/* If there was a list of inducers, free each Inducer instance */
	if (pcpt->inducers != NULL)
	{
		for (i=0;i<pcpt->inducersCount;i++)
		{
			if (pcpt->inducers[i] != NULL)
			{
				/* Sum the generated features count of each inducer */
				oldFeatsCount += pcpt->inducers[i]->generatedFeatsCount;
				/* Free the inducer instance */
				pcpt->inducers[i]->free(pcpt->inducers[i]);
			}
		}
		/* Free the list. */
		free (pcpt->inducers);
		pcpt->inducers = NULL;
		pcpt->inducersCount = 0;

		/* Update number of columns on W */
		pcpt->WColumns -= oldFeatsCount;
	}
}

static void Perceptron_Free (Perceptron * pcpt)
{
	/* Free the weights matrix */
	if (pcpt->W != NULL)
		free (pcpt->W);

	/* Free the inducers array */
	Perceptron_FreeInducers (pcpt);

	/* Call the "super class" free */
	super.free((Classifier *) pcpt);
}

static int Perceptron_CopyInducers (Perceptron * pcpt, int inducersCount, FeatInducer ** inducers)
{
	int i;

	/* If the list is empty, there's nothing to do */
	if (inducersCount <= 0 || inducers == NULL)
		return ML_OK;

	/* If there was a list of inducers, count the ammount of generated features before setting the new list */
	if (pcpt->inducers != NULL)
	{
		/* Free the old list. 
		NOTE: Although a realloc would be a "faster" solution, this happens so sparcely that I'd rather do this over work for code clarity */
		Perceptron_FreeInducers (pcpt);
	}

	/* malloc memory to store the inducers array. */
	pcpt->inducers = (FeatInducer **) malloc (sizeof(FeatInducer *) * inducersCount);
	if (pcpt->inducers == NULL)
	{
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Zero all entries of pcpt->inducers to ensure that all unused entries are "NULL" */
	memset (pcpt->inducers, 0, sizeof(FeatInducer *) * inducersCount);

	/* Update pcpt->inducersCount (it'll be usefull to have this set in case we need to "rollback" and free all values) */
	pcpt->inducersCount = inducersCount;

	/* Clone all entries of "inducers" array */
	for (i=0;i<inducersCount;i++)
	{
		pcpt->inducers[i] = inducers[i]->clone(inducers[i]);
		/* If something went wrong, break the loop. We'll free everything needed after it */
		if (pcpt->inducers[i] == NULL)
			break;

		/* Update WColumns size */
		pcpt->WColumns += pcpt->inducers[i]->generatedFeatsCount;
	}

	/* if something went wrong on the prior loop, free everything and return error */
	if (i != inducersCount)
	{
		Perceptron_FreeInducers (pcpt);
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}

	/* Return OK */
	return ML_OK;
}

/************************
* "Protected" Functions	*
************************/
int Perceptron_Init (Perceptron * pcpt)
{
	/* If the local "super" isn't initialized, init it */
	if (!superInitialized)
	{
		Classifier_Init(&super);
		superInitialized = 1;
	}

	/* Call the initializer for the "superclass" */
	Classifier_Init((Classifier *) pcpt);

	/* Initialize the function pointers. */
	pcpt->batchLearn = (int(*)(Classifier * , BatchDataSet *, unsigned long, unsigned long *, unsigned long *)) Perceptron_BatchLearn;
	pcpt->stepLearn = (int(*)(Classifier * , EntryData *, unsigned long *, unsigned long *)) Perceptron_StepLearn;
	pcpt->predict = (int(*)(Classifier * , EntryData *, unsigned long *)) Perceptron_Predict;
	pcpt->test = (int(*)(Classifier * , DataSet *, unsigned long *, unsigned long *)) Perceptron_Test;
	pcpt->save = (int(*)(Classifier * , char *)) Perceptron_Save;
	pcpt->free = (void(*)(Classifier *)) Perceptron_Free;
	/* TODO: Create a setInducers function that will realloc W to the new size */

	/* Return OK */
	return ML_OK;
}

/************************
* "Public" Functions	*
************************/
Perceptron * Perceptron_New (DataSet * dataset, unsigned char largeMargin, double alpha, int inducersCount, FeatInducer ** inducers)
{
	Perceptron * pcpt;

	/* malloc memory to store the structure */
	pcpt = (Perceptron *) malloc (sizeof(Perceptron));
	if (pcpt == NULL)
		return NULL;

	/* Zero memory */
	memset (pcpt, 0, sizeof(Perceptron)); 

	/* Initialize the structure data and pointers */
	Perceptron_Init(pcpt);

	/* Initialize instance data */
	pcpt->alpha = alpha;

	/* Save the features and classes informations */
	pcpt->featsCount = dataset->featsCount;
	pcpt->classesCount = dataset->classesCount;
	/* Set the initial value of WColumns. Will be updated later if inducersCount > 0 */
	pcpt->WColumns = pcpt->featsCount + 1;		/* + 1 for bias */

	/* Copy the list of inducers to the internal structure (generates a copy of each inducer) */
	if (Perceptron_CopyInducers(pcpt, inducersCount, inducers) != ML_OK)
	{
		Perceptron_Free (pcpt);
		return NULL;
	}

	/* Save the type of the perceptron */
	pcpt->largeMargin = (largeMargin != 0);

	/* malloc memory for the weights Matrix */
	pcpt->W = (double *) malloc (sizeof(double) * pcpt->classesCount * pcpt->WColumns);
	if (pcpt->W == NULL)
	{
		Perceptron_Free (pcpt);
		return NULL;
	}

	/* Zero the bytes stored in W. Needed because learning functions do incremental updates over W, not full writes */
	memset (pcpt->W, 0, sizeof(double) * pcpt->classesCount * pcpt->WColumns);

	/* Return the new instance */
	return pcpt;
}

/* TODO: Save and load have issues when saving on one environment and then loading in a different one (32 bit -> 64 bit for instance) */
Perceptron * Perceptron_Load(char * srcPath)
{
	FILE * in;
	int i;
	Perceptron * pcpt;

	/* Open the input file */
	in = fopen (srcPath, "rb");
	if (in == NULL)
		return NULL;

	/* malloc memory for the perceptron structure */
	pcpt = (Perceptron *) malloc (sizeof(Perceptron));
	if (pcpt == NULL)
	{
		fclose (in);
		return NULL;
	}

	/* Read Perceptron structure data */
	if (fread (pcpt, sizeof(Perceptron), 1, in) != 1)
	{
		fclose(in);
		free(pcpt);
		return NULL;
	}

	/* Run init to update function pointers */
	Perceptron_Init (pcpt);

	/* malloc memory for the weights Matrix */
	pcpt->W = (double *) malloc (sizeof(double) * pcpt->classesCount * pcpt->WColumns);
	if (pcpt->W == NULL)
	{
		fclose(in);
		free(pcpt);
		return NULL;
	}

	/* Read W data */
	if (fread (pcpt->W, sizeof(double), pcpt->classesCount * pcpt->WColumns, in) != pcpt->classesCount * pcpt->WColumns)
	{
		/* NOTE: The reason for not simply calling Perceptron_Free is that the inducers array may have invalid non-NULL values */
		fclose(in);
		free(pcpt->W);
		free(pcpt);
		return NULL;
	}

	/* Malloc memory for the inducers array */
	pcpt->inducers = (FeatInducer **) malloc (sizeof(FeatInducer *) * pcpt->inducersCount);
	if (pcpt->inducers == NULL)
	{
		/* NOTE: From now on it's safe to call Perceptron_Free */
		fclose(in);
		Perceptron_Free(pcpt);
		return NULL;
	}
	/* Zero inducers data to keep Perceptron_Free safe should we need it */
	memset (pcpt->inducers, 0 , sizeof(FeatInducer *) * pcpt->inducersCount);

	/* Read Inducers data */
	for (i=0;i<pcpt->inducersCount;i++)
	{
		/* Read each inducer instance - NOTE: We don't really care about the type of the inducer in this context */
		pcpt->inducers[i] = FeatInducer_ReadData(in);
		if (pcpt->inducers[i] == NULL)
			break;
	}

	/* If anything went wrong, return error */
	if (i != pcpt->inducersCount || ferror(in))
	{
		fclose(in);
		Perceptron_Free(pcpt);
		return NULL;
	}

	/* Close the output file */
	fclose(in);

	/* Return the new instance */
	return pcpt;
}

