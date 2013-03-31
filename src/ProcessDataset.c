#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "Dataset.h"
#include "Arff.h"
#include "C50File.h"

void printUsage (char * progName)
{
	printf ("Usage: %s -d dataset-file -t transform-file -o output-file\n", progName);
	puts ("");
	puts ("  -d, --dataset fileName\tArquivo de dataset que ser� lido.");
	puts ("  -t, --transform fileName\tArquivo de transforma��es a serem feitas.");
	puts ("  -a, --arff fileName\t\tArquivo para gravar o dataset de sa�da no formato Arff.");
	puts ("  -c, --c50 fileName\t\tArquivo para gravar o dataset de sa�da no formato C50.");
	puts ("  -?, --help\t\t\tMostra estas informa��es.");
}

int main (int argc, char ** argv)
{
	int opt;
//	DataSetType dsType = DS_TYPE_BATCH;
	DataSet dataset;
	char * datasetPath = NULL;
	char * arffOutputPath = NULL;
	char * c50OutputPath = NULL;
	char * transformPath = NULL;
	int longIndex;
	int ret;
	FeatsTransform featsTransform;
	/* Inicializa as op��es que o programa recebe */
	struct option longOpts[] = {
		{"dataset",			required_argument,	NULL, 'd'},
		{"transform",		required_argument,	NULL, 't'},
		{"arff",			required_argument,	NULL, 'a'},
		{"c50",				required_argument,	NULL, 'c'},
		{"help",			no_argument,		NULL, '?'},
		{NULL,				0,					NULL, 0}
	};

	memset (&featsTransform, 0, sizeof(FeatsTransform));
	/* Trata as op��es */
	while ((opt = getopt_long(argc, argv, ":d:t:a:c:?",longOpts, &longIndex)) != -1)
	{
		switch (opt)
		{
			case 'd':
				/* Salva o arquivo do dataset */
				datasetPath = optarg;
				break;
			case 't':
				/* Salva o arquivo de transforma��o */
				transformPath = optarg;
				break;
			case 'a':
				/* Salva o aruqivo de saida para arff */
				arffOutputPath = optarg;
				break;
			case 'c':
				/* Salva o aruqivo de saida para C50 */
				c50OutputPath = optarg;
				break;
			case '?':
				printUsage(argv[0]);
				return -1;
			default:
				printf ("Op��o inv�lida: %c\n", opt);
				printUsage(argv[0]);
				return -1;
		}
	}

	/* Testa se veio uma combina��o �til de par�metros*/
	if (datasetPath == NULL || (transformPath == NULL && c50OutputPath == NULL) || (c50OutputPath == NULL && arffOutputPath == NULL))
	{
		printUsage(argv[0]);
		return -1;
	}

	if (transformPath != NULL)
	{
		/* Carrega o featsTransform */
		ret = Dataset_LoadTransform (transformPath, &featsTransform);
		if (ret != DS_OK)
		{
			printf ("Erro lendo as transforma��es: %d\n", ret);
			return -1;
		}
	}

	/* Carrega o dataset */
	ret = Arff_LoadDataset (datasetPath, &dataset, DS_TYPE_BATCH);
	if (ret != DS_OK)
	{
		printf ("Erro lendo arquivo arff: %d\n", ret);
		return -2;
	}

	if (transformPath != NULL)
	{
		/* Processa o dataset */
		ret = Dataset_ProcessDataset(&dataset, &featsTransform);
		if (ret != DS_OK)
		{
			printf ("Erro processando o dataset: %d\n", ret);
			return -3;
		}
	}

	/* Salva o arquivo ARFF */
	if (arffOutputPath != NULL)
	{
		/* Salva o dataset no arquivo de sa�da */
		ret = Arff_SaveDataset (arffOutputPath, &dataset);
		if (ret != DS_OK)
		{
			printf ("Erro salvando o dataset em ARFF: %d\n", ret);
			return -4;
		}
	}

	/* Salva o arquivo c50 */
	if (c50OutputPath != NULL)
	{
		/* Salva o dataset no arquivo de sa�da */
		ret = C50File_SaveDataset (c50OutputPath, &dataset);
		if (ret != DS_OK)
		{
			printf ("Erro salvando o dataset em C50: %d\n", ret);
			return -4;
		}
	}

	/* Libera a mem�ria */
	if (transformPath != NULL)
		Dataset_FreeTransform(&featsTransform);
	Arff_Free(&dataset);

	/* Retorna OK */
	return 0;
}
