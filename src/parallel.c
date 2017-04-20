#include "libminiomp.h"

static void parallel_implicit_barrier(miniomp_tasklist_t *tasklist,
				      miniomp_task_t *task)
{
	tasklist_dispatch_for_task(tasklist, task,
				   TASKLIST_DISPATCH_FOR_DESCENDANTS |
				   TASKLIST_DISPATCH_WAIT_RUN);
}

static void *parallel_worker_function(void *args)
{
	miniomp_specific_t specific;
	miniomp_parallel_worker_args_t *worker = args;
	miniomp_task_t *task = worker->current_task;
	miniomp_tasklist_t *tasklist = task->tasklist;

	specific.id = worker->id;
	specific.current_task = task;
	miniomp_set_specific(&specific);

	/*
	 * Implicit barrier of the parallel clause
	 */
	parallel_implicit_barrier(tasklist, task);

	return NULL;
}

static void *parallel_barrier_function(void *args)
{
	miniomp_specific_t specific;
	miniomp_parallel_barrier_args_t *barrier = args;
	miniomp_task_t *task = barrier->current_task;
	miniomp_tasklist_t *tasklist = task->tasklist;

	specific.id = barrier->id;
	specific.current_task = task;
	miniomp_set_specific(&specific);

	parallel_implicit_barrier(tasklist, task);

	return NULL;
}

void
GOMP_parallel(void (*fn)(void *), void *data, unsigned num_threads,
	      unsigned int flags)
{
	unsigned i;
	pthread_t *threads;
	miniomp_parallel_worker_args_t parallel_worker;
	miniomp_parallel_barrier_args_t *parallel_barriers;
	miniomp_task_t *implicit_task;
	miniomp_tasklist_t *tasklist;

	if (!num_threads)
		num_threads = omp_get_num_threads();

	dbgprintf("Starting a parallel region using %d threads\n", num_threads);

	threads = calloc(num_threads, sizeof(*threads));
	if (!threads) {
		errprintf("Error allocating thread IDs\n");
		return;
	}

	parallel_barriers = calloc(num_threads - 1, sizeof(*parallel_barriers));
	if (!parallel_barriers) {
		errprintf("Error allocating parallel_barriers\n");
		goto error_parallel_barrier_alloc;
	}

	tasklist = tasklist_create();
	if (!tasklist) {
		errprintf("Error creating the parallel tasklist\n");
		goto error_tasklist_create;
	}

	/*
	 * OMP parallel construct creates an implicit task.
	 */
	implicit_task = task_create(NULL, tasklist, fn, data, false);
	if (!implicit_task) {
		errprintf("Error creating the parallel implicit task\n");
		goto error_implicit_task_create;
	}

	task_ref_get(implicit_task);

	tasklist_insert(tasklist, implicit_task);

	/*
	 * Parallel-single thread creation
	 */
	parallel_worker.id = 0;
	parallel_worker.current_task = implicit_task;
	pthread_create(&threads[0], NULL,
		       parallel_worker_function, &parallel_worker);

	/*
	 * Barrier thread creation
	 */
	for (i = 0; i < num_threads - 1; i++) {
		parallel_barriers[i].id = i + 1;
		parallel_barriers[i].current_task = implicit_task;
		pthread_create(&threads[i + 1], NULL, parallel_barrier_function,
			       &parallel_barriers[i]);
	}

	/*
	 * Wait for threads to finish
	 */
	for (i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	task_ref_put(implicit_task);
	task_destroy(implicit_task);

	/* fallthrough */
error_implicit_task_create:
	tasklist_destroy(tasklist);
error_tasklist_create:
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
