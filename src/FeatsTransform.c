/* Conte�do do .h */

/* Guarda as informa��es de uma coluna */
typedef struct
{
	double totalVal;
	unsigned long entryCount;
	double maxVal;
	double minVal;
}FeatureNorm;

/* Guarda as informa��es de transforma��o das features */
typedef struct  
{
	unsigned long featsInCount;
	FeatureNorm * featsNorm;
	unsigned long featsAfterNormalize;
	unsigned char runPCA;
	double * PCAData;
	unsigned long featsAfterPCA;
}FeatsTransform;

int Dataset_LearnTransform (DataSet * dataset, FeatsTransform * featsTransform);
int Dataset_ProcessDataset (DataSet * dataset, FeatsTransform * featsTransform);
int Dataset_ProcessEntry (EntryData * entry, FeatsTransform * featsTransform);
void Dataset_FreeTransform (FeatsTransform * featsTransform);
int Dataset_SaveTransform (char * fileName, FeatsTransform * featsTransform);
int Dataset_LoadTransform (char * fileName, FeatsTransform * featsTransform);

/*********************************************/




#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <time.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include "Dataset.h"
#include "MatrixUtil.h"

static void Dataset_UpdateFeatureNorm (FeatureNorm * norm, double val)
{
	/* Atualiza os campos da estrutura FeatureNorm com base no valor passado */
	if (val < norm->minVal)
		norm->minVal = val;
	if (val > norm->maxVal)
		norm->maxVal = val;
	norm->entryCount++;
	norm->totalVal += val;
}

static void Dataset_InitFeatureNorm (FeatureNorm * norm, unsigned long count)
{
	unsigned long i;

	for (i=0;i<count;i++)
	{
		norm[i].minVal = DBL_MAX;
		norm[i].maxVal = -DBL_MAX;
		norm[i].entryCount = 0;
		norm[i].totalVal = 0;
	}
}

static void Dataset_PCAFeats (double * featsIn, FeatsTransform * featsTransform, double * featsOut)
{
	gsl_matrix_view entryMatrix;				/* [1 x n] == [1 x afterNormalize] */
	gsl_matrix_view PCAMatrix;					/* [n X k] == [afterNormalize X afterPCAFeats] */
	gsl_matrix_view outMatrix;					/* [1 x k] == [1 x afterPCAFeats] */
	int ret;
	//	static i = 1;

	/* inicializa o vetor e a matriz */
	entryMatrix = gsl_matrix_view_array(featsIn, 1, featsTransform->featsAfterNormalize);
	PCAMatrix = gsl_matrix_view_array(featsTransform->PCAData, featsTransform->featsAfterNormalize, featsTransform->featsAfterPCA);
	outMatrix = gsl_matrix_view_array(featsOut, 1, featsTransform->featsAfterPCA);

	/*
	if (i)
	{
	printMatrix("entryMatrix", &entryMatrix.matrix);
	printMatrix("PCAMatrix", &PCAMatrix.matrix);
	}
	*/
	/* Multiplica as matrizes */
	ret = gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, &entryMatrix.matrix, &PCAMatrix.matrix, 0.0, &outMatrix.matrix);
	if (ret != 0)
		printf ("Erro no BLAS: %d\n", ret);

	/*
	if (i)
	{
	i =0;
	printMatrix("outMatrix", &outMatrix.matrix);
	}
	*/
}

static void Dataset_NormalizeFeats (double * featsIn, FeatsTransform * featsTransform, double * featsOut)
{
	unsigned long i, j;

	/* Processa cada feature */
	for (i=0,j=0;i<featsTransform->featsInCount;i++)
	{
		/* Se o min == max, a feature � in�til e deve ser descartada */
		if (featsTransform->featsNorm[i].minVal == featsTransform->featsNorm[i].maxVal)
			continue;
		/* Se n�o foi descartada, faz a normaliza��o e salva na sa�da */
		featsOut[j++] = (featsIn[i] - (featsTransform->featsNorm[i].totalVal/featsTransform->featsNorm[i].entryCount))/(featsTransform->featsNorm[i].maxVal - featsTransform->featsNorm[i].minVal);
	}
}


