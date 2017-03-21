#include "libminiomp.h"

// This file implements the PARALLEL construct

// Private structs definitions

// Declaration of array for storing pthread identifier from pthread_create function
pthread_t *miniomp_threads;

// Global variable for parallel descriptor
miniomp_parallel_t miniomp_parallel;
miniomp_parallel_barrier_t *miniomp_parallel_barrier;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

// This is the prototype for the Pthreads starting function
static void *parallel_worker(void *args)
{
	miniomp_specific_t specific;
	miniomp_parallel_t *parallel = args;

	specific.id = parallel->id;
	pthread_setspecific(miniomp_specifickey, &specific);

	parallel->fn(parallel->fn_data);

	/*
	 * Join the implicit barrier of the parallel clause
	 */

	// insert all necessary code here for:
	//   1) save thread-specific data
	//   2) invoke the per-threads instance of function encapsulating the parallel region
	//   3) exit the function
	return NULL;
}

// This is the prototype for the parallel's implicit barrier
static void *parallel_barrier_worker(void *args)
{
	miniomp_specific_t specific;
	miniomp_parallel_barrier_t *barrier = args;

	specific.id = barrier->id;
	pthread_setspecific(miniomp_specifickey, &specific);

	// insert all necessary code here for:
	//   1) save thread-specific data
	//   2) invoke the per-threads instance of function encapsulating the parallel region
	//   3) exit the function
	return NULL;
}

void
GOMP_parallel(void (*fn) (void *), void *data, unsigned num_threads,
	      unsigned int flags)
{
	unsigned i;
	int ret;

	if (!num_threads)
		num_threads = omp_get_num_threads();

	printf("Starting a parallel region using %d threads\n", num_threads);

	miniomp_threads = calloc(num_threads, sizeof(*miniomp_threads));
	if (!miniomp_threads) {
		printf("Error calloc()\n");
		return;
	}

	miniomp_parallel_barrier = calloc(num_threads - 1,
				          sizeof(*miniomp_parallel_barrier));
	if (!miniomp_parallel_barrier) {
		printf("Error calloc()\n");
		goto error_parallel_barrier_alloc;
	}

	ret = pthread_key_create(&miniomp_specifickey, NULL);
	if (ret != 0) {
		printf("Error pthread_key_create(): %d\n", ret);
		goto error_key_create;
	}

	/*
	 * Parallel-single thread creation
	 */
	miniomp_parallel.fn = fn;
	miniomp_parallel.fn_data = data;
	miniomp_parallel.id = 0;
	ret = pthread_create(&miniomp_threads[0], NULL,
			     parallel_worker, &miniomp_parallel);

	/*
	 * Barrier thread creation
	 */
	for (i = 1; i < num_threads; i++) {
		miniomp_parallel_barrier[i - 1].id = i;

		ret = pthread_create(&miniomp_threads[i], NULL,
				     parallel_barrier_worker,
				     &miniomp_parallel_barrier[i - 1]);
	}


	/*
	for (int i = 0; i < num_threads; i++)
		fn(data);
	*/

	/*
	 * Wait for threads to finish
	 */
	for (i = 0; i < num_threads; i++) {
		ret = pthread_join(miniomp_threads[i], NULL);
	}


	 pthread_key_delete(miniomp_specifickey);

error_key_create:
	free(miniomp_parallel_barrier);
error_parallel_barrier_alloc:
	free(miniomp_threads);
}
