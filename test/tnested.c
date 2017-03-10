#include <stdio.h>
#include <stdlib.h>
#include <omp.h>	/* OpenMP */


int foo() {
    int total=0;
    #pragma omp parallel reduction(+:total) num_threads(2)
    {
    printf("I amb thread %d in level %d\n", omp_get_thread_num(), omp_get_level());
    if (omp_get_thread_num() == 0) omp_set_num_threads(4);
    else omp_set_num_threads(6);

    #pragma omp parallel 
    {
    printf("I amb thread %d in level %d, son of %d, after executing first region\n", omp_get_thread_num(), omp_get_level(), omp_get_ancestor_thread_num(omp_get_level()-1));

    #pragma omp critical
    total++;
    }

    #pragma omp parallel shared(total) num_threads(8)
    {
    printf("I amb thread %d in level %d, son of %d, after executing second region\n", omp_get_thread_num(), omp_get_level(), omp_get_ancestor_thread_num(omp_get_level()-1));

    #pragma omp for reduction(+: total)
    for (int i = 0; i < 16; i++)
        total++;

    }
    }

    return(total);
}

int main(int argc, char *argv[]) {
    printf("Nested parallelism enabled? %d\n", omp_get_nested());
    omp_set_nested(1);
    printf("Value for total is %d\n", foo());
}
