#include <pthread.h>

typedef struct {
	int count;
	bool done;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
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

typedef struct {
	miniomp_parallel_shared_data_t *shared;
} miniomp_parallel_global_data_t;

extern pthread_key_t miniomp_specifickey;
extern miniomp_parallel_global_data_t miniomp_global_data;


void GOMP_parallel(void (*fn) (void *), void *data, unsigned num_threads,
		   unsigned int flags);
