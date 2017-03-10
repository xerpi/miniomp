#include <stdio.h>
#include <stdlib.h>
#include <omp.h>		/* OpenMP */

long result = 0;

void foo()
{
#pragma omp parallel		// reduction(+:result)
	{
		for (long i = 0; i < 10; i++)
#pragma omp critical
			result++;
#pragma omp barrier

#pragma omp atomic
		result++;

#pragma omp flush(result)

#pragma omp barrier
		printf("result = %ld\n", result);

#pragma omp barrier
		printf("To double check ... result = %ld\n", result);
	}
}

int main(int argc, char *argv[])
{
	foo();
}
