#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "Arff.h"
#include "MatrixUtil.h"

void Arff_Free (DataSet * dataset)
{
	if (dataset != NULL)
	{
		/* Se for um dataset online, tem mais coisas a fazer */
		if (dataset->type == DS_TYPE_ONLINE)
		{
			/* Libera a cópia do FeatsTransform armazenada */
			if (dataset->online.featsTransform != NULL)
			{
				Dataset_FreeTransform(dataset->online.featsTransform);
				free (dataset->online.featsTransform);
			}
			/* Fecha o arquivo */
			if (dataset->online.file != NULL)
				fclose (dataset->online.file);
		}

		if (dataset->entries != NULL)
		{
			/* a posição 0 aponta para a base do buffer completo. Libera ele */
			if (dataset->entries[0].features != NULL)
				free (dataset->entries[0].features);

			/* Libera o vetor */
			free (dataset->entries);
		}

		/* Libera o vetor de ordem de leitura */
		free (dataset->readOrder);

		/* Zera o conteúdo */
		memset (dataset, 0, sizeof (DataSet));
	}

}

static int Arff_ReadLine (FILE * datasetFile, EntryData * entry, unsigned long featsCount)
{
	unsigned long i;

	/* Lê os atributos de cada registro */
	for (i=0;i<featsCount;i++)
	{
		if (fscanf (datasetFile, "%lf", &entry->features[i]) == EOF)
			return DS_WARN_EOF;
		fgetc (datasetFile);
	}

	/* Lê a classe do atributo */
	if (fscanf (datasetFile, "%d", &entry->classe) == EOF)
		return DS_WARN_EOF;
	fgetc (datasetFile);

	/* Se chegou no fim do arquivo, retorna EOF */
	if (feof (datasetFile))
		return DS_WARN_EOF;

	/* Se deu erro de leitura, retorna erro */
	if (ferror (datasetFile))
		return DS_ERR_FILE;

	/* Retorna OK */
	return DS_OK;
}



/* Fiz funções separadas para o online para o batch para evitar um if a cada linha do dataset (450k ifs fazem diferença =)) */
static int Arff_ResetDataset_Online (DataSet * dataset)
{
	/* Para datasets online, volta o arquivo para a posição de início dos dados */
	if (fseeko (dataset->online.file, dataset->online.dataStartPos, SEEK_SET) != 0)
		return DS_ERR_FILE;
	
	return DS_OK;
}


static int Arff_ResetDataset_Batch (DataSet * dataset)
{
	/* Para datasets Batch, basta zerar currentPos */
	dataset->batch.currentPos = 0;

	/* Retorna ok */
	return DS_OK;
}

static int Arff_NextEntry_Online (DataSet * dataset, EntryData ** entry)
{
	int ret;
	/* TODO: posiciona o dataset baseado no vetor readOrder */

	/* Lê uma linha do arquivo */
	if (dataset->online.featsTransform == NULL)
	{
		ret = Arff_ReadLine(dataset->online.file, dataset->entries, dataset->featsCount);
	}
	else
	{
		ret = Arff_ReadLine(dataset->online.file, dataset->entries, dataset->online.featsTransform->featsInCount);
		/* Transforma depois de ler */
		if (ret == DS_OK) 
			Dataset_ProcessEntry(dataset->entries, dataset->online.featsTransform);
	}
	
	/* Se deu erro, retorna o erro */
	if (ret != DS_OK)
		return ret;

	*entry = dataset->entries;

	/* Retorna ok */
	return DS_OK;
}

static int Arff_NextEntry_Batch (DataSet * dataset, EntryData ** entry)
{
	unsigned long nextPos;

	/* Testa se já leu o dataset inteiro */
	if (dataset->batch.currentPos >= dataset->entriesCount)
		return DS_WARN_EOF;

	/* Para datasets Batch, basta retornar a posição na memória, baseado no vetor de ordem */
	nextPos = dataset->readOrder[dataset->batch.currentPos++];
	*entry = &dataset->entries[nextPos];

	/* Retorna ok */
	return DS_OK;
}