static int Dataset_LearnNormalization (DataSet * dataset, FeatsTransform * featsTransform)
{
	FeatureNorm *auxInfo;
	unsigned long i;
	EntryData * entry;
	int ret;

	/* Aloca memoria para as features info */
	auxInfo = (FeatureNorm *) malloc (sizeof(FeatureNorm) * dataset->featsCount);
	if (auxInfo == NULL)
		return DS_ERR_OUTOFMEMORY;

	/* inicializa os dados das colunas */
	Dataset_InitFeatureNorm(auxInfo, dataset->featsCount);

	/* Garante que o dataset est� no come�o */
	dataset->reset(dataset);

	/* Processa todos os exemplos para aprender as infos */
	while ((ret = dataset->nextEntry(dataset, &entry)) == DS_OK)
	{
		/* Aprende cada uma das features */
		for (i=0;i<dataset->featsCount;i++)
			Dataset_UpdateFeatureNorm(&auxInfo[i], entry->features[i]);
	}

	/* Testa se deu algum erro durante a leitura do dataset */
	if (ret != DS_WARN_EOF)
	{
		printf ("LearnNorm: Erro no dataset - %d\n", ret);
		return ret;
	}

	/* Retorna as informa��es dos atributos */
	featsTransform->featsNorm = auxInfo;
	featsTransform->featsInCount = dataset->featsCount;
	featsTransform->featsAfterNormalize = 0;

	/* Conta s� as colunas que n�o v�o ser desprezadas */
	for (i=0;i<featsTransform->featsInCount;i++)
	{
		if (featsTransform->featsNorm[i].minVal == featsTransform->featsNorm[i].maxVal)
			continue;

		/* Incrementa um na sa�da */
		featsTransform->featsAfterNormalize++;
	}

	/* Retorna OK */
	return DS_OK;
}

static int Dataset_NormalizeDataset (DataSet * dataset, FeatsTransform * featsTransform)
{
	unsigned long i;
	double *nextFeatsPos;
	double *aux;

	/* Se o dataset � online, nada a fazer */
	if (dataset->type == DS_TYPE_ONLINE)
		return DS_OK;

	/* Se j� estava normalizado, retorna OK */
	if (dataset->batch.normalized)
		return DS_OK;

	/* Inicializa a posi��o para gravar os dados da primeira linha */
	nextFeatsPos = dataset->entries[0].features;

	/* i varia de 0 at� m, onde m � o total de exemplos do dataset */
	for (i=0;i<dataset->entriesCount;i++)
	{
		/* Para cada exemplo, processa as featues */
		Dataset_NormalizeFeats(dataset->entries[i].features, featsTransform, nextFeatsPos);

		/* Atualiza a base desta linha */
		dataset->entries[i].features = nextFeatsPos;

		/* Salva a posi��o de in�cio da pr�xima linha */
		nextFeatsPos = dataset->entries[i].features + featsTransform->featsAfterNormalize;
	}

	/* Realloca data para liberar a mem�ria desnecess�ria */
	if (featsTransform->featsInCount != featsTransform->featsAfterNormalize)
	{
		/* Em tese, aux sempre ser� igual ao entries[0].features, j� que estou diminuindo o tamanho */
		aux = (double *) realloc (dataset->entries[0].features, sizeof(double) * (dataset->entriesCount * featsTransform->featsAfterNormalize));
		if (aux != dataset->entries[0].features)
			return DS_ERR_OUTOFMEMORY;
	}

	/* Salva a nova quantidade de features do dataset */
	dataset->featsCount = featsTransform->featsAfterNormalize;

	/* Marca que o dataset est� normalizado */
	dataset->batch.normalized = 1;

	return DS_OK;
}

