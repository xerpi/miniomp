#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>

int main(int argc, char *argv[])
{
	uint32_t count = 0;

	#pragma omp parallel
	#pragma omp single // Only needed for GOMP
	{
		for (int i = 0; i < 5000; i++) {
			#pragma omp task
			{
				for (int j = 0; j < 500; j++) {
					#pragma omp task
					{
						#pragma omp atomic
						count++;
					}
				}
			}
		}
	}

	printf("Count: %d\n", count);
}
