#include <pthread.h>
#include "task.h"

typedef struct miniomp_parallel_worker_args_t {
	int id;
	void (*fn)(void *);
	void *fn_data;
	miniomp_task_t *current_task;
} miniomp_parallel_worker_args_t;

typedef struct miniomp_parallel_barrier_args_t {
	int id;
	miniomp_task_t *current_task;
} miniomp_parallel_barrier_args_t;

extern pthread_key_t miniomp_specific_key;

void GOMP_parallel(void (*fn) (void *), void *data, unsigned num_threads,
		   unsigned int flags);