static int Dataset_LearnPCA (DataSet * dataset, FeatsTransform * featsTransform)
{
	gsl_matrix_view datasetMatrix;				/* [m x n] == [entriesCount X featsCount] */
	gsl_matrix_view sigma, V;					/* [n X n] == [featsCount X featsCount] */
	gsl_matrix_view auxOut;						/* Auxiliar s� para salvar o PCA */
	gsl_vector_view vectorS, work, entryVect;	/* [1 X n] == [1 X featsCount] */
	double * sigmaData;
	double * VData;
	double * SData;
	double * workData;
	double totalSum, currentSum;
	unsigned long i;
	int ret;
	EntryData * entry;

	/* Aloca mem�ria para armazenar as matrizes e vetores */
	sigmaData = (double *) malloc (sizeof(double) * dataset->featsCount * dataset->featsCount);
	VData = (double *) malloc (sizeof(double) * dataset->featsCount * dataset->featsCount);
	SData = (double *) malloc (sizeof(double) * dataset->featsCount);
	workData = (double *) malloc (sizeof(double) * dataset->featsCount);

	/* Se deu erro em alguma, d� free em todas e retorna erro */
	if (sigmaData == NULL || VData == NULL || SData == NULL || workData == NULL)
	{
		if (sigmaData) free (sigmaData);
		if (VData) free (VData);
		if (SData) free (SData);
		if (workData) free (workData);
		return DS_ERR_OUTOFMEMORY;
	}

	/* Monta as matrizes e vetores no formato do GSL */
	sigma = gsl_matrix_view_array(sigmaData, dataset->featsCount, dataset->featsCount);
	V = gsl_matrix_view_array(VData, dataset->featsCount, dataset->featsCount);
	vectorS = gsl_vector_view_array(SData, dataset->featsCount);
	work = gsl_vector_view_array(workData, dataset->featsCount);

	/* Calcula a matriz de covariancia */
	if (dataset->type == DS_TYPE_BATCH)
	{
		/* Tenho a matriz inteira na mem�ria. Calcula por multiplica��o de matrizes */
		datasetMatrix = gsl_matrix_view_array(dataset->entries[0].features, dataset->entriesCount, dataset->featsCount);
		ret = gsl_blas_dgemm (CblasTrans, CblasNoTrans, 1.0/dataset->entriesCount, &datasetMatrix.matrix, &datasetMatrix.matrix, 0.0, &sigma.matrix);
		//		printMatrix("Sigma", &sigma.matrix);
	}
	else
	{
		/* Dataset online - Tenho que calcular de maneira iterativa */
		/* Zera os valores de sigma */
		memset (sigmaData, 0, sizeof(double) * dataset->featsCount * dataset->featsCount);

		/* Garante que o dataset est� no in�cio */
		dataset->reset(dataset);

		printf ("dataset->featsCount = %lu\n"
			"dataset->online.featsTransform->featsInCount = %lu\n"
			"dataset->online.featsTransform->normFeatsCount = %lu\n"
			"featsTransform->normFeatsCount = %lu\n",
			dataset->featsCount,
			dataset->online.featsTransform->featsInCount,
			dataset->online.featsTransform->featsAfterNormalize,
			featsTransform->featsAfterNormalize);

		/* Processa todos os registros */
		while ((ret = dataset->nextEntry(dataset, &entry)) == DS_OK)
		{
			/* Salva o array */
			entryVect = gsl_vector_view_array(entry->features, dataset->featsCount);

			/* Atualiza sigma */
			gsl_blas_dger (1.0/dataset->entriesCount, &entryVect.vector, &entryVect.vector, &sigma.matrix);
		}
		//printVect("Entry", &entryVect.vector);
		//printMatrix("Sigma", &sigma.matrix);

		/* Se deu erro lendo o dataset, retorna o erro */
		if (ret != DS_WARN_EOF)
		{
			if (sigmaData) free (sigmaData);
			if (VData) free (VData);
			if (SData) free (SData);
			if (workData) free (workData);
			return ret;
		}
		ret = 0;
	}
	/* Se foi ok, calcula o svd */
	if (ret == 0)
		ret = gsl_linalg_SV_decomp (&sigma.matrix, &V.matrix, &vectorS.vector, &work.vector);

	/* Independente de dar erro, n�o vou precisar do work nem do V */
	free (VData);
	free (workData);

	/* Se deu algum erro, libera as vari�veis e retorna erro */
	if (ret != 0)
	{
		free (sigmaData);
		free (SData);
		return DS_ERR_GSL;
	}

	/* N�o deu erro, ent�o imprime U e S */
	//	printVect("S", &vectorS.vector);
	//	printMatrix("U", &sigma.matrix);

	/* Soma todas as posi��es de S */
	for (i=0,totalSum=0;i<dataset->featsCount;i++)
		totalSum += SData[i];

	/* Calcula quantas feats para manter a variancia de 99% */
	for (i=0,currentSum=0;i<dataset->featsCount;i++)
	{
		currentSum += SData[i];
		if ((currentSum/totalSum) > 0.99d)
			break;
	}

	/* Salva a quantidade de features mantidas */
	featsTransform->featsAfterPCA = i+1;
	//	printf ("Com %lu features: %.3lf%% de variancia mantida\n", featsTransform->featsAfterPCA, (currentSum/totalSum) * 100);

	/* N�o preciso mais do S */
	free (SData);

	/* Aloca mem�ria para salvar os dados de PCA */
	featsTransform->PCAData = (double *) malloc (sizeof(double) * dataset->featsCount * featsTransform->featsAfterPCA);
	if (featsTransform->PCAData == NULL)
	{
		free (sigmaData);
		return DS_ERR_OUTOFMEMORY;
	}

	/* Inicializa uma matriz auxiliar  [N x featsTransform->featsAfterPCA]*/
	auxOut = gsl_matrix_view_array(featsTransform->PCAData, dataset->featsCount, featsTransform->featsAfterPCA);

	/* Copia o sigmaData, que virou o "U" do SVD e vai ser usado para calcular as features "resumidas" */
	if (!copyMatrix(&auxOut.matrix, &sigma.matrix, dataset->featsCount, featsTransform->featsAfterPCA))
	{
		free (sigmaData);
		free (featsTransform->PCAData);
		return DS_ERR_GSL;
	}

	/* Libera a mem�ria de sigmaData */
	free (sigmaData);

	/* Retorna OK */
	return DS_OK;
}

