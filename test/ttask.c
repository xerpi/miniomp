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
//#pragma omp single
    {
    int argum = 1;
    #pragma omp task  shared(result) firstprivate(argum)
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result += argum;
        }

    for (long i = 0; i < 10; i++) {
        argum++;
        #pragma omp task shared(result) firstprivate(argum)
	#pragma omp atomic
        result += argum;
        }

    #pragma omp task firstprivate(result) firstprivate(argum)
    printf("Hello from third task, up to now result=%ld and argum = %d\n", result, argum);
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... result = %ld\n", result);
}