static int Arff_LoadDataset_Batch (FILE * datasetFile, DataSet * dataset)
{
	unsigned long i;
	double * data;
	EntryData * auxEntries;
	int ret;

	/* Aloca memória para ler todo o dataset em um bloco contíguo - necessário para rodar o PCA, e é mais eficiente de qq forma */
	data = (double *) malloc (sizeof(double) * (dataset->entriesCount * dataset->featsCount));
	if (data == NULL)
		return DS_ERR_OUTOFMEMORY;

	/* Aloca um array para guardar as linhas  */
	auxEntries = (EntryData *) malloc (sizeof(EntryData) * dataset->entriesCount);
	if (auxEntries == NULL)
	{
		free (data);
		return DS_ERR_OUTOFMEMORY;
	}

	/* Zera todas as posições */
	memset (auxEntries, 0, sizeof(int *) * dataset->entriesCount);
	memset (data, 0, sizeof(double) * (dataset->entriesCount * dataset->featsCount));

	/* Lê todos os registros */
	for (i=0;i<dataset->entriesCount;i++)
	{
		/* Aponta essa linha para a posição do buffer data que deve ser usada */
		auxEntries[i].features = data + (dataset->featsCount * i);

		/* Lê uma linha do dataset */
		ret = Arff_ReadLine (datasetFile, &auxEntries[i], dataset->featsCount);
		if (ret != DS_OK)
		{
			free (data);
			free (auxEntries);
			return ret;
		}
	}

	/* Salva as variáveis de retorno */
	dataset->entries = auxEntries;

	/* Retorna OK */
	return DS_OK;
}

