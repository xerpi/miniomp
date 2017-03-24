#include <pthread.h>

typedef struct {
	int count;
} miniomp_parallel_shared_data_t;

typedef struct {
	int id;
	void (*fn)(void *);
	void *fn_data;
	miniomp_parallel_shared_data_t *shared;
} miniomp_parallel_worker_t;

typedef struct {
	int id;
	miniomp_parallel_shared_data_t *shared;
} miniomp_parallel_barrier_t;

typedef struct {
	int id;
} miniomp_specific_t;

// Declaration of per-thread specific key
extern pthread_key_t miniomp_specifickey;

// Functions implemented in this module
void GOMP_parallel(void (*fn) (void *), void *data, unsigned num_threads,
		   unsigned int flags);
