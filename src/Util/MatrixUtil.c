#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "MacLearn/Util/MatrixUtil.h"

void printULMatrix (char * name, unsigned long * mat, int size1, int size2)
{
	int i, j;

	printf ("%s =\n",name);
	for (i=0;i<size1;i++)
	{
		for (j=0;j<size2;j++)
			printf ("%8lu\t",mat[i * size1 + j]); 
		printf ("\n");
	}
	printf ("\n");
}

void printGSLMatrix (char * name, gsl_matrix * mat)
{
	size_t i, j;

	printf ("%s =\n",name);
	for (i=0;i<mat->size1;i++)
	{
		for (j=0;j<mat->size2;j++)
			printf ("%- 8lf\t",mat->data[i * mat->tda + j]); 
		printf ("\n");
	}
	printf ("\n");
}

void printGSLVect (char * name, gsl_vector * vect)
{
	size_t i;

	printf ("%s =\n",name);
	for (i=0;i<vect->size;i++)
		printf ("%- 8lf\t",vect->data[i]); 
	printf ("\n");
	printf ("\n");
}

int copyMatrix (gsl_matrix * dst, gsl_matrix * src, size_t size1, size_t size2)
{
	size_t i;

	if (dst->size1 < size1 || dst->size2 < size2 ||
		src->size1 < size1 || src->size2 < size2)
		return 0;

	/* Copia cada linha */
	for (i=0;i<size1;i++)
		memcpy (&dst->data[dst->tda * i], &src->data[src->tda * i], sizeof(double) * size2);

	/* atualiza o tamanho de dst */
	dst->size1 = size1;
	dst->size2 = size2;

	return 1;
}

void printMatrix (char * name, double * mat, size_t rows, size_t cols)
{
	gsl_matrix_view matView;

	matView = gsl_matrix_view_array(mat,rows,cols);
	printGSLMatrix(name, &matView.matrix);
}

void printVector (char * name, double * vect, size_t size)
{
	gsl_vector_view vectView;

	vectView = gsl_vector_view_array(vect,size);
	printGSLVect(name, &vectView.vector);
}

int shuffleVector (void * v, size_t elementSize, size_t elementCount)
{
	size_t i;
	size_t src;
	void * aux;
	unsigned char * ucBase;

	/* Recast base to unsigned char so we can do pointer math in windows */
	ucBase = (unsigned char *) v;

	aux = malloc (elementSize);
	if (aux == NULL)
		return 0;

	/* Inicializa o gerador de números aleatórios */
	srand((unsigned int) time(NULL));
	for (i=elementCount - 1;i>0;i--)
	{
		/* Pega um número aleatório */
		src = rand()%i;

		/* Troca a posição i com a posição aleatória */
		memcpy (aux, ucBase + (src * elementSize), elementSize);
		memcpy ( ucBase + (src * elementSize), ucBase + (i * elementSize), elementSize);
		memcpy (ucBase + (i * elementSize), aux, elementSize);
	}

	/* Libera a memória */
	free (aux);

	/* Return OK */
	return 1;
}