static int Dataset_RunPCA (DataSet * dataset, FeatsTransform * featsTransform)
{
	double * nextFeatsPos;
	double * aux;
	unsigned long i;

	/* Se for dataset online, nada a fazer */
	if (dataset->type == DS_TYPE_ONLINE)
		return DS_OK;

	/* PCA j� foi executado */
	if (dataset->batch.PCAed)
		return DS_OK;

	/* Aloca uma mem�ria auxiliar para guardar o processamento de cada linha */
	aux = (double *) malloc (sizeof(double) * featsTransform->featsAfterPCA);
	if (aux == NULL)
		return DS_ERR_OUTOFMEMORY;

	/* Inicializa a posi��o para gravar os dados da primeira linha */
	nextFeatsPos = dataset->entries[0].features;

	/* i varia de 0 at� m, onde m � o total de exemplos do dataset */
	for (i=0;i<dataset->entriesCount;i++)
	{
		/* Para cada exemplo, processa as features */
		Dataset_PCAFeats(dataset->entries[i].features, featsTransform, aux);

		/* Atualiza a base desta linha */
		dataset->entries[i].features = nextFeatsPos;

		/* Atualiza os dados de features[i] */
		memcpy (dataset->entries[i].features, aux, sizeof(double) * featsTransform->featsAfterPCA);

		/* Imprime as primeiras 10 linhas */
		//		if (i < 10)
		//			printf ("%lf %lf %lf %lf %lf\n",dataset->entries[i].features[0],dataset->entries[i].features[1],dataset->entries[i].features[2],dataset->entries[i].features[3],dataset->entries[i].features[4]);

		/* Salva a posi��o de in�cio da pr�xima linha */
		nextFeatsPos = dataset->entries[i].features + featsTransform->featsAfterPCA;
	}

	/* Realloca data para liberar a mem�ria desnecess�ria */
	if (featsTransform->featsAfterNormalize != featsTransform->featsAfterPCA)
	{
		/* Em tese, aux sempre ser� igual ao entries[0].features, j� que estou diminuindo o tamanho */
		aux = (double *) realloc (dataset->entries[0].features, sizeof(double) * (dataset->entriesCount * featsTransform->featsAfterPCA));
		if (aux != dataset->entries[0].features)
			return DS_ERR_OUTOFMEMORY;
	}

	/* Atualiza o numero de features */
	dataset->featsCount = featsTransform->featsAfterPCA;

	/* Salva que j� rodou o PCA no dataset */
	dataset->batch.PCAed = 1;

	/* Retorna OK */
	return DS_OK;
}

void Dataset_FreeTransform (FeatsTransform * featsTransform)
{
	/* Libera as informa��es das colunas */
	if (featsTransform != NULL)
	{
		/* Libera a mem�ria */
		if (featsTransform->featsNorm != NULL)
			free (featsTransform->featsNorm);

		if (featsTransform->PCAData != NULL)
			free (featsTransform->PCAData);

		/* Zera o conte�do */
		memset (featsTransform, 0, sizeof(FeatsTransform));
	}
}

