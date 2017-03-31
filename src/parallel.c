#include "libminiomp.h"

pthread_key_t miniomp_specifickey;
miniomp_parallel_global_data_t miniomp_global_data;

static void parallel_implicit_barrier(miniomp_parallel_shared_data_t *shared)
{
	do {
		miniomp_task_t *task;

		pthread_mutex_lock(&shared->mutex);

		__sync_fetch_and_add(&shared->count, 1);

		task = taskqueue_dequeue(miniomp_taskqueue);
		if (task_is_valid(task)) {
			pthread_mutex_unlock(&shared->mutex);
			task->fn(task->data);
			__sync_fetch_and_sub(&shared->count, 1);
			free(task);
		} else {
			if (__sync_fetch_and_sub(&shared->count, 1) == 1) {
				shared->done = 1;
				__sync_synchronize();
				pthread_cond_broadcast(&shared->cond);
				pthread_mutex_unlock(&shared->mutex);
			} else {
				__sync_synchronize();
				if (!shared->done) {
					pthread_cond_wait(&shared->cond,
							  &shared->mutex);
				}
				pthread_mutex_unlock(&shared->mutex);
			}
		}
		__sync_synchronize();
	} while (!shared->done);
}

// This is the prototype for the Pthreads starting function
static void *parallel_worker_function(void *args)
{
	miniomp_specific_t specific;
	miniomp_parallel_worker_t *worker = args;

	specific.id = worker->id;
	pthread_setspecific(miniomp_specifickey, &specific);

	worker->fn(worker->fn_data);

	__sync_fetch_and_sub(&worker->shared->count, 1);

	/*
	 * Join the implicit barrier of the parallel clause
	 */
	parallel_implicit_barrier(worker->shared);

	// insert all necessary code here for:
	//   1) save thread-specific data
	//   2) invoke the per-threads instance of function encapsulating the parallel region
	//   3) exit the function
	return NULL;
}

// This is the prototype for the parallel's implicit barrier
static void *parallel_barrier_function(void *args)
{
	miniomp_specific_t specific;
	miniomp_parallel_barrier_t *barrier = args;

	specific.id = barrier->id;
	pthread_setspecific(miniomp_specifickey, &specific);

	parallel_implicit_barrier(barrier->shared);

	// insert all necessary code here for:
	//   1) save thread-specific data
	//   2) invoke the per-threads instance of function encapsulating the parallel region
	//   3) exit the function
	return NULL;
}

void
GOMP_parallel(void (*fn)(void *), void *data, unsigned num_threads,
	      unsigned int flags)
{
	unsigned i;
	int ret;
	pthread_t *threads;
	miniomp_parallel_worker_t parallel_worker;
	miniomp_parallel_barrier_t *parallel_barriers;
	miniomp_parallel_shared_data_t *shared_data;

	if (!num_threads)
		num_threads = omp_get_num_threads();

	printf("Starting a parallel region using %d threads\n", num_threads);

	threads = calloc(num_threads, sizeof(*threads));
	if (!threads) {
		printf("Error allocating thread IDs\n");
		return;
	}

	parallel_barriers = calloc(num_threads - 1, sizeof(*parallel_barriers));
	if (!parallel_barriers) {
		printf("Error allocating parallel_barriers\n");
		goto error_parallel_barrier_alloc;
	}

	shared_data = malloc(sizeof(*shared_data));
	if (!shared_data) {
		printf("Error allocating shared_data\n");
		goto error_alloc_shared_data;
	}

	ret = pthread_key_create(&miniomp_specifickey, NULL);
	if (ret != 0) {
		printf("Error pthread_key_create(): %d\n", ret);
		goto error_key_create;
	}

	/*
	 * Initialize shared data
	 */
	pthread_mutex_init(&shared_data->mutex, NULL);
	pthread_cond_init(&shared_data->cond, NULL);
	shared_data->done = false;
	shared_data->count = 1;
	__sync_synchronize();

	/*
	 * Set parallel global libminiomp data
	 */
	miniomp_global_data.shared = shared_data;

	/*
	 * Parallel-single thread creation
	 */
	parallel_worker.id = 0;
	parallel_worker.fn = fn;
	parallel_worker.fn_data = data;
	parallel_worker.shared = shared_data;
	ret = pthread_create(&threads[0], NULL,
			     parallel_worker_function, &parallel_worker);

	/*
	 * Barrier thread creation
	 */
	for (i = 1; i < num_threads; i++) {
		parallel_barriers[i - 1].id = i;
		parallel_barriers[i - 1].shared = shared_data;

		ret = pthread_create(&threads[i], NULL,
				     parallel_barrier_function,
				     &parallel_barriers[i - 1]);
	}

	/*
	 * Wait for threads to finish
	 */
	for (i = 0; i < num_threads; i++) {
		ret = pthread_join(threads[i], NULL);
	}

	/*
	 * Cleanup
	 */
	pthread_mutex_destroy(&shared_data->mutex);
	pthread_cond_destroy(&shared_data->cond);
	pthread_key_delete(miniomp_specifickey);

error_key_create:
	free(shared_data);
error_alloc_shared_data:
	free(parallel_barriers);
error_parallel_barrier_alloc:
	free(threads);
}

/*
 * For older compiler versions.
 */
void GOMP_parallel_start(void (*fn)(void *), void *data, unsigned num_threads)
{
	GOMP_parallel(fn, data, num_threads, 0);
}

void GOMP_parallel_end(void)
{
}
