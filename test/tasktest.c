#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <omp.h>

int main(int argc, char *argv[])
{
	#pragma omp parallel
	{
		#pragma omp task
		printf("Hello from thread: %d\n",
			omp_get_thread_num());
	}
}
