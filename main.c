#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

#include "matrixWizard.c"

#define true 1;
#define false 0;

pthread_barrier_t barrier;


struct assignedRows {
    //---------------------------------------------------------------
    // Stores data for row assignment
    //---------------------------------------------------------------
    int *assignedStartRow;
    int *assignedNumberOfRows;
};

struct thread_args{
    //---------------------------------------------------------------
    // Encapsulates the arguments passed to a thread
    //---------------------------------------------------------------
    double ***matrix;
    double ***lastMatrix;
    int **rowsComplete;

    int threadNumber;
    int totalThreads;
    int scale;
    int rowFrom;
    int rowTo;
    double precision;
};



void getArgs(int* scale,        int *threads, 
             double* precision, char *type, 
             int argc,          char* argv[]){

    printf("\n");
    
    //---------------------------------------------------------------
    // Check the correct amount of arguments are given
    //---------------------------------------------------------------
    if (argc < 5){
        printf("Not enough arguments, please provide arguments for");
        printf(" 'Scale', 'Threads', 'Precision', and 'Type'.\n");
        exit(0);
    }
    if (argc > 5){
        printf("Too many arguments, please only provide arguments for");
        printf(" 'Scale', 'Threads', 'Precision', and 'Type'.\n");
        exit(0);
    }


    //---------------------------------------------------------------
    // Collect arguments and put them into the addresses given
    //---------------------------------------------------------------
    *scale = atoi(argv[1]);
    *threads = atoi(argv[2]);
    sscanf(argv[3], "%lf", precision);
    *type = argv[4][0];

    if (*threads < 1){
        printf("You cannot run on less than 1 thread.\n");
        printf("Exiting\n");
        exit(0);
    }


    //---------------------------------------------------------------
    // Print arguments
    //---------------------------------------------------------------

    printf("Arguments set as...\n");
    printf("Scale     = %d\n", *scale);
    printf("Threads   = %d\n", *threads);
    printf("Precision = %f\n", *precision);

    if (*type == 't'){
        printf("Type      = Test\n");
    }
    else if (*type == 'c'){
        printf("Type      = Correctness\n");
    }

    else{
        printf("Type      = Single\n");
    }

    printf("\n");
}


struct assignedRows getAssignedRows(int scale, int threads){

    struct assignedRows AR;

    //---------------------------------------------------------------
    // Multi threading works in this program by dividing up the
    // rows of the matrix, so there must be enough rows for each
    // thread.
    //---------------------------------------------------------------
    int workableRows = scale - 2;
    if (threads > workableRows){
        printf("Scale must be at least x+2 in size to work with x threads.\n");
        exit(0);
    }


    AR.assignedNumberOfRows = malloc(threads*sizeof(int));
    AR.assignedStartRow = malloc(threads*sizeof(int));
    for (int i=0; i<threads; i++){
        AR.assignedNumberOfRows[i] = 0;
    }


    //---------------------------------------------------------------
    // Each thread is incrementally given rows until all number
    // of rows are assigned.
    //---------------------------------------------------------------
    int threadToIncrement = 0;
    for (int i=0; i<workableRows; i++){

        AR.assignedNumberOfRows[threadToIncrement]++;
        
        threadToIncrement++;
        if (threadToIncrement >= threads){
            threadToIncrement = 0;
        }
    }


    //---------------------------------------------------------------
    // The row each thread is to start from is assigned.
    // Each row will then have a start row, and a number of rows
    // to work in. So the range can then be worked out.
    //---------------------------------------------------------------
    int startRow = 1;
    for (int i=0; i<threads; i++){
        AR.assignedStartRow[i] = startRow;
        startRow = startRow + AR.assignedNumberOfRows[i];
    }

    return AR;
}


int allTrue(int **array, int size){
    //---------------------------------------------------------------
    // Checks all values in an array are 1;
    //---------------------------------------------------------------
    for (int i=0; i<size; i++){
        if ((*array)[i] == 0){
            return 0;
        }
    }
    return 1;
}


