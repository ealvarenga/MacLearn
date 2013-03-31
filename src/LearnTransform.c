#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "Arff.h"
#include "Dataset.h"

void printUsage (char * progName)
{
	printf ("Usage: %s [-b|-o] dataset-file output-file\n", progName);
	puts ("");
	puts ("  -b, --batch\t\tProcessa o dataset de forma batch. (Default)");
	puts ("\t\t\tConsome mais memória, mas é muito mais eficiente.");
	puts ("  -o, --online\t\tProcessa o dataset de forma online.");
	puts ("  -p, --pca\t\tAprende o PCA (com 99% de variancia mantida).");
	puts ("  -?, --help\t\tMostra estas informações.");
}

int main (int argc, char ** argv)
{
	int opt;
	DataSetType dsType = DS_TYPE_BATCH;
	DataSet dataset;
	char * datasetPath = NULL;
	char * outputFile = NULL;
	int longIndex;
	int ret;
	FeatsTransform featsTransform;
	/* Inicializa as opções que o programa recebe */
	struct option longOpts[] = {
		{"online",			no_argument,		NULL, 'o'},
		{"batch",			no_argument,		NULL, 'b'},
		{"help",			no_argument,		NULL, '?'},
		{"pca",				no_argument,		NULL, '?'},
		{NULL,				0,					NULL, 0}
	};

	memset (&featsTransform, 0, sizeof(FeatsTransform));
	/* Trata as opções */
	while ((opt = getopt_long(argc, argv, ":obtp?",longOpts, &longIndex)) != -1)
	{
		switch (opt)
		{
			case 'o':
				/* dataset online */
				dsType = DS_TYPE_ONLINE;
				break;
			case 'b':
				/* dataset bactch - Lê tudo para a memória e depois processa. */
				dsType = DS_TYPE_BATCH;
				break;
			case 'p':
				/* Salva se deve aprender o PCA */
				featsTransform.runPCA = 1;
				break;
			case '?':
				printUsage(argv[0]);
				return -1;
			default:
				printf ("Opção inválida: %s\n", argv[optind]);
				printUsage(argv[0]);
				return -1;
		}
	}

	/* Testa se vieram os nomes dos arquivos de entrada e saída */
	if (argc != optind + 2)
	{
		printUsage(argv[0]);
		return -1;
	}
	/* Salva os nomes dos arquivos de entrada e saída */
	datasetPath = argv[optind];
	outputFile= argv[optind + 1];

	/* Carrega o dataset */
	ret = Arff_LoadDataset (datasetPath, &dataset, dsType);
	if (ret != DS_OK)
	{
		printf ("Erro lendo arquivo arff: %d\n", ret);
		return -2;
	}

	/* Aprende as transformações em cima do dataset */
	ret = Dataset_LearnTransform(&dataset, &featsTransform);
	if (ret != DS_OK)
	{
		printf ("Erro processando o dataset: %d\n", ret);
		return -3;
	}

	/* Salva o transform no arquivo de saída */
	ret = Dataset_SaveTransform (outputFile, &featsTransform);
	if (ret != DS_OK)
	{
		printf ("Erro salvando as transformações: %d\n", ret);
		return -3;
	}

	/* Libera a memória */
	Dataset_FreeTransform(&featsTransform);
	Arff_Free(&dataset);

	/* Retorna OK */
	return 0;
}