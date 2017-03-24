#include <stdbool.h>
#include <pthread.h>

#define TASKQUEUE_DEFAULT_SIZE 128

typedef struct {
	void (*fn)(void *);
	void *data;
} miniomp_task_t;

typedef struct {
	int max_elements;
	int count;
	int head;
	int tail;
	pthread_mutex_t mutex;
	miniomp_task_t **queue;
} miniomp_taskqueue_t;

extern miniomp_taskqueue_t *miniomp_taskqueue;

bool task_is_valid(miniomp_task_t *task);

miniomp_taskqueue_t *taskqueue_create(int max_elements);
void taskqueue_destroy(miniomp_taskqueue_t *taskqueue);
bool taskqueue_is_empty(miniomp_taskqueue_t *taskqueue);
bool taskqueue_is_full(miniomp_taskqueue_t *taskqueue);
bool taskqueue_enqueue(miniomp_taskqueue_t *taskqueue, miniomp_task_t *task);
miniomp_task_t *taskqueue_dequeue(miniomp_taskqueue_t *taskqueue);
