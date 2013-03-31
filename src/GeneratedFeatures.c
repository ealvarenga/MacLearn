#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#include "GeneratedFeatures.h"

static void skipLine (FILE * file)
{
	int c;

	do
	{
		c = fgetc(file);
	} while (c != '\n' && c >= 0);

}

int GeneratedFeatures_LoadMultFeats (char * path, GeneratedFeatureList * genFeatsList, int maxLvl, unsigned long baseFeats)
{
	FILE * file;
	int lvl;
	unsigned long featsCount = 0;
	unsigned long i;
	GeneratedFeature * auxGenFeats;


	/* Abre o arquivo de entrada */
	file = fopen (path, "rt");
	if (file == NULL)
		return 0;

	/* Conta quantas feats v�o ser geradas */
	while (!feof(file))
	{
		/* Se deu erro na leitura do lvl, para a leitura do arquivo e considera que vai at� aqui */
		if (fscanf (file, "%d", &lvl) == EOF)
			break;

		/* Passa para a pr�xima linha */
		skipLine(file);

		/* Se o lvl � maior que o m�ximo desejado, desconsidera esta feature */
		if (lvl > maxLvl)
			continue;

		/* Incrementa o n�mero de features */
		featsCount ++;
	}

	/* Volta o arquivo para o in�cio */
	fseek (file, 0, SEEK_SET);

	printf ("featsCount = %ld\n", featsCount);

	/* Aloca mem�ria para todas as novas features */
	auxGenFeats = (GeneratedFeature *) malloc (sizeof(GeneratedFeature) * featsCount);
	if (auxGenFeats == NULL)
	{
		fclose (file);
		return 0;
	}
	memset (auxGenFeats, 0, sizeof(GeneratedFeature) * featsCount);

	/* L� todas as combina��es */
	for (i=0;i<featsCount;i++)
	{
		/* L� o lvl */
		if (fscanf (file, "%d", &lvl) == EOF)
			break;

		/* Se o lvl � maior que o maxLvl, ignora esta feat */
		if (lvl > maxLvl)
		{
			skipLine(file);
			continue;
		}

		/* Salva o lvl desta feature */
		auxGenFeats[i].lvl = lvl;

		/* pula o divisor */
		fgetc(file);

		/* L� o primeiro inteiro */
		if (fscanf (file, "%ld", &auxGenFeats[i].feat1) == EOF)
			break;

		/* pula o divisor */
		fgetc(file);

		/* L� o segundo inteiro */
		if (fscanf (file, "%ld", &auxGenFeats[i].feat2) == EOF)
			break;

		/* Se o segundo inteiro era -1, vira 0 (para multiplicar pelo bias, que � sempre 1), sen�o, soma o n�mero base de feats */
		if (auxGenFeats[i].feat2 == -1)
		{
			puts ("Feat -1");
			auxGenFeats[i].feat2 = 0;
		}
		else
			auxGenFeats[i].feat2 += baseFeats + 1;	/* Soma a base e mais um para compensar o bias */

		/* Inicializa os dados de normaliza��o */
		auxGenFeats[i].norm.entryCount = 0;
		auxGenFeats[i].norm.maxVal = -DBL_MAX;
		auxGenFeats[i].norm.minVal = DBL_MAX;
		auxGenFeats[i].norm.totalVal = 0;

		/* Passa para a pr�xima linha */
		skipLine(file);
	}

	/* fecha o arquivo */
	fclose (file);

	/* Se n�o leu todas as feats, retorna erro */
	if (i != featsCount)
	{
		free (auxGenFeats);
		return 0;
	}

	/* Salva os dados */
	genFeatsList->genFeats = auxGenFeats;
	genFeatsList->genFeatsCount = featsCount;

	printf ("Lvl = %d\tNewFeats = %lu\n",maxLvl,featsCount);
	/* Retorna OK */
	return 1;
}

