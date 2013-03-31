#ifndef __MATRIXUTIL_H__
#define __MATRIXUTIL_H__

#include <gsl/gsl_matrix.h>

void printMatrix (char * name, double * mat, size_t rows, size_t cols);
void printVector (char * name, double * vect, size_t size);
int copyMatrix (gsl_matrix * dst, gsl_matrix * src, size_t size1, size_t size2);
void printGSLVect (char * name, gsl_vector * vect);
void printGSLMatrix (char * name, gsl_matrix * mat);
int shuffleVector (void * v, size_t elementSize, size_t elementCount);
void printULMatrix (char * name, unsigned long * mat, int size1, int size2);


#endif