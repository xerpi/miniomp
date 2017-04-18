#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <pthread.h>
#include "list.h"

typedef struct miniomp_taskbatch_t {
	list_t *task_list;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	uint32_t refs;
	bool done;
} miniomp_taskbatch_t;

typedef struct miniomp_task_t {
	void (*fn)(void *);
	void *data;
	miniomp_taskbatch_t *children_batch;
} miniomp_task_t;

miniomp_task_t *task_create(void (*fn)(void *), void *data);
void task_destroy(miniomp_task_t *task);
bool task_is_valid(const miniomp_task_t *task);
void task_run(const miniomp_task_t *task);

miniomp_taskbatch_t *taskbatch_create(void);
void taskbatch_destroy(miniomp_taskbatch_t *taskbatch);
int taskbatch_enqueue(miniomp_taskbatch_t *taskbatch, miniomp_task_t *task);
miniomp_task_t *taskbatch_dequeue(miniomp_taskbatch_t *taskbatch);
int taskbatch_lock(miniomp_taskbatch_t *taskbatch);
int taskbatch_unlock(miniomp_taskbatch_t *taskbatch);
int taskbatch_wait(miniomp_taskbatch_t *taskbatch);
int taskbatch_signal(miniomp_taskbatch_t *taskbatch);
int taskbatch_broadcast(miniomp_taskbatch_t *taskbatch);
int taskbatch_ref_get(miniomp_taskbatch_t *taskbatch);
int taskbatch_ref_put(miniomp_taskbatch_t *taskbatch);
void taskbatch_set_done(miniomp_taskbatch_t *taskbatch, bool done);
bool taskbatch_get_done(const miniomp_taskbatch_t *taskbatch);

#endif