/* Tremendo bacalha... esse c�digo � praticamente igual ao de cima, mas fiquei com pregui�a de fazer direito... 00:00 de domingo, d� um desconto */
int GeneratedFeatures_LoadBoolFeats (char * path, GeneratedFeatureList * genFeatsList, int maxLvl, unsigned long baseFeats)
{
	FILE * file;
	int lvl;
	unsigned long featsCount = 0;
	unsigned long i;
	int currChar;
	GeneratedBoolFeature * auxGenFeats;


	/* Abre o arquivo de entrada */
	file = fopen (path, "rt");
	if (file == NULL)
		return 0;

	/* Conta quantas feats v�o ser geradas */
	while (!feof(file))
	{
		/* Se deu erro na leitura do lvl, para a leitura do arquivo e considera que vai at� aqui */
		if (fscanf (file, "%d", &lvl) == EOF)
			break;

		/* Passa para a pr�xima linha */
		skipLine(file);

		/* Se o lvl � maior que o m�ximo desejado, desconsidera esta feature */
		if (lvl > maxLvl)
			continue;

		/* Incrementa o n�mero de features */
		featsCount ++;
	}

	/* Volta o arquivo para o in�cio */
	fseek (file, 0, SEEK_SET);

	printf ("BoolFeatsCount = %ld\n", featsCount);

	/* Aloca mem�ria para todas as novas features */
	auxGenFeats = (GeneratedBoolFeature *) malloc (sizeof(GeneratedBoolFeature) * featsCount);
	if (auxGenFeats == NULL)
	{
		fclose (file);
		return 0;
	}
	memset (auxGenFeats, 0, sizeof(GeneratedBoolFeature) * featsCount);

	/* L� todas as combina��es */
	for (i=0;i<featsCount;i++)
	{
		/* L� o lvl */
		if (fscanf (file, "%d", &lvl) == EOF)
			break;

		/* Se o lvl � maior que o maxLvl, ignora esta feat */
		if (lvl > maxLvl)
		{
			skipLine(file);
			continue;
		}

		/* Salva o lvl desta feature */
		auxGenFeats[i].lvl = lvl;

		/* pula o divisor */
		fgetc(file);

		/* L� o primeiro byte da linha - tem que ser um F */
		currChar = fgetc(file);
		if (currChar != 'F')
		{
			printf ("Nao era F... '%c' - %d\n",currChar, currChar);
			break;
		}

		/* L� o primeiro inteiro (corresponde ao n�mero da feature de refer�ncia) */
		if (fscanf (file, "%ld", &auxGenFeats[i].feat1) == EOF)
		{
			printf ("Erro no fscanf\n");
			break;
		}

		/* L� o operador */
		/* TODO: S� considero os operadores <= e >, pois s�o os �nicos que aparecem no nosso arquivo */
		currChar = fgetc(file);
		if (currChar == '>')
			auxGenFeats[i].op = GF_OP_GT;
		else if (currChar == '<')
		{
			auxGenFeats[i].op = GF_OP_LE;
			/* L� o "=" */
			fgetc(file);
		}
		else
		{
			printf ("Operador errado... '%c' - %d\n",currChar, currChar);
			break;
		}

		/* L� o valor de refer�ncia */
		if (fscanf (file, "%lf", &auxGenFeats[i].referenceValue) == EOF)
		{
			printf ("Erro no segundo fscanf\n");
			break;
		}

		/* L� o pr�ximo char. Se for um '\n', acabou a linha e o feat2 ficar� como -1. */
		currChar = fgetc(file);
		if (currChar == '\n')
		{
			auxGenFeats[i].feat2 = -1;
			continue;
		}

		/* N�o era '\n', ent�o tem que ser '&' e o pr�ximo tem que ser 'G' */
		if (currChar != '&' || fgetc(file) != 'G')
		{
			printf ("Nao era & ou n�o era G... '%c' - %d   linha = %lu\n",currChar, currChar,i);
			break;
		}

		/* L� o segundo inteiro (corresponde ao �ndice da feature gerada nesse passo que deve ser considerada em conjunto com a atual) */
		if (fscanf (file, "%ld", &auxGenFeats[i].feat2) == EOF)
		{
			printf ("Erro no terceiro fscanf\n");
			break;
		}


		/* Soma a base */
		auxGenFeats[i].feat2 += baseFeats + 1;

		/* Passa para a pr�xima linha */
		skipLine(file);
	}

	/* fecha o arquivo */
	fclose (file);

	/* Se n�o leu todas as feats, retorna erro */
	if (i != featsCount)
	{
		free (auxGenFeats);
		return 0;
	}

	/* Salva os dados */
	genFeatsList->genBoolFeats = auxGenFeats;
	genFeatsList->genBoolFeatsCount = featsCount;

	printf ("Lvl = %d\tNewBoolFeats = %lu\n",maxLvl,featsCount);
	/* Retorna OK */
	return 1;
}

int GeneratedFeatures_Load (char * path, char * boolPath, GeneratedFeatureList * genFeatsList, int maxLvl, int maxBoolLvl, unsigned long baseFeats)
{
	/* Zera a estrutura de feats geradas */
	memset (genFeatsList, 0, sizeof(GeneratedFeatureList));
	/* Se passou um arquivo de features multiplicadas, gera estas features */
	if (path != NULL)
	{
		if (!GeneratedFeatures_LoadMultFeats(path, genFeatsList, maxLvl, baseFeats))
			return 0;
	}
	/* Se passou um arquivo de features booleanas, gera estas features */
	if (boolPath != NULL)
	{
		if (!GeneratedFeatures_LoadBoolFeats(boolPath, genFeatsList, maxBoolLvl, baseFeats + genFeatsList->genFeatsCount))
		{
			/* Se deu erro neste passo mas n�o no anterior, libera a mem�ria j� alocada */
			if (genFeatsList->genFeats != NULL)
				free (genFeatsList->genFeats);
			return 0;
		}
	}

	/* Retorna OK SSSS*/
	return 1;
}
/* TODO: Implementar um "free" */