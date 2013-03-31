/* Proximos passos: */
/* Alterar o esquema de contagem de linhas para usar o mesmo que o wc */
/* Implementar o C50Dataset */
/* Implementar as transformações do DataSet - normalização, remoção de colunas estáticas e de colunas duplicadas, PCA */
/* Implementar o MultInducer */
/* Implementar o C50Inducer (unlikely) */

#include <stdio.h>
#include "MacLearn/DataSet/ArffDataset.h"
#include "MacLearn/DataSet/CSVDataset.h"
#include "MacLearn/Classifier/Perceptron.h"
#include "MacLearn/Inducer/BooleanInducer.h"
#include "MacLearn/Util/Profiler.h"

void printDataset (DataSet * ds)
{
	EntryData * entry;
	unsigned long i;

	while (ds->nextEntry(ds, &entry) == ML_OK)
	{
		for (i=0;i<ds->featsCount;i++)
			printf ("%lf\t",entry->features[i]);
		printf ("[%d]\n",entry->class);
	}
}

int main (int argc, char ** argv)
{
	ArffDataSet * trainDS, *cvDS;
	Perceptron * pcpt;
	unsigned long trainMatrix[138*138];
	unsigned long cvMatrix[138*138];
	FeatInducer * inducers[1];		/* Looks dumb, I know. Just want to have it as an array for future use */
	unsigned long trainErrors;
	unsigned long cvErrors;
	char pcptFileName[250];
	int maxLvl = 9;
	int maxIter = 20;


	Profiler_Start("Read Train DataSet");
	puts ("Lendo Dataset");
	trainDS = ArffDataSet_New(BD_RM_FULL, "MPEG_HOG/NormTrainDataSet_MPEG_HOE.arff");
	if (trainDS == NULL)
	{
		puts ("Error opening training file");
		return -1;
	}
	Profiler_Stop ("Read Train DataSet");

	puts ("Lendo inducers");
	Profiler_Start ("Read Bool Inducer");
	inducers[0] = (FeatInducer *) BooleanInducer_New("MPEG_HOG/NormTrainDataSet_MPEG_HOE.boolFeats", maxLvl);
	if (inducers[0] == NULL)
	{
		puts ("Error loading inducers");
		return -1;
	}
	Profiler_Stop ("Read Bool Inducer");


	puts ("Iniciando Perceptron");
	pcpt = Perceptron_New((DataSet *) trainDS, 0, 0.5, 1, inducers);
//	pcpt = Perceptron_Load("XOR.pcpt");

	Profiler_PrintTable(stdout);

	Profiler_Start ("Batch Learn");
	pcpt->batchLearn((Classifier *) pcpt, (BatchDataSet *) trainDS, maxIter, &trainErrors, trainMatrix);
	Profiler_Stop ("Batch Learn");

	trainDS->free((DataSet *) trainDS);

	puts ("Lendo CV Dataset");
	cvDS = ArffDataSet_New(BD_RM_FULL, "MPEG_HOG/NormCvDataSet_MPEG_HOE.arff");
	if (cvDS == NULL)
	{
		puts ("Error opening training file");
		return -1;
	}

	puts ("Iniciando teste do Perceptron");
	pcpt->test((Classifier *) pcpt, (DataSet *) cvDS, &cvErrors, cvMatrix);
	printf ("Errors on cv data: %lu  (%.2lf%%)\n", cvErrors, (1 - ((double) cvErrors)/cvDS->entriesCount)*100);

	snprintf (pcptFileName, sizeof(pcptFileName), "MPEG_HOG/NormPcpt_MPEG_HOE_%d_%04lu.pcpt", maxLvl, (unsigned long) ((1 - ((double) cvErrors)/cvDS->entriesCount)*10000));

	pcpt->save((Classifier *) pcpt, pcptFileName);

	cvDS->free((DataSet *) cvDS);

	pcpt->free ((Classifier *) pcpt);

	Profiler_PrintTable(stdout);

	return 0;
}
