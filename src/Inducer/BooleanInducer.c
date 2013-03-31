#define EXTEND_FEATINDUCER
#include <stdio.h>		/* For FILE, printf etc */
#include <stdlib.h>		/* For malloc/free */

#include "MacLearn/Inducer/BooleanInducer.h"

/* Create a local var to save references to "super class" functions */
static FeatInducer super;
static unsigned char superInitialized = 0;

/************************
* "Private" Functions	*
************************/
/* Utilitary function to skip to the next line in a file */
static void skipLine (FILE * file)
{
	int c;

	do
	{
		c = fgetc(file);
	} while (c != '\n' && c >= 0);

}
/* Utilitary function to read a character and compare it to an expected value */
static unsigned char readAndCompare (FILE * file, char expected)
{
	int c;

	/* Read the character */
	c = fgetc (file);
	/* Test if reached the end of file */
	if (feof(file))
	{
		printf ("Unexpected end of file reached. Expected '%c'.", expected);
		return 0;
	}
	/* Test for errors */
	if (ferror(file))
	{
		printf ("Error reading the file. Expected '%c'.", expected);
		return 0;
	}

	/* Test if the character read matches the expected */
	if (c != (int) expected)
	{
		printf ("Unexpected character reading rule - '%c'/%d. Expected '%c'.\n",c, c, expected);
		return 0;
	}

	/* Return OK */
	return 1;
}