void relax_sync(double*** matrix, int scale, double precision, int verbose){

    printf("Starting relaxation of %d x %d ", scale, scale);
    printf("matrix with check function to precision %f\n", precision);

    //---------------------------------------------------------------
    // Create second matrix to hold the previous value for comparison
    //---------------------------------------------------------------
    double **lastMatrix = cloneMatrix(matrix, scale);


    //---------------------------------------------------------------
    // Iterativley relax matrix until the difference is less than
    // the precision.
    //---------------------------------------------------------------
    int count = 0;
    do {
        if (verbose){
            printf("Matrix after %d step/s\n", count);
            printMatrix(matrix, scale);
        }

        swapMatrix(matrix, &lastMatrix);

        relaxMatrix(&lastMatrix, matrix, scale);
        
        count++;

    } while (!sameMatrixToPrecision(matrix, &lastMatrix, scale, precision));



    printf("Finished in %d step/s\n", count);
    if (verbose){
        printf("Final Matrix\n");
        printMatrix(matrix, scale);
        printf("\n");
    }
    

    free(lastMatrix);
}


void relax_rows(double ***matrix, double ***lastMatrix, int **rowsComplete,
                int scale,        int rowFrom,          int rowTo, 
                double precision, int threadNumber,     int totalThreads){


    int count = 0;
    while (1){

        if (threadNumber == 0){
            //printf("Matrix after %d step/s\n", count);
            //printMatrix(matrix, scale);

            swapMatrix(matrix, lastMatrix);
        }

  
        pthread_barrier_wait(&barrier);

        //printf("Thread %d relaxing its rows\n", threadNumber);
        relaxMatrixRows(lastMatrix, matrix, scale, rowFrom, rowTo);
        count++;


        if ( ((*rowsComplete)[threadNumber] == 0) &&
             (sameMatrixRowsToPrecision(lastMatrix, matrix, scale,
                                      precision, rowFrom, rowTo))) {

            (*rowsComplete)[threadNumber] = 1;
        }

        pthread_barrier_wait(&barrier);
        
        
        if (allTrue(rowsComplete, totalThreads)){
            //printf("Thread %d found all to be correct now\n", threadNumber);
            pthread_barrier_wait(&barrier);
            break;        
        }
    }

    pthread_barrier_wait(&barrier);

    if (threadNumber == 0){
        printf("Matrix finished after %d steps!\n", count);
        //printf("Final matrix\n");
        //printMatrix(matrix, scale);
    }
}


void *relax_rows_thread(void *payload){
    //---------------------------------------------------------------
    // Runs  'relax_rows' function in a seperate thread.
    //---------------------------------------------------------------
    struct thread_args *p = payload;  
    relax_rows((*p).matrix, 
               (*p).lastMatrix,
               (*p).rowsComplete,
               (*p).scale, 
               (*p).rowFrom, 
               (*p).rowTo, 
               (*p).precision, 
               (*p).threadNumber, 
               (*p).totalThreads);

    return payload;
}


