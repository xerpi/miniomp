#include <stdbool.h>
#include <pthread.h>

#define TASKQUEUE_DEFAULT_SIZE 128

typedef struct miniomp_task_t miniomp_task_t;
typedef struct miniomp_taskqueue_t miniomp_taskqueue_t;

extern miniomp_taskqueue_t *miniomp_taskqueue;

miniomp_task_t *task_create(void (*fn)(void *), void *data);
void task_destroy(miniomp_task_t *task);
bool task_is_valid(const miniomp_task_t *task);
void task_run(const miniomp_task_t *task);

miniomp_taskqueue_t *taskqueue_create(int max_elements);
void taskqueue_destroy(miniomp_taskqueue_t *taskqueue);
bool taskqueue_is_empty(const miniomp_taskqueue_t *taskqueue);
bool taskqueue_is_full(const miniomp_taskqueue_t *taskqueue);
bool taskqueue_enqueue(miniomp_taskqueue_t *taskqueue, miniomp_task_t *task);
miniomp_task_t *taskqueue_dequeue(miniomp_taskqueue_t *taskqueue);
