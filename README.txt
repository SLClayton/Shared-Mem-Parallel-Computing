
CM30225 Parallel Computing Coursework 1 - University of Bath

This is the solution of differential equations using a method called the relaxation technique. This is done by having an array of values and repeatedly replacing a value with the average of its four neighbours; excepting boundary values, which remain at fixed values. This is repeated until all values settle down to within a given precision. This is a shared memory parallel solution written in C using p_threads.

The following command should compile the code correctly on Linux


gcc main.c -o program -std=gnu99 -lpthread -lrt -Wall


To run the program after compilation, you need 4 arguments in this order.

1. Scale of matrix (Integer).
2. Number of threads (Integer).
3. Precision to work to (Double).
4. Type (Char)
    - 's' for Single
    - 'c' for Correctness test
    - 't' for Test


Eg

./program 100 5 0.1 s


'Single' will do a simple one time relaxation with the given arguments. And tell you how long it takes in seconds via printing the result.

'Correctness' test will do matrix relaxation first with the non parallel function (to determine a ‘correct’ answer, more on this later) and then with all thread numbers up to a certain size. So if your argument for threads is N, it will run and time the script running on 1 thread, then 2, then 3, up until N threads is tested. After each it will verify if the if the matrix is correct according to the tested non parallel function answer. Given the same final solution, and the fact that it has taken the same amount of steps. It is a safe assumption the answer is correct.

'Test' will create 3 different matrices of size 'Scale', run each one, on every number of threads to see an average speedup each number of threads creates. It will then print the results.

