#include <stdio.h>
#include <stdlib.h>
#include <omp.h>	/* OpenMP */

long result=0;

void foo() {
#pragma omp parallel 
#pragma omp single
    {
    int argum = 1;
    #pragma omp taskgroup
    {
    #pragma omp task  shared(result) firstprivate(argum)
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result += argum;
        }

    argum++;
    #pragma omp task  shared(result) firstprivate(argum)
    for (long i = 0; i < 10; i++) {
	#pragma omp atomic
        result += argum;
        }
    #pragma omp taskwait

    argum = result;
    for (long i = 0; i < 10; i++) {
        #pragma omp task shared(result) firstprivate(argum)
	#pragma omp atomic
        result += argum;
        }
    }

    #pragma omp task firstprivate(result) firstprivate(argum)
    printf("Hello from third task, up to now result=%ld and argum = %d\n", result, argum);
    }
}

int main(int argc, char *argv[]) {
    foo();
    printf("Back in main ... result = %ld\n", result);
}
