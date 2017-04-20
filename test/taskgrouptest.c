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
			for (int i = 0; i < 50; i++) {
				#pragma omp task
				{
					printf("Group 1 task: %d\n", i);
				}
			}
		}

		printf("After taskgroup 1!\n");

		#pragma omp taskgroup
		{
			for (int i = 0; i < 50; i++) {
				#pragma omp task
				{
					printf("Group 2 task: %d\n", i);
				}
			}
		}

		printf("After taskgroup 2!\n");

	}

	printf("main() done!\n");
}
