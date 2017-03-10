#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <omp.h>	/* OpenMP */

long result=0;

void foo() {
#pragma omp parallel 
    {
    #pragma omp for 
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result++;
    }

    #pragma omp for schedule(dynamic,1) reduction(+: result)
    for (long i = 0; i < 10; i++)
        result++;

    #pragma omp for schedule(dynamic,1) 
    for (long i = 0; i < 10; i++)
	#pragma omp atomic
        result++;

    #pragma omp for schedule(dynamic,2) nowait
    for (long i = 0; i < 10; i++)
	#pragma omp critical
        result++;

    #pragma omp atomic
    result++;

    #pragma omp barrier
    #pragma omp single
    printf("Inside foo ... result = %ld\n", result);
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... result = %ld\n", result);
}
