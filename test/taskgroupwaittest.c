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
		#pragma omp taskgroup
		{
			for (int i = 0; i < 50000; i++) {
				#pragma omp task
				{
					printf("Task: %d\n", i);
				}
			}

			#pragma omp taskwait

			printf("After taskwait!\n");

			for (int i = 0; i < 5; i++) {
				#pragma omp task
				{
					printf("NEWWWWWWWWW Task: %d\n", i);
				}
			}

		}
		printf("After group!\n");

	}

	printf("main() done!\n");
}
