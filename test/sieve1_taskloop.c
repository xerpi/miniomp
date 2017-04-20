#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef _OPENMP
#include <omp.h>
#endif

double getusec_()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	return ((double)time.tv_sec * (double)1e6 + (double)time.tv_usec);
}

#define START_COUNT_TIME stamp = getusec_();
#define STOP_COUNT_TIME stamp = getusec_() - stamp;\
                        stamp = stamp/1e6;

// simple serial sieve of Eratosthenes
int eratosthenes(int lastNumber)
{
	int found = 0;
	int sqrt_lN = sqrt(lastNumber);
	// 1. create a list of natural numbers 2, 3, 4, ... all of them initially marked as potential primes
	char *isPrime = (char *)malloc((lastNumber + 1) * sizeof(char));

	#pragma omp parallel
	#pragma omp single
	#pragma omp taskloop
	for (int i = 0; i <= lastNumber; i++)
		isPrime[i] = 1;

	// 2. Starting from i=2, the first unmarked number on the list ...
	for (int i = 2; i <= sqrt_lN; i++) {
		//for (int i = 2; i*i <= lastNumber; i++)
		// ... find the smallest number greater or equal than i that is unmarked
		if (isPrime[i]) {
			// 3. Mark all multiples of i between i^2 and lastNumber
			#pragma omp parallel
			#pragma omp single
			#pragma omp taskloop
			for (int j = i * i; j <= lastNumber; j += i)
				isPrime[j] = 0;
		
		}
	}
#ifdef _OPENMP
	// 4. The unmarked numbers are primes, count primes
	struct {
		int num;
		int padding[(64 - sizeof(int)) / 4]; /* Avoid false sharing */
	} *local_found = calloc(omp_get_max_threads(), sizeof(*local_found));	
	#pragma omp parallel
	#pragma omp single
	#pragma omp taskloop
	for (int i = 2; i <= lastNumber; i++)
		local_found[omp_get_thread_num()].num += isPrime[i];
		
	for (int i = 0; i < omp_get_max_threads(); i++)
		found += local_found[i].num;
	
	free(local_found);
#else
	for (int i = 2; i <= lastNumber; i++)
		found += isPrime[i];
#endif

	// 5. We are done with the isPrime array, free it
	free(isPrime);
	return found;
}

void usage(void)
{
#ifdef _OPENMP
	printf("sieve <range> <thread count>\n");
	printf("      <range> is an integer N - the range is from 2 - N\n");
	printf("      <thread count> is the number of threads to use\n");
#else
	printf("sieve <range>\n");
	printf("      <range> is an integer N - the range is from 2 - N\n");
#endif
}

int main(int argc, char **argv)
{
	// argv[1]: Upper-bound on primes
	// argv[2]: Number of threads to run in parallel

#ifdef _OPENMP
	if (argc != 3) {
#else
	if (argc != 2) {
#endif
		printf("Error: Invalid number of arguments\n");
		usage();
		return 0;
	}

	int range_max = atoi(argv[1]);
	printf("Range = %d\n", range_max);
#ifdef _OPENMP
	int num_threads = atoi(argv[2]);
#endif

	if (range_max < 2) {
		printf
		    ("Error: <range> Must be an integer greater than or equal to 2\n");
		usage();
		return 0;
	}
#ifdef _OPENMP
	if (num_threads < 1) {
		printf
		    ("Error: <thread count> Must be a positive value between 1 an %d\n",
		     omp_get_max_threads());
		usage();
		return 0;
	} else if (num_threads > omp_get_max_threads()) {
		num_threads = omp_get_max_threads();
	}
	omp_set_num_threads(num_threads);

	// Make sure we haven't created too many threads.
	int temp = (range_max - 1) / num_threads;
	if ((1 + temp) < (int)sqrt((double)range_max)) {
		printf("Error: Too many threads requested!\n");
		printf
		    ("       The first thread must have a block size >= sqrt(n)\n");
		exit(1);
	}
#endif

	double stamp;
	// Solutions count
	START_COUNT_TIME;
	int count = eratosthenes(range_max);
	STOP_COUNT_TIME;

	// Print the results.
	printf("Number of primes smaller than or equal to %d = %d\n", range_max,
	       count);
	printf("%0.6f\n", stamp);

	return 0;
}
