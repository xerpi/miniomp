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
		for (int i = 0; i < 100; i++) {
			#pragma omp task shared(count)
			{
				/*printf("Hello from thread: %d\n",
					omp_get_thread_num());*/
				#pragma omp atomic
				count++;
				//usleep(250 * 1000);
			}
		}
	}

	printf("Count: %d\n", count);
}