int Dataset_ProcessEntry (EntryData * entry, FeatsTransform * featsTransform)
{
	double * aux;

	/* Aloca uma mem�ria auxiliar para guardar o processamento do PCA de cada linha */
	aux = (double *) malloc (sizeof(double) * featsTransform->featsAfterPCA);
	if (aux == NULL)
		return DS_ERR_OUTOFMEMORY;


	/* Processa uma entrada apenas - Para ser usado com o dataset online */
	Dataset_NormalizeFeats(entry->features, featsTransform, entry->features);

	/* Roda o PCA em cima destas features apenas */
	Dataset_PCAFeats (entry->features, featsTransform, aux);

	/* Atualiza os dados de entry */
	memcpy (entry->features, aux, sizeof(double) * featsTransform->featsAfterPCA);

	/* Libera aux */
	free (aux);

	/* Retorna OK */
	return DS_OK;
}

static int Dataset_CopyTransform (FeatsTransform * dst, FeatsTransform * src)
{
	size_t pcaSize;
	/* Copia todas as vari�veis */
	memcpy (dst, src, sizeof(FeatsTransform));

	/* Aloca mem�ria para os dados de normaliza��o */
	dst->featsNorm = (FeatureNorm *) malloc (sizeof(FeatureNorm) * dst->featsInCount);
	if (dst->featsNorm == NULL)
		return DS_ERR_OUTOFMEMORY;

	/* Copia os dados de normaliza��o */
	memcpy (dst->featsNorm, src->featsNorm, sizeof(FeatureNorm) * dst->featsInCount);

	/* Processa os dados de PCA, se existirem */
	if (dst->runPCA)
	{
		pcaSize = sizeof(double) * dst->featsAfterNormalize * dst->featsAfterPCA;

		/* Aloca mem�ria para os dados do PCA */
		dst->PCAData = (double *) malloc (pcaSize);
		if (dst->PCAData == NULL)
		{
			free (dst->featsNorm);
			return DS_ERR_OUTOFMEMORY;
		}

		/* Copia os dados do PCA */
		memcpy (dst->PCAData, src->PCAData, pcaSize);

	}

	/* Retorna OK */
	return DS_OK;
}

int Dataset_ProcessDataset (DataSet * dataset, FeatsTransform * featsTransform)
{
	int ret;

	if (dataset->type == DS_TYPE_ONLINE)
	{
		/* Se o dataset � online, aloca um featsTransform e copia os dados para ele */
		dataset->online.featsTransform = (FeatsTransform *) malloc (sizeof(FeatsTransform));
		if (dataset->online.featsTransform == NULL)
			return DS_ERR_OUTOFMEMORY;

		/* Copia os dados de transforma��o para o dataset */
		ret = Dataset_CopyTransform(dataset->online.featsTransform, featsTransform);
		if (ret != DS_OK)
		{
			free (dataset->online.featsTransform);
			return ret;
		}

		/* Retorna OK */
		return DS_OK;
	}

	/* Se n�o � online, normaliza o dataset */
	ret = Dataset_NormalizeDataset(dataset, featsTransform);
	if (ret != DS_OK)
		return ret;

	/* Se necess�rio, roda o PCA */
	if (featsTransform->runPCA)
	{
		ret = Dataset_RunPCA (dataset, featsTransform);
		if (ret != DS_OK)
			return ret;
	}

	return DS_OK;
}

