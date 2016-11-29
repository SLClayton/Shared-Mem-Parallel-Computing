#include <stdio.h>
#include <stdlib.h>
#include <math.h>



void swapMatrix(double*** a, double*** b){
	//---------------------------------------------------------------
    // Swaps the address two pointers that point to matrices
    //---------------------------------------------------------------
	double** temp = *a;
	*a = *b;
	*b = temp;
}


double **createMatrix(int scale){
	//---------------------------------------------------------------
    // Creates 2D array of doubles
    //---------------------------------------------------------------
	double **matrix = malloc(scale*sizeof(double*));
	if (matrix == NULL){
		printf("matrix is null so exiting");
		exit(0);
	}

	double *buf = malloc(scale*scale*sizeof(double));
	if (buf == NULL){
		printf("Matrix buffer is null so exiting");
		exit(0);
	}

	for (int i=0; i<scale; i++){
		matrix[i] = buf + (scale*i);
	}

	return matrix;
}


void printMatrix(double*** matrix, int scale){
	//---------------------------------------------------------------
    // Given a 2d Array, prints its values in a table format.
    //---------------------------------------------------------------
	for (int i=0; i<scale; i++){
		for (int j=0; j<scale; j++){
			printf("%.2f ", (*matrix)[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}


int copyMatrix(double*** copyFrom, double*** copyTo, int scale){
	//---------------------------------------------------------------
    // Copies the contents of one matrix to another
    //---------------------------------------------------------------
	for (int i=0; i<scale; i++){
		for (int j=0; j<scale; j++){
			(*copyTo)[i][j] = (*copyFrom)[i][j];
		}
	}
	return 0;
}


double** cloneMatrix(double*** clonedFrom, int scale){
	//---------------------------------------------------------------
    // Creates a new matrix with the same values as a given matrix
    //---------------------------------------------------------------
	double** clonedTo = createMatrix(scale);

	copyMatrix(clonedFrom, &clonedTo, scale);

	return clonedTo;
}


void relaxMatrixRows(double ***read, double*** write, int scale, 
	                 int rowFrom,    int rowTo){

	//---------------------------------------------------------------
    // Relaxes a matrix by averages sets of 4 numbers, but only
    // of the rows specified of one matrix to another.
    //---------------------------------------------------------------

	for (int i=rowFrom; i<=rowTo; i++){
		for (int j=1; j<(scale-1); j++){

			double above = (*read)[i-1][j];
			double below = (*read)[i+1][j];
			double left = (*read)[i][j-1];
			double right = (*read)[i][j+1];

			(*write)[i][j] = (above+below+left+right) / 4.0;
		}
	}
}


void relaxMatrix(double*** read, double*** write, int scale){
	//---------------------------------------------------------------
    // Relaxes entire matrix by specifying all relaxable rows
    // in function.
    //---------------------------------------------------------------
	relaxMatrixRows(read, write, scale, 1, scale-2);
}


void fillMatrix(double ***m, int scale, double min, double max){
	//---------------------------------------------------------------
    // Fills a matrix with random doubles
    //---------------------------------------------------------------

	srand(time(NULL));

	for (int i=0; i<scale; i++){
		for (int j=0; j<scale; j++){
			(*m)[i][j] = ((double) rand() * (max - min)) /
						 ((double) RAND_MAX + min);
		}
	}
}


int sameNumberToPrecision(double a, double b, double precision){
	//---------------------------------------------------------------
    // Checks that two doubles are within a certain precision of
    // each other (plus a double margin of error)
    //---------------------------------------------------------------

	 if (fabs(a - b) <= precision + 0.000000000001){
	 	return 1;
	 }
	return 0;
}


int sameMatrixRowsToPrecision(double*** a,      double*** b, int scale,
	                          double precision, int rowFrom, int rowTo){
	
	//---------------------------------------------------------------
    // Checks whether two matrices contain the same values in the 
    // given rows to a given precision
    //---------------------------------------------------------------

	for (int i=rowFrom; i<=rowTo; i++){
		for (int j=1; j<(scale-1); j++){

			if (!sameNumberToPrecision((*a)[i][j], (*b)[i][j], precision)){
				return 0;
			}
		}
	}
	return 1;
}

int sameMatrixToPrecision(double*** a, double*** b, int scale, double precision){
	//---------------------------------------------------------------
    // Checks whether two matrices are the same to a given precision
    //---------------------------------------------------------------
	return sameMatrixRowsToPrecision(a, b, scale, precision, 1, scale-2);
}

int sameMatrix(double ***a, double ***b, int scale){
	//---------------------------------------------------------------
    // Checks whether two matrices are the same
    //---------------------------------------------------------------
	return sameMatrixToPrecision(a, b, scale, 0.0);
}