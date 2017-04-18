#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <pthread.h>
#include "list.h"

typedef struct miniomp_taskbatch_t {
	miniomp_list_t task_list;
	uint32_t refs;
	bool done;
} miniomp_taskbatch_t;

typedef struct miniomp_task_t {
	void (*fn)(void *);
	void *data;

	uint32_t refs;

	pthread_mutex_t mutex;

	/*
	 * Used to signal when a new child task is created.
	 */
	pthread_cond_t cond;

	/*
	 * Parent task. NULL if it is the root task.
	 */
	struct miniomp_task_t *parent;

	/*
	 * Children tasks of this one.
	 */
	miniomp_taskbatch_t *children_batch;

	/*
	 * Link to the children task list of the parent.
	 */
	miniomp_list_t children_link;

	/*
	 * taskgroup children of this task.
	 */
	miniomp_taskbatch_t *taskgroup_batch;

	/*
	 * Link to the taskgroup task list of the parent.
	 */
	miniomp_list_t taskgroup_link;

	/*
	 * Wheter it is within a taskgroup_{start,end} region.
	 */
	bool in_taskgroup;
} miniomp_task_t;

miniomp_task_t *task_create(void (*fn)(void *), void *data);
void task_destroy(miniomp_task_t *task);
bool task_is_valid(const miniomp_task_t *task);
int task_lock(miniomp_task_t *task);
int task_unlock(miniomp_task_t *task);
int task_ref_get(miniomp_task_t *task);
int task_ref_put(miniomp_task_t *task);
void task_run(miniomp_task_t *task);
void task_dispatch(miniomp_task_t *task, bool recursively);

miniomp_taskbatch_t *taskbatch_create(void);
void taskbatch_destroy(miniomp_taskbatch_t *taskbatch);
void taskbatch_insert(miniomp_taskbatch_t *taskbatch, miniomp_list_t *task_link);
void taskbatch_remove(miniomp_list_t *task_link);
bool taskbatch_is_empty(const miniomp_taskbatch_t *taskbatch);
void taskbatch_set_done(miniomp_taskbatch_t *taskbatch, bool done);
bool taskbatch_get_done(const miniomp_taskbatch_t *taskbatch);

#define taskbatch_front(taskbatch, sample, member) \
	miniomp_list_front(&(taskbatch)->task_list, sample, member)

#endif