int Dataset_LearnTransform (DataSet * dataset, FeatsTransform * featsTransform)
{
	int ret;
	FeatsTransform * auxFeatsTransform;
	unsigned long auxFeatsCount;
	unsigned char learnPCA;
	DataSetType dsType;

	/* Salva se deve rodar o PCA ou n�o */
	learnPCA = featsTransform->runPCA;

	/* Zera todos os dados do transform */
	memset (featsTransform, 0, sizeof(FeatsTransform));
	featsTransform->runPCA = (learnPCA != 0);

	/* Aprende a normaliza��o - Esse � obrigat�rio */
	printf ("%ld Aprendendo regras de normaliza��o do dataset...\n", time(NULL));
	ret = Dataset_LearnNormalization(dataset, featsTransform);
	if (ret != DS_OK)
		return ret;

	/* Normaliza para poder aprender os pr�ximos passos */
	printf ("%ld Normalizando dataset...\n", time(NULL));
	ret = Dataset_NormalizeDataset(dataset, featsTransform);
	if (ret != DS_OK)
		return ret;

	/* Se necess�rio, aprende o PCA */
	if (learnPCA)
	{
		dsType = dataset->type;
		if (dsType == DS_TYPE_ONLINE)
		{
			/* Usa temporariamente o featsTransform passado para aprender o PCA  */
			auxFeatsTransform = dataset->online.featsTransform;
			auxFeatsCount = dataset->featsCount;
			dataset->online.featsTransform = featsTransform;
			dataset->featsCount = featsTransform->featsAfterNormalize;
		}
		printf ("%ld Aprendendo PCA...\n", time(NULL));
		ret = Dataset_LearnPCA (dataset, featsTransform);
		if (dsType == DS_TYPE_ONLINE)
		{
			/* Desfaz as altera��es, independente se deu erro ou n�o */
			dataset->online.featsTransform = auxFeatsTransform;
			dataset->featsCount = auxFeatsCount;
		}
		if (ret != DS_OK)
			return ret;
	}

	printf ("%ld Done!\n", time(NULL));
	return DS_OK;
}

int Dataset_SaveTransform (char * fileName, FeatsTransform * featsTransform)
{
	FILE * file;
	size_t pcaSize;

	/* Abre o arquivo de sa�da */
	file = fopen (fileName, "wb");
	if (file == NULL)
		return DS_ERR_FILE;

	/* Grava os dados de featsTransform */
	if (fwrite (featsTransform, sizeof(FeatsTransform), 1, file) != 1)
	{
		fclose (file);
		return DS_ERR_FILE;
	}

	/* Grava os dados de normaliza��o */
	if (fwrite (featsTransform->featsNorm, sizeof(FeatureNorm), featsTransform->featsInCount, file) != featsTransform->featsInCount)
	{
		fclose (file);
		return DS_ERR_FILE;
	}


	/* Grava os dados de PCA, se existirem */
	if (featsTransform->runPCA)
	{
		pcaSize = featsTransform->featsAfterNormalize * featsTransform->featsAfterPCA;
		if (fwrite (featsTransform->PCAData, sizeof(double), pcaSize, file) != pcaSize)
		{
			fclose (file);
			return DS_ERR_FILE;
		}
	}

	/* Fecha o arquivo */
	fclose (file);

	/* Retorna OK */
	return DS_OK;
}

int Dataset_LoadTransform (char * fileName, FeatsTransform * featsTransform)
{
	FILE * file;
	size_t pcaSize;

	/* Abre o arquivo de sa�da */
	file = fopen (fileName, "rb");
	if (file == NULL)
		return DS_ERR_FILE;

	/* Grava os dados de featsTransform */
	if (fread (featsTransform, sizeof(FeatsTransform), 1, file) != 1)
	{
		fclose (file);
		return DS_ERR_FILE;
	}

	/* Aloca mem�ria para os dados de normaliza��o */
	featsTransform->featsNorm = (FeatureNorm *) malloc (sizeof(FeatureNorm) * featsTransform->featsInCount);
	if (featsTransform->featsNorm == NULL)
	{
		fclose (file);
		return DS_ERR_OUTOFMEMORY;
	}

	/* L� os dados de normaliza��o */
	if (fread (featsTransform->featsNorm, sizeof(FeatureNorm), featsTransform->featsInCount, file) != featsTransform->featsInCount)
	{
		free (featsTransform->featsNorm);
		fclose (file);
		return DS_ERR_FILE;
	}


	/* L� os dados de PCA, se existirem */
	if (featsTransform->runPCA)
	{
		pcaSize = featsTransform->featsAfterNormalize * featsTransform->featsAfterPCA;

		/* Aloca mem�ria para os dados do PCA */
		featsTransform->PCAData = (double *) malloc (sizeof(double) * pcaSize);
		if (featsTransform->PCAData == NULL)
		{
			free (featsTransform->featsNorm);
			fclose (file);
			return DS_ERR_OUTOFMEMORY;
		}

		/* L� os dados do PCA */
		if (fread (featsTransform->PCAData, sizeof(double), pcaSize, file) != pcaSize)
		{
			free (featsTransform->featsNorm);
			fclose (file);
			return DS_ERR_FILE;
		}
	}

	/* Fecha o arquivo */
	fclose (file);

	/* Retorna OK */
	return DS_OK;
}

