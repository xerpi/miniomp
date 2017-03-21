#include "libminiomp.h"
//#include "intrinsic.h"

// Library constructor and desctructor
void init_miniomp(void) __attribute__((constructor));
void fini_miniomp(void) __attribute__((destructor));

// Function to parse OMP_NUM_THREADS environment variable
void parse_env(void);

void init_miniomp(void)
{
	printf("mini-omp is being initialized\n");

	// Parse OMP_NUM_THREADS environment variable to initialize nthreads_var internal control variable
	parse_env();

	// Initialize Pthread data structures and thread-specific data
	// Initialize OpenMP default lock and default barrier
	// Initialize OpenMP workdescriptors for loop and single and taskqueue
	miniomp_taskqueue = taskqueue_create(TASKQUEUE_DEFAULT_SIZE);

}

void fini_miniomp(void)
{
	// free structures allocated during library initialization
	taskqueue_destroy(miniomp_taskqueue);

	printf("mini-omp is finalized\n");
}
