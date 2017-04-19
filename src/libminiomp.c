#include "libminiomp.h"

void init_miniomp(void) __attribute__((constructor));
void fini_miniomp(void) __attribute__((destructor));

pthread_key_t miniomp_specific_key;

void init_miniomp(void)
{
	int ret;

	dbgprintf("mini-omp is being initialized\n");

	parse_env();

	ret = pthread_key_create(&miniomp_specific_key, NULL);
	if (ret != 0) {
		errprintf("Error pthread_key_create(): %d\n", ret);
		return;
	}

	miniomp_set_specific(NULL);
}

void fini_miniomp(void)
{
	pthread_key_delete(miniomp_specific_key);

	dbgprintf("mini-omp is finalized\n");
}

miniomp_specific_t *miniomp_get_specific(void)
{
	return pthread_getspecific(miniomp_specific_key);
}

int miniomp_set_specific(const void *value)
{
	return pthread_setspecific(miniomp_specific_key, value);
}
