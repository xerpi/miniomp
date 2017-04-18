#include "libminiomp.h"

void init_miniomp(void) __attribute__((constructor));
void fini_miniomp(void) __attribute__((destructor));

pthread_key_t miniomp_specifickey;
static miniomp_task_t *root_task;
static miniomp_specific_t root_specific;

void init_miniomp(void)
{
	int ret;
	printf("mini-omp is being initialized\n");

	// Parse OMP_NUM_THREADS environment variable to initialize nthreads_var internal control variable
	parse_env();

	// Initialize Pthread data structures and thread-specific data
	// Initialize OpenMP default lock and default barrier
	// Initialize OpenMP workdescriptors for loop and single and taskqueue

	ret = pthread_key_create(&miniomp_specifickey, NULL);
	if (ret != 0) {
		printf("Error pthread_key_create(): %d\n", ret);
		return;
	}

	root_task = task_create(NULL, NULL);

	root_specific.id = 0;
	root_specific.current_task = root_task;

	pthread_setspecific(miniomp_specifickey, &root_specific);

	//miniomp_taskqueue = taskqueue_create(TASKQUEUE_DEFAULT_SIZE);
}

void fini_miniomp(void)
{
	// free structures allocated during library initialization
	//taskqueue_destroy(miniomp_taskqueue);
	task_destroy(root_task);

	pthread_key_delete(miniomp_specifickey);

	printf("mini-omp is finalized\n");
}


miniomp_specific_t *miniomp_get_specific(void)
{
	return pthread_getspecific(miniomp_specifickey);
}