void relax_async(double ***matrix, int scale, int threads, double precision){

    printf("Starting relaxation of %d x %d ", scale, scale);
    printf("matrix with %d threads to precision %f\n", threads, precision);

    //---------------------------------------------------------------
    // Collect data on which thread will work on which rows.
    //---------------------------------------------------------------
    struct assignedRows AR = getAssignedRows(scale, threads);

    //---------------------------------------------------------------
    // Create array of argument structs to hold arguments for each
    // thread.
    //---------------------------------------------------------------
    struct thread_args *p = malloc(threads*sizeof(struct thread_args));

    pthread_t *ptt = malloc((threads-1)*sizeof(pthread_t));
    pthread_barrier_init(&barrier, NULL, threads);



    int *rowsComplete = malloc(threads*sizeof(int));
    for (int i=0; i<threads; i++){
        rowsComplete[i] = 0;
    }

    //---------------------------------------------------------------
    // Create second matrix to hold previous value for comparison
    //---------------------------------------------------------------
    double **lastMatrix = cloneMatrix(matrix, scale);


    //---------------------------------------------------------------
    // Create each thread with the arguments telling it the addresses
    // of the matrices to work on, and the rows it is designated to
    // work with within that matrix.
    //---------------------------------------------------------------
    for (int i=0; i<threads; i++){

        //---------------------------------------------------------------
        // Fill struct with arguments specified for each thread
        //---------------------------------------------------------------
        p[i].matrix = matrix;
        p[i].lastMatrix = &lastMatrix;
        p[i].rowsComplete = &rowsComplete;
        p[i].scale = scale;
        p[i].rowFrom = AR.assignedStartRow[i];
        p[i].rowTo = AR.assignedStartRow[i] + AR.assignedNumberOfRows[i] - 1;
        p[i].precision = precision;
        p[i].threadNumber = i;
        p[i].totalThreads = threads;

        

        //---------------------------------------------------------------
        // A new thread is created and run for n threads specified, the 
        // last thread is run by *this* thread to avoid having n+1
        // threads running.
        //---------------------------------------------------------------
        if (i == (threads-1)){
            relax_rows(p[i].matrix,
                       p[i].lastMatrix,
                       p[i].rowsComplete,
                       p[i].scale,
                       p[i].rowFrom,
                       p[i].rowTo,
                       p[i].precision,
                       p[i].threadNumber,
                       p[i].totalThreads);
        }
        else{
            pthread_create(&ptt[i], NULL, &relax_rows_thread, &(p[i]));
        }

    }

    for (int i=0; i<(threads-1); i++){
        pthread_join(ptt[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    free(lastMatrix);
}


void test_correctness(int scale, double precision, int threads){
    struct timespec start, finish;

    //--------------------------------------------------------------------
    // Create original matrix to begin from
    //--------------------------------------------------------------------
    double **originalMatrix = createMatrix(scale);
    fillMatrix(&originalMatrix, scale, 0, 10);



    //--------------------------------------------------------------------
    // Use the 'check' function to generate the 'right' answer
    //--------------------------------------------------------------------
    double **correctMatrix = cloneMatrix(&originalMatrix, scale);;

    clock_gettime(CLOCK_MONOTONIC, &start);
    relax_sync(&correctMatrix, scale, precision, 0);
    clock_gettime(CLOCK_MONOTONIC, &finish);

    double time_answer_seconds = (finish.tv_sec - start.tv_sec);
    time_answer_seconds += ((finish.tv_nsec - start.tv_nsec) / 1000000000.0);
    
    

    //--------------------------------------------------------------------
    // Run the multi thread function for each number of threads up to 
    // given amount, and time how long each takes
    //--------------------------------------------------------------------
    double **workingMatrix = createMatrix(scale);
    double *time_seconds = malloc(threads * sizeof(double));
    



    for (int i=0; i<threads; i++){
        printf("-------------------------------------------------------\n");


            copyMatrix(&originalMatrix, &workingMatrix, scale);

            clock_gettime(CLOCK_MONOTONIC, &start);
            relax_async(&workingMatrix, scale, i+1, precision);
            clock_gettime(CLOCK_MONOTONIC, &finish);

            time_seconds[i] = (finish.tv_sec - start.tv_sec);
            time_seconds[i] += ((finish.tv_nsec - start.tv_nsec) / 1000000000.0);



        //-----------------------------------------------------------------
        // Check each answer computed is the same as the 'correct' answer
        //-----------------------------------------------------------------
        if (sameMatrix(&workingMatrix, &correctMatrix, scale)){
            printf("Matrix checked and is correct\n");
        }
        else{
            printf("ERROR! Matrix has been checked and is not correct!\n");

            if (scale <= 20){
                printf("This matrix\n");
                printMatrix(&workingMatrix, scale);
                printf("Correct Matrix\n");
                printMatrix(&correctMatrix, scale);
            }
            exit(0);
        }
        

    }



    //---------------------------------------------------------------
    // Print results
    //---------------------------------------------------------------
    printf("\nRESULTS-------------------------------\n");
    printf("Sync function: \t%.3f seconds\n", time_answer_seconds);
    printf("--------------------------------------\n");
    for (int i=0; i<threads; i++){
        printf("%d thread: \t%.3f seconds\n", i+1, time_seconds[i]);
    }
}


void test_scale(int scale, double precision, int threads, int iterations){
    struct timespec start, finish;


    double **time_seconds = malloc(threads * sizeof(double*));
    for (int i=0; i<threads; i++){
        time_seconds[i] = malloc(iterations*sizeof(double));
    }

    //--------------------------------------------------------------------
    // Create original matrix to begin from and working matrix
    //--------------------------------------------------------------------
    double **originalMatrix = createMatrix(scale);
    double **workingMatrix = createMatrix(scale);


    for (int j=0; j<iterations; j++){
        printf("-------------------------------------------------------\n");

        fillMatrix(&originalMatrix, scale, 0, 10);
        

        for (int i=0; i<threads; i++){


                copyMatrix(&originalMatrix, &workingMatrix, scale);

                clock_gettime(CLOCK_MONOTONIC, &start);
                relax_async(&workingMatrix, scale, i+1, precision);
                clock_gettime(CLOCK_MONOTONIC, &finish);

                time_seconds[i][j] = (finish.tv_sec - start.tv_sec);
                time_seconds[i][j] += ((finish.tv_nsec - start.tv_nsec) / 1000000000.0);
        }
    }

    //---------------------------------------------------------------
    // Print results
    //---------------------------------------------------------------
    printf("\nRESULTS - TIME (seconds) ------------------\n");
    printf("- Threads ----- average -------------------\n");
    printf("-------------------------------------------\n");
    for (int i=0; i<threads; i++){

        double av = 0;
        for (int j=0; j<iterations; j++){
            av = av + time_seconds[i][j];
        }
        av = av / iterations;


        printf("%d thread: \t%.3f ( ", i+1, av);
        for (int j=0; j<iterations; j++){
            printf("%.3f ", time_seconds[i][j]);
        }
        printf(")\n");
    }



    printf("\nRESULTS - Speedup (times faster than 1 thread)--\n");
    printf("- Threads ----- Average ------------------------\n");
    printf("------------------------------------------------\n");


    double **speedup = malloc(threads * sizeof(double*));
    for (int i=0; i<threads; i++){
        speedup[i] = malloc(iterations*sizeof(double));
    }

    for (int i=0; i<threads; i++){
        for (int j=0; j<iterations; j++){
            speedup[i][j] = time_seconds[0][j] / time_seconds[i][j];
        }
    }

    for (int i=0; i<threads; i++){
        double av = 0;
        for (int j=0; j<iterations; j++){
            av = av + speedup[i][j];
        }
        av = av / iterations;

        printf("%d threads: \t%.3f ( ", i+1, av);
        for (int j=0; j<iterations; j++){
            printf("%.3f ", speedup[i][j]);
        }
        printf(")\n");
    }



    printf("\n");
}

void single_test(int scale, double precision, int threads){
    double **matrix = createMatrix(scale);
    fillMatrix(&matrix, scale, 0, 10);
    


    struct timespec start, finish;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);
    relax_async(&matrix, scale, threads, precision);
    clock_gettime(CLOCK_MONOTONIC, &finish);

    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        
    printf("Time = %f\n\n", elapsed);

    free(matrix);
}


int main(int argc, char *argv[]) {

    int scale;
    int threads;
    double precision;
    char type;
    getArgs(&scale, &threads, &precision, &type, argc, argv);


    if (type == 't'){
        test_scale(scale, precision, threads, 3);
    }
    else if (type == 'c'){
        test_correctness(scale, precision, threads);
    }
    else {
        single_test(scale, precision, threads);
    }

    return 0;
}

