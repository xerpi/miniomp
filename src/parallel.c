#include "libminiomp.h"

miniomp_parallel_global_data_t miniomp_global_data;

static void parallel_task_barrier(miniomp_parallel_shared_data_t *shared, miniomp_task_t *cur_task)
{
	miniomp_taskbatch_t *children_batch = cur_task->children_batch;

	taskbatch_dispatch(children_batch, true);
}

// This is the prototype for the Pthreads starting function
static void *parallel_worker_function(void *args)
{
	miniomp_specific_t specific;
	miniomp_parallel_worker_t *worker = args;
	miniomp_task_t *cur_task = worker->current_task;

	specific.id = worker->id;
	specific.current_task = cur_task;
	pthread_setspecific(miniomp_specifickey, &specific);

	worker->fn(worker->fn_data);

	taskbatch_lock(cur_task->children_batch);
	taskbatch_ref_put(cur_task->children_batch);
	taskbatch_unlock(cur_task->children_batch);

	/*
	 * Join the implicit barrier of the parallel clause
	 */
	//parallel_implicit_barrier(worker->shared);
	parallel_task_barrier(worker->shared, worker->current_task);

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
	specific.current_task = barrier->current_task;
	pthread_setspecific(miniomp_specifickey, &specific);

	//parallel_implicit_barrier(barrier->shared);
	parallel_task_barrier(barrier->shared, barrier->current_task);

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
	pthread_t *threads;
	miniomp_parallel_worker_t parallel_worker;
	miniomp_parallel_barrier_t *parallel_barriers;
	miniomp_parallel_shared_data_t *shared_data;
	miniomp_task_t *current_task;

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

	current_task = miniomp_get_specific()->current_task;

	taskbatch_ref_get(current_task->children_batch);

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
	parallel_worker.current_task = current_task;
	pthread_create(&threads[0], NULL,
		       parallel_worker_function, &parallel_worker);

	/*
	 * Barrier thread creation
	 */
	for (i = 1; i < num_threads; i++) {
		parallel_barriers[i - 1].id = i;
		parallel_barriers[i - 1].current_task = current_task;
		parallel_barriers[i - 1].shared = shared_data;

		pthread_create(&threads[i], NULL, parallel_barrier_function,
			       &parallel_barriers[i - 1]);
	}

	/*
	 * Wait for threads to finish
	 */
	for (i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	/*
	 * Cleanup
	 */
	pthread_mutex_destroy(&shared_data->mutex);
	pthread_cond_destroy(&shared_data->cond);

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
