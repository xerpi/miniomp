#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <pthread.h>
#include "list.h"

typedef struct miniomp_tasklist_t {
	miniomp_list_t list;
	pthread_mutex_t mutex;
} miniomp_tasklist_t;

typedef enum miniomp_tasklist_dispatch_flags_t {
	TASKLIST_DISPATCH_FOR_DESCENDANTS,
	TASKLIST_DISPATCH_FOR_CHILDREN,
	TASKLIST_DISPATCH_FOR_TASKGROUP
} miniomp_tasklist_dispatch_flags_t;

typedef struct miniomp_task_t {
	struct miniomp_task_t *parent;
	miniomp_tasklist_t *tasklist;

	void (*fn)(void *);
	void *data;

	uint32_t refs;

	uint32_t descendant_count;
	uint32_t children_count;
	uint32_t taskgroup_count;

	bool has_run;
	bool in_taskgroup;
	bool created_in_taskgroup;

	pthread_mutex_t mutex;
	pthread_cond_t cond;

	miniomp_list_t link;
} miniomp_task_t;

miniomp_task_t *task_create(miniomp_task_t *parent, miniomp_tasklist_t *tasklist,
			    void (*fn)(void *), void *data, bool created_in_taskgroup);
void task_destroy(miniomp_task_t *task);
bool task_is_valid(const miniomp_task_t *task);
int task_lock(miniomp_task_t *task);
int task_unlock(miniomp_task_t *task);
uint32_t task_ref_get(miniomp_task_t *task);
uint32_t task_ref_put(miniomp_task_t *task);
void task_run(miniomp_task_t *task);

miniomp_tasklist_t *tasklist_create(void);
void tasklist_destroy(miniomp_tasklist_t *tasklist);
void tasklist_insert(miniomp_tasklist_t *tasklist, miniomp_task_t *task);
bool tasklist_is_empty(const miniomp_tasklist_t *tasklist);
miniomp_task_t *tasklist_pop_front(miniomp_tasklist_t *tasklist);
void tasklist_dispatch_for_task(miniomp_tasklist_t *tasklist, miniomp_task_t *task,
				miniomp_tasklist_dispatch_flags_t flags);

#define tasklist_front(tasklist, sample, member) \
	miniomp_list_front(&(tasklist)->task_list, sample, member)

#endif
