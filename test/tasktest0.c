#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>

int main(int argc, char *argv[])
{
	#pragma omp parallel
	{
		for (int i = 0; i < 5; i++) {
			#pragma omp task
			printf("Task: %d\n", i);
		}
	}

	printf("Done!\n");
}
