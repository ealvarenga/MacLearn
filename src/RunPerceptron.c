#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

#include "Dataset.h"
#include "Arff.h"
#include "Perceptron.h"
#include "GeneratedFeatures.h"
#include "MatrixUtil.h"

void printUsage (char * progName)
{
	printf ("Usage: %s -d dataset-file -t transform-file\n", progName);
	puts ("");
	puts ("  -d, --dataset fileName                 Arquivo de dataset que será lido.");
	puts ("  -t, --transform fileName               Arquivo de transformações a serem feitas.");
	puts ("  -c, --cross-validation fileName        Arquivo de dataset que será usado para testar o perceptron.");
	puts ("  -g, --gen-feats fileName               Arquivo com as feats que devem ser geradas.");
	puts ("  -b, --gen-bool-feats fileName          Arquivo com as feats booleanas que devem ser geradas.");
	puts ("  -m, --max-lvl num                      Level maximo na arvore de features.");
	puts ("  -M, --max-bool-lvl num                 Level maximo na arvore de features booleanas.");
	puts ("  -i, --max-iterations num               Número máximo de iterações.");
	puts ("  -a, --alpha dblNum                    \"Learning rate\".");
	puts ("  -?, --help\t\t\tMostra estas informações.");
}

int main (int argc, char ** argv)
{
	int opt;
//	DataSetType dsType = DS_TYPE_BATCH;
	DataSet trainDataset;
	DataSet cvDataset;
	char * datasetPath = NULL;
	char * cvDatasetPath = NULL;
	char * transformPath = NULL;
	char * genFeatsPath = NULL;
	char * genBoolFeatsPath = NULL;
	int longIndex;
	int ret;
	int maxLvl=1;
	int maxBoolLvl=1;
	unsigned long totalErrors;
	Perceptron perceptron;
	FeatsTransform featsTransform;
	GeneratedFeatureList genFeatsList;
	unsigned long * confMatrix;
	int maxIter = 10;
	double alpha = 1.0;
	/* Inicializa as opções que o programa recebe */
	struct option longOpts[] = {
		{"dataset",			required_argument,	NULL, 'd'},
		{"transform",		required_argument,	NULL, 't'},
		{"cross-validation",required_argument,	NULL, 'c'},
		{"gen-feats",		required_argument,	NULL, 'g'},
		{"gen-bool-feats",	required_argument,	NULL, 'b'},
		{"max-iterations",	required_argument,	NULL, 'i'},
		{"max-lvl",			required_argument,	NULL, 'i'},
		{"max-bool-lvl",	required_argument,	NULL, 'i'},
		{"alpha",			required_argument,	NULL, 'a'},
		{"help",			no_argument,		NULL, '?'},
		{NULL,				0,					NULL, 0}
	};

	memset (&featsTransform, 0, sizeof(FeatsTransform));
	/* Trata as opções */
	while ((opt = getopt_long(argc, argv, ":d:t:c:g:b:m:M:i:a:?",longOpts, &longIndex)) != -1)
	{
		switch (opt)
		{
			case 'd':
				/* Salva o arquivo do dataset */
				datasetPath = optarg;
				break;
			case 'c':
				/* Salva o arquivo do dataset */
				cvDatasetPath = optarg;
				break;
			case 'm':
				/* Salva o lvl maximo */
				maxLvl = atoi(optarg);
				break;
			case 'M':
				/* Salva o lvl maximo */
				maxBoolLvl = atoi(optarg);
				break;
			case 'i':
				/* Salva o número de iterações */
				maxIter = atoi(optarg);
				break;
			case 'a':
				/* Salva o número de iterações */
				alpha = atof(optarg);
				break;
			case 'g':
				/* Salva o arquivo do dataset */
				genFeatsPath = optarg;
				break;
			case 'b':
				/* Salva o arquivo do dataset */
				genBoolFeatsPath = optarg;
				break;
			case 't':
				/* Salva o arquivo de transformação */
				transformPath = optarg;
				break;
			case '?':
				printUsage(argv[0]);
				return -1;
			default:
				printf ("Opção inválida: %c\n", opt);
				printUsage(argv[0]);
				return -1;
		}
	}

	/* Testa se veio uma combinação útil de parâmetros*/
	if (datasetPath == NULL || cvDatasetPath == NULL)
	{
		printUsage(argv[0]);
		return -1;
	}

	/* Imprime os parâmetros usados */
	printf ("Train Dataset file = %s\n", datasetPath);
	printf ("Validation Dataset file = %s\n", cvDatasetPath);
	printf ("Transformation file = %s\n", transformPath);
	printf ("Feature Generation file = %s\n", genFeatsPath);
	printf ("Boolean Feature Generation file = %s\n", genBoolFeatsPath);
	printf ("Max Level = %d\n", maxLvl);
	printf ("Max Bool Level = %d\n", maxBoolLvl);
	printf ("Max Iterations = %d\n", maxIter);
	printf ("Learning Rate = %lf\n", alpha);
	printf ("\n\n");

	if (transformPath != NULL)
	{
		/* Carrega o featsTransform */
		ret = Dataset_LoadTransform (transformPath, &featsTransform);
		if (ret != DS_OK)
		{
			printf ("Erro lendo as transformações: %d\n", ret);
			return -1;
		}
	}

	/* Carrega o dataset de treino */
	ret = Arff_LoadDataset (datasetPath, &trainDataset, DS_TYPE_BATCH);
	if (ret != DS_OK)
	{
		printf ("Erro lendo arquivo arff: %d\n", ret);
		return -2;
	}

	if (transformPath != NULL)
	{
		/* Processa o dataset */
		ret = Dataset_ProcessDataset(&trainDataset, &featsTransform);
		if (ret != DS_OK)
		{
			printf ("Erro processando o dataset: %d\n", ret);
			return -3;
		}
	}

	/* Carrega o dataset de cv */
	ret = Arff_LoadDataset (cvDatasetPath, &cvDataset, DS_TYPE_BATCH);
	if (ret != DS_OK)
	{
		printf ("Erro lendo arquivo arff: %d\n", ret);
		return -2;
	}

	if (transformPath != NULL)
	{
		/* Processa o dataset */
		ret = Dataset_ProcessDataset(&cvDataset, &featsTransform);
		if (ret != DS_OK)
		{
			printf ("Erro processando o dataset: %d\n", ret);
			return -3;
		}
	}

	/* Aloca memória para a matriz de confusão */
	confMatrix = malloc (sizeof(unsigned long) * trainDataset.classesCount * trainDataset.classesCount);
	if (confMatrix == NULL)
	{
		printf ("Out of memory!\n");
		return -5;
	}

	/* Inicializa a qtd de feats geradas */
	genFeatsList.genBoolFeatsCount = 0;
	genFeatsList.genFeatsCount = 0;
	genFeatsList.genFeats = NULL;
	genFeatsList.genBoolFeats = NULL;

	/* Carrega as features geradas */
	if (genFeatsPath != NULL || genBoolFeatsPath != NULL)
	{
		if (!GeneratedFeatures_Load(genFeatsPath, genBoolFeatsPath, &genFeatsList, maxLvl, maxBoolLvl, trainDataset.featsCount))
		{
			printf ("Erro lendo as arquivo de geração de atributos\n");
			return -1;
		}
	}

	/* Aprende o perceptron */
	/* TODO: Ler o learning rate e o maxIterations como parametros */
	ret = Perceptron_Learn(&trainDataset, alpha, maxIter, (genFeatsList.genBoolFeatsCount != 0 || genFeatsList.genFeatsCount != 0) ? &genFeatsList : NULL, 0.0, &perceptron, &totalErrors, confMatrix);
	if (ret != DS_OK)
	{
		printf ("Erro executando o perceptron: %d\n", ret);
		return -4;
	}

//	printULMatrix ("Confusion Matrix (Train)", confMatrix, dataset.classesCount, dataset.classesCount);

	printf ("Taxa de acerto no dataset de treino: %02.02lf\n", 100 * ((double) trainDataset.entriesCount - totalErrors)/trainDataset.entriesCount);

	/* Testa o perceptron */
	ret = Perceptron_Test(&cvDataset, &perceptron, &totalErrors, confMatrix);
	if (ret != DS_OK)
	{
		printf ("Erro testando o perceptron: %d\n", ret);
		return -2;
	}

	/* Achei um parametro bom */
	printULMatrix ("Confusion Matrix (Test)", confMatrix, cvDataset.classesCount, cvDataset.classesCount);
	printf ("Taxa de acerto no dataset de cross-validation: %02.02lf\n", 100 * ((double) cvDataset.entriesCount - totalErrors)/cvDataset.entriesCount);

	/* TODO: Usar um free certo - Dá uma aliviada na memória */
	free(perceptron.W);
	if (genFeatsList.genFeats != NULL)
		free(genFeatsList.genFeats);
	if (genFeatsList.genBoolFeats != NULL)
		free(genFeatsList.genBoolFeats);

	/* Libera a memória do treino */
	Arff_Free(&trainDataset);

	/* Libera a memória */
	if (transformPath != NULL)
		Dataset_FreeTransform(&featsTransform);
	Arff_Free(&cvDataset);


	/* Retorna OK */
	return 0;
}
