#define _BSD_SOURCE
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
	unsigned int count = 0;

	#pragma omp parallel
	{
		for (int i = 0; i < 10; i++) {
			#pragma omp task shared(count)
			for (int j = 0; j < 10; j++) {
				#pragma omp task shared(count)
				{
					//printf("j = %d\n", j);
					#pragma omp atomic
					count++;
				}
			}
		}
	}

	printf("Count: %d\n", count);
}
