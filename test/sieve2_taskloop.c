#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

// process only odd numbers of a specified block
int eratosthenesBlock(const int from, const int to)
{
	// 1. create a list of natural numbers 2, 3, 4, ... all of them initially marked as potential primes
	const int memorySize = (to - from + 1) / 2;	// only odd numbers
	char *isPrime = (char *)malloc((memorySize) * sizeof(char));
	for (int i = 0; i < memorySize; i++)
		isPrime[i] = 1;

	// 2. Starting from i=3, the first unmarked number on the list ...
	for (int i = 3; i * i <= to; i += 2) {
		// ... find the smallest number greater or equal than i that is unmarked
		// skip multiples of three: 9, 15, 21, 27, ...
		if (i >= 3 * 3 && i % 3 == 0)
			continue;
		// skip multiples of five
		if (i >= 5 * 5 && i % 5 == 0)
			continue;
		// skip multiples of seven
		if (i >= 7 * 7 && i % 7 == 0)
			continue;
		// skip multiples of eleven
		if (i >= 11 * 11 && i % 11 == 0)
			continue;
		// skip multiples of thirteen
		if (i >= 13 * 13 && i % 13 == 0)
			continue;
		// skip numbers before current slice
		int minJ = ((from + i - 1) / i) * i;
		if (minJ < i * i)
			minJ = i * i;
		// start value must be odd
		if ((minJ & 1) == 0)
			minJ += i;

		// 3. Mark all multiples of i between i^2 and lastNumber
		for (int j = minJ; j <= to; j += 2 * i) {
			int index = j - from;
			isPrime[index / 2] = 0;
		}
	}

	// 4. The unmarked numbers are primes, count primes
	int found = 0;
	for (int i = 0; i < memorySize; i++)
		found += isPrime[i];
	// 2 is not odd => include on demand
	if (from <= 2)
		found++;

	// 5. We are done with the isPrime array, free it
	free(isPrime);
	return found;
}

// process slice-by-slice
int eratosthenes(int lastNumber, int sliceSize)
{
	int found = 0;
	// each slice covers ["from" ... "to"], incl. "from" and "to"
	#pragma omp parallel
	#pragma omp single
	#pragma omp taskloop
	for (int from = 2; from <= lastNumber; from += sliceSize) {
		int to = from + sliceSize;
		if (to > lastNumber)
			to = lastNumber;
		#pragma omp atomic
		found += eratosthenesBlock(from, to);
	}
	return found;
}

void usage(void)
{
#ifdef _OPENMP
	printf("sieve <range> <slice_size> <thread count>\n");
	printf("      <range> is an integer N - the range is from 2 - N\n");
	printf
	    ("      <slice_size> is to sieve the list from 2 - N in blocks\n");
	printf("      <thread count> is the number of threads to use\n");
#else
	printf("sieve <range> <slice_size>\n");
	printf("      <range> is an integer N - the range is from 2 - N\n");
	printf
	    ("      <slice_size> is to sieve the list from 2 - N in blocks\n");
#endif
}

int main(int argc, char **argv)
{
	// argv[1]: Upper-bound on primes
	// argv[2]: Number of threads to run in parallel if OpenMP enabled

#ifdef _OPENMP
	if (argc != 4) {
#else
	if (argc != 3) {
#endif
		printf("Error: Invalid number of arguments\n");
		usage();
		return 0;
	}

	int range_max = atoi(argv[1]);
	int slice_size = atoi(argv[2]);
#ifdef _OPENMP
	int num_threads = atoi(argv[3]);
#endif

	if (range_max < 2) {
		printf
		    ("Error: <range> Must be an integer greater than or equal to 2\n");
		usage();
		return 0;
	}

	if ((slice_size > range_max) || (slice_size < 2)) {
		printf
		    ("Error: <slice_size> Must be an integer greater than or equal to 2 but smaller or equal than range\n");
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
	START_COUNT_TIME;
	// Solutions count
	int count = eratosthenes(range_max, slice_size);
	STOP_COUNT_TIME;

	// Print the results.
	printf("Number of primes smaller than or equal to %d = %d\n", range_max,
	       count);
	printf("%0.6f\n", stamp);

	return 0;
}