int Arff_LoadDataset (char * datasetName, DataSet * dataset, DataSetType type)
{
	FILE * datasetFile;
	char auxTxt[11];
	off_t dataStartPos;
	off_t lastAttributePos = 0;
	int ret;
	
	/* Testa se o tipo pedido é válido */
	if (type != DS_TYPE_ONLINE && type != DS_TYPE_BATCH)
		return DS_ERR_PARAM;

	/* Zera o conteúdo do dataset */
	memset (dataset, 0, sizeof (DataSet));

	/* Inicializa as variáveis do dataset */
	dataset->entriesCount = 0;
	dataset->featsCount = 0;
	dataset->type = type;

	/* Tenta abrir o arquivo passado */
	datasetFile = fopen (datasetName, "rb");
	if (datasetFile == NULL)
	{	
		perror ("Erro lendo arquivo");
		return DS_ERR_FILENOTFOUND;
	}

	printf ("Quantidade de atributos....");
	fflush (stdout);
	/* Só aceita datasets com tipo NUMBER (já que é só isso que a gente precisa mesmo) */
	while (fgets (auxTxt, sizeof(auxTxt), datasetFile))
	{
		/* Se o que eu li foi um atributo conta ele e continua */
		if (strncmp (auxTxt, "@ATTRIBUTE", 10) == 0)
		{
			dataset->featsCount++;
			lastAttributePos = ftello (datasetFile);
			continue;
		}
		/* Se foi o @data acabaram os atributos */
		if (strncmp (auxTxt, "@DATA", 5) == 0)
			break;
	}

	if (feof(datasetFile) || ferror(datasetFile) || dataset->featsCount == 0)
	{
		/* Acabou o arquivo de dataset ou deu erro de leitura */
		fclose (datasetFile);
		return DS_ERR_FILE;
	}
	
	/* Retira um atributo da contagem, pois é a classe do registro */
	dataset->featsCount--;

	printf ("%ld\n", dataset->featsCount);
	fflush (stdout);

	/* Testas se leu um \n junto com @data */
	if (auxTxt[strlen(auxTxt) - 1] != '\n')
		while (fgetc(datasetFile) != '\n');

	/* Marca a posição do inicio dos dados para voltar pra cá depois */
	dataStartPos = ftello (datasetFile);

	/* Começa a contagem das classes */
	printf ("Quantidade de classes no dataset...");
	fflush (stdout);

	/* Volta na posição do último atributo para contar a quantidade de classes */
	fseeko (datasetFile, lastAttributePos, SEEK_SET);
	/* Chega até o { para começar a contar as classes */
	while (fgetc(datasetFile) != '{');
	dataset->classesCount = 0;
	/* Conta a quantidade de classes */
	while (1)
	{
		ret = fgetc(datasetFile);
		if (ret == ',' || ret == '}')
			dataset->classesCount++;
		if (ret == '}')
			break;
	}

	printf ("%ld\n", dataset->classesCount);
	fflush (stdout);

	/* Volta o ponteiro para a posição do inicio dos dados para contar a quantidade de registros */
	fseeko (datasetFile, dataStartPos, SEEK_SET);

	/* Conta a quantidade de "newlines" para determinar o tamanho do dataset */
	printf ("Quantidade de entradas no dataset...");
	fflush (stdout);

	dataset->entriesCount = 0;
	while (!feof(datasetFile) && !ferror(datasetFile))
	{
		if (fgetc(datasetFile) == '\n')
			dataset->entriesCount++;
	}

	/* Volta o arquivo para o inicio dos dados */
	fseeko (datasetFile, dataStartPos, SEEK_SET);
	printf ("%ld\n", dataset->entriesCount);
	fflush (stdout);

	/* Aloca o ponteiro com a ordem de leitura do dataset */
	dataset->readOrder = (unsigned long *) malloc (sizeof(unsigned long) * dataset->entriesCount);
	if (dataset->readOrder == NULL)
	{
		fclose(datasetFile);
		return DS_ERR_OUTOFMEMORY;
	}

	/* Ordena o dataset */
	Arff_SortDataset(dataset);

	/* Testa o tipo de dataset */
	if (type == DS_TYPE_BATCH)
	{
		printf ("Lendo dados....");
		fflush (stdout);

		/* Lê o dataset todo de modo batch */
		ret = Arff_LoadDataset_Batch(datasetFile, dataset);
		printf ("OK\n");
		fflush (stdout);
		
		/* Terminou de ler, fecha o dataset */
		fclose (datasetFile);

		/* Se deu algum erro, retorna o erro */
		if (ret != DS_OK)
		{
			free (dataset->readOrder);
			return ret;
		}

		/* Salva a versão batch das funções */
		dataset->nextEntry = Arff_NextEntry_Batch;
		dataset->reset = Arff_ResetDataset_Batch;
	}
	else
	{
		/* Online Dataset */
		/* Um dataset online deve ter um "entry" */
		dataset->entries = (EntryData *) malloc (sizeof(EntryData));
		if (dataset->entries == NULL)
		{
			free (dataset->readOrder);
			return DS_ERR_OUTOFMEMORY;
		}

		/* Este entry tem que ter espaço para guardar uma linha */
		dataset->entries[0].features = (double *) malloc (sizeof(double) * dataset->featsCount);
		if (dataset->entries[0].features == NULL)
		{
			free (dataset->entries);
			free (dataset->readOrder);
			return DS_ERR_OUTOFMEMORY;
		}

		/* Salva as informações do arquivo */
		dataset->online.dataStartPos = dataStartPos;
		dataset->online.file = datasetFile;

		/* Salva a versão online das funções */
		dataset->nextEntry = Arff_NextEntry_Online;
		dataset->reset = Arff_ResetDataset_Online;
	}
	
	/* Salva as funções comuns para Online e Batch */
	dataset->shuffle = Dataset_ShuffleDataset;
	dataset->sort = Dataset_SortDataset;

	/* Retorna OK */
	return DS_OK;
}

int Arff_SaveDataset (char * datasetName, DataSet * dataset)
{
	EntryData * entry;
	FILE * file;
	int i;

	/* Abre o arquivo de saída */
	file = fopen (datasetName, "wb");
	if (file == NULL)
		return DS_ERR_FILE;

	/* Grava o cabecalho ARFF */
	fprintf (file, "@relation GENERATED_RELATION\n");
	/* Grava o nome dos atributos */
	for (i=0;i<dataset->featsCount;i++)
		fprintf(file, "@ATTRIBUTE bit%d  NUMERIC\n", i + 1);
	/* Grava as classes */
	fprintf(file, "@ATTRIBUTE classe  {1");
	for (i=1;i<dataset->classesCount;i++)
		fprintf(file, ",%d", i+1);
	fprintf(file, "}\n\n");
	fprintf(file, "@DATA\n");

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
