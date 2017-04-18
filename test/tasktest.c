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
	{for (int i = 0; i < 5; i++) {
			#pragma omp task
			{
				printf("Outer task: %d\n", i);
				for (int j = 0; j < 5; j++) {
					#pragma omp task
					{
						printf("  Inner task: %d - %d\n", i, j);
					}
				}
			}
		}
	}

	printf("Done!\n");
}