/* Load the rules for generating feats from a file */
static int BooleanInducer_LoadFeats (BooleanInducer * boolInducer, char * path, unsigned long maxLvl)
{
	FILE * file;
	unsigned long lvl;
	unsigned long i;
	int currChar;
	BooleanRule * auxRules;

	/* Opens the input file */
	file = fopen (path, "rt");
	if (file == NULL)
	{
		errno = ENOENT;
		return ML_ERR_FILENOTFOUND;
	}

	/* Count the number of new feats */
	while (!feof(file))
	{
		/* If there's any problem reading the "level" information, stop the reading and consider this the end of the file */
		if (fscanf (file, "%lu", &lvl) == EOF)
			break;

		/* skip to the next line */
		skipLine(file);

		/* If current level is greater than the maximum level, skip this feature */
		if (lvl > maxLvl)
			continue;

		/* Increment the total generated feats count */
		boolInducer->generatedFeatsCount++;
	}

	/* Reset the file to the beginning */
	fseek (file, 0, SEEK_SET);

	/* Get memory to all new features */
	auxRules = (BooleanRule *) malloc (sizeof(BooleanRule) * boolInducer->generatedFeatsCount);
	if (auxRules == NULL)
	{
		fclose (file);
		errno = ENOMEM;
		return ML_ERR_OUTOFMEMORY;
	}
	/* No need to initialize it, as we'll write valid data to all positions */
	//memset (auxRules, 0, sizeof(BooleanRule) * boolInducer->generatedFeatsCount);

	/* Read all rules */
	for (i=0;i<boolInducer->generatedFeatsCount;i++)
	{
		/* Read level */
		if (fscanf (file, "%lu", &lvl) == EOF)
			break;

		/* If current level is greater than the maximum level, skip this feature */
		if (lvl > maxLvl)
		{
			skipLine(file);
			i--;			/* Decrement index so this position is read again from the file */
			continue;
		}

		/* Save current level */
		auxRules[i].lvl = lvl;

		/* Skip the field separator (ignores which char it actually is) */
		fgetc(file);

		/* Read the first byte of the rule. MUST be an 'F' */
		if (!readAndCompare(file, 'F'))
			break;

		/* Read the first number (corresponds to the feature reference number) */
		if (fscanf (file, "%ld", &auxRules[i].feat1) == EOF)
		{
			printf ("Error reading from file.\n");
			break;
		}

		/* Read the operator */
		currChar = fgetc(file);
		if (currChar == '=')
		{
			/* The next char must be another = */
			if (!readAndCompare(file, '='))
				break;
			/* Save the operation mode */
			auxRules[i].op = BI_OP_EQ;
		}
		else if (currChar == '>')
		{
			/* Save the operation as '>' */
			auxRules[i].op = BI_OP_GT;
			/* Read another char to check if it's actually >= */
			currChar = fgetc(file);
			if (currChar == '=')
				auxRules[i].op = BI_OP_GE;	/* Change to >= */
			else
				ungetc (currChar, file);		/* Not >=, so return the character to the buffer */
		}
		else if (currChar == '<')
		{
			/* Save the operation as '<' */
			auxRules[i].op = BI_OP_LT;
			/* Read another char to check if it's actually <= */
			currChar = fgetc(file);
			if (currChar == '=')
				auxRules[i].op = BI_OP_LE;	/* Change to <= */
			else
				ungetc (currChar, file);		/* Not <=, so return the character to the buffer */
		}
		else if (currChar == '!')
		{
			/* The next char must be a = */
			if (!readAndCompare(file, '='))
				break;
			/* Save the operation mode */
			auxRules[i].op = BI_OP_NE;
		}
		else
		{
			printf ("Unexpected character reading rule - '%c'/%d. Expected an operator.\n\n",currChar, currChar);
			break;
		}

		/* Read the reference value */
		if (fscanf (file, "%lf", &auxRules[i].referenceValue) == EOF)
		{
			printf ("Error reading the reference value\n");
			break;
		}

		/* Read the next char */
		currChar = fgetc(file);
		/* If reached the end of the rule, feat2 stays as -1 */
		if (currChar == '\n')
		{
			auxRules[i].feat2 = -1;
			continue;
		}

		/* Wasn't a new line, so it must be '&' */
		if (currChar != '&')
		{
			printf ("Unexpected character reading rule - '%c'/%d. Expected '&'.\n\n",currChar, currChar);
			break;
		}

		/* The next char must be a 'G' */
		if (!readAndCompare(file, 'G'))
			break;

		/* Read the next integer. Corresponds to the number of a generated feature to be considered along with this one */
		if (fscanf (file, "%ld", &auxRules[i].feat2) == EOF)
		{
			printf ("Error reading feature info\n");
			break;
		}

		/* Skip to the next line */
		skipLine(file);
	}

	/* Close the file */
	fclose (file);

	/* If didn't read all feats, return error */
	if (i != boolInducer->generatedFeatsCount)
	{
		free (auxRules);
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* Save rules */
	boolInducer->booleanRules = auxRules;

	/* Return true (OK) */
	return ML_OK;
}

static void BooleanInducer_Free (BooleanInducer * boolInducer)
{
	/* If there are rules, free it */
	if (boolInducer->booleanRules != NULL)
	{
		free (boolInducer->booleanRules);
		boolInducer->booleanRules = NULL;
	}

	/* Call the "superclass" free function */
	super.free((FeatInducer *) boolInducer);
}

static int BooleanInducer_Generate(BooleanInducer * boolInducer, double * feats, unsigned long baseIndex)
{
	unsigned long i;
	unsigned char newVal;

	/* Generate features based on existing rules */
	for (i=0;i<boolInducer->generatedFeatsCount;i++)
	{
		switch (boolInducer->booleanRules[i].op)
		{
			case BI_OP_EQ:
				newVal = (feats[boolInducer->booleanRules[i].feat1] == boolInducer->booleanRules[i].referenceValue);
				break;
			case BI_OP_GE:
				newVal = (feats[boolInducer->booleanRules[i].feat1] >= boolInducer->booleanRules[i].referenceValue);
				break;
			case BI_OP_LE:
				newVal = (feats[boolInducer->booleanRules[i].feat1] <= boolInducer->booleanRules[i].referenceValue);
				break;
			case BI_OP_GT:
				newVal = (feats[boolInducer->booleanRules[i].feat1] > boolInducer->booleanRules[i].referenceValue);
				break;
			case BI_OP_LT:
				newVal = (feats[boolInducer->booleanRules[i].feat1] < boolInducer->booleanRules[i].referenceValue);
				break;
			case BI_OP_NE:
				newVal = (feats[boolInducer->booleanRules[i].feat1] != boolInducer->booleanRules[i].referenceValue);
				break;
			default:
				/* Doesn't happen, it's just to avoid a warning from some compilers */
				newVal = 0;
				break;
		}

		/* If there's a second feat, test it as well */
		if (boolInducer->booleanRules[i].feat2 != -1)
			newVal = (newVal && feats[boolInducer->booleanRules[i].feat2 + baseIndex]);

		/* Save newVal at the specific position */
		feats[baseIndex + i] = newVal;
	}

	/* Return OK */
	return ML_OK;
}

static BooleanInducer * BooleanInducer_Clone (BooleanInducer * boolInducer)
{
	BooleanInducer * aux;

	/* malloc memory to store the cloned instance */
	aux = (BooleanInducer *) malloc (sizeof(BooleanInducer));
	if (aux == NULL)
		return NULL;

	/* Copy all data from boolInducer to aux */
	memcpy (aux, boolInducer, sizeof(BooleanInducer));

	/* If boolInducer have rules, malloc memory for them */
	if (boolInducer->booleanRules != NULL)
	{
		aux->booleanRules = (BooleanRule *) malloc (sizeof(BooleanRule) * boolInducer->generatedFeatsCount);
		if (aux->booleanRules == NULL)
		{
			free (aux);
			return NULL;
		}

		/* Copy all rules data to aux */
		memcpy (aux->booleanRules, boolInducer->booleanRules, sizeof(BooleanRule) * boolInducer->generatedFeatsCount);
	}

	/* Return aux (it's a duplicate copy, with it's own area of rules, so boolInducer can be freed without consequences to the new instance */
	return aux;
}

static int BooleanInducer_WriteData (BooleanInducer * boolInducer, FILE * out)
{
	/* Write an identifier to know what kind of inducer is it when reading */
	if (fwrite (BOOLEANINDUCER_ID, sizeof(char), INDUCER_ID_LEN, out) != INDUCER_ID_LEN)
	{
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* Write BooleanInducer data */
	if (fwrite (boolInducer, sizeof(BooleanInducer), 1, out) != 1)
	{
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* Write Rules data */
	if (fwrite (boolInducer->booleanRules, sizeof(BooleanRule), boolInducer->generatedFeatsCount, out) != boolInducer->generatedFeatsCount)
	{
		errno = EIO;
		return ML_ERR_FILE;
	}

	/* Return OK */
	return ML_OK;

}

/************************
* "Protected" Functions	*
************************/
void BooleanInducer_Init (BooleanInducer * boolInducer)
{
	/* If the local "super" isn't initialized, init it */
	if (!superInitialized)
	{
		FeatInducer_Init(&super);
		superInitialized = 1;
	}

	/* Call the initializer for the "superclass" */
	FeatInducer_Init((FeatInducer *) boolInducer);

	/* Initialize the function pointers. */
	boolInducer->generate = (int (*)(FeatInducer *, double *, unsigned long)) BooleanInducer_Generate;
	boolInducer->clone = (FeatInducer * (*)(FeatInducer *)) BooleanInducer_Clone;
	boolInducer->writeData = (int (*)(FeatInducer *, FILE *)) BooleanInducer_WriteData;

	/* Overrides the default free method */
	boolInducer->free = (void(*)(FeatInducer *))BooleanInducer_Free;
}

/************************
* "Public" Functions	*
************************/
BooleanInducer * BooleanInducer_New (char * path, unsigned long maxLvl)
{
	BooleanInducer * boolInducer;

	/* malloc memory to store the structure */
	boolInducer = (BooleanInducer *) malloc (sizeof(BooleanInducer));
	if (boolInducer == NULL)
		return NULL;

	/* Zero memory */
	memset (boolInducer, 0, sizeof(BooleanInducer)); 

	/* Initialize the structure data and pointers */
	BooleanInducer_Init(boolInducer);

	/* Load data from path */
	if (BooleanInducer_LoadFeats (boolInducer, path, maxLvl) != ML_OK)
	{
		BooleanInducer_Free(boolInducer);
		return NULL;
	}

	/* Return the new instance */
	return boolInducer;
}

BooleanInducer * BooleanInducer_ReadData (FILE * in)
{
	BooleanInducer * boolInducer;
	char idOnFile[INDUCER_ID_LEN];
	
	/* Read the ID on the file to check if it matches the expected id */
	if (fread(idOnFile, sizeof(char), INDUCER_ID_LEN, in) != INDUCER_ID_LEN)
		return NULL;
	
	/* Check if it's correct */
	if (memcmp(idOnFile, BOOLEANINDUCER_ID, INDUCER_ID_LEN) != 0)
		return NULL;

	/* malloc memory to store the structure */
	boolInducer = (BooleanInducer *) malloc (sizeof(BooleanInducer));
	if (boolInducer == NULL)
		return NULL;

	/* Read BooleanInducer data */
	if (fread (boolInducer, sizeof(BooleanInducer), 1, in) != 1)
	{
		free (boolInducer);
		return NULL;
	}

	/* Call init to update function pointers */
	BooleanInducer_Init(boolInducer);

	/* malloc memory to store the rules */
	boolInducer->booleanRules = (BooleanRule *) malloc (sizeof(BooleanRule) * boolInducer->generatedFeatsCount);
	if (boolInducer->booleanRules == NULL)
	{
		BooleanInducer_Free (boolInducer);
		return NULL;
	}

	/* Read Rules data */
	if (fread (boolInducer->booleanRules, sizeof(BooleanRule), boolInducer->generatedFeatsCount, in) != boolInducer->generatedFeatsCount)
	{
		BooleanInducer_Free (boolInducer);
		return NULL;
	}

	/* Return the new instance */
	return boolInducer;


}