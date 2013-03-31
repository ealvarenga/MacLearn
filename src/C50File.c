#include <stdio.h>

#include "C50File.h"

#ifndef MAX_PATH
	#define MAX_PATH			260
#endif


void C50File_Free (DataSet * dataset)
{
//	return DS_ERR_NOTIMPLEMENTED;
}

int C50File_LoadDataset (char * datasetName, DataSet * dataset, DataSetType type)
{
	return DS_ERR_NOTIMPLEMENTED;
}

int C50File_SaveDataset (char * datasetName, DataSet * dataset)
{
	EntryData * entry;
	FILE * file;
	char fileName[MAX_PATH];
	int i;

	/* Monta o nome do arquivo ".names" */
	snprintf (fileName, sizeof(fileName), "%s.names", datasetName);

	/* Abre o arquivo de saída */
	file = fopen (fileName, "wb");
	if (file == NULL)
		return DS_ERR_FILE;

	/* Grava o nome do atributo que deve ser classificado */
	fprintf (file, "classe.\n");
	/* Grava o nome/tipo dos atributos */
	for (i=0;i<dataset->featsCount;i++)
		fprintf(file, "bit%d:  continuous.\n", i + 1);
	/* Grava as classes */
	fprintf(file, "classe:  1");
	for (i=1;i<dataset->classesCount;i++)
		fprintf(file, ",%d", i+1);
	fprintf(file, ".\n");

	/* Fecha o arquivo de nomes */
	fclose (file);

	/* Monta o nome do arquivo de valores */
	snprintf (fileName, sizeof(fileName), "%s.data", datasetName);

	/* Abre o arquivo de saída */
	file = fopen (fileName, "wb");
	if (file == NULL)
		return DS_ERR_FILE;

	/* Grava todas as linhas */
	while (dataset->nextEntry(dataset, &entry) == DS_OK)
	{
		/* Grava o valor dos atributos */
		for (i=0;i<dataset->featsCount;i++)
			fprintf(file, "%lf,", entry->features[i]);

		/* Grava o valor da classe */
		fprintf(file, "%d\n", entry->classe);
	}

	/* Fecha o arquivo */
	fclose (file);

	/* Retorna OK */
	return 0;
}
