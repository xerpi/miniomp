#include <assert.h>
#include "libminiomp.h"

void GOMP_taskwait(void)
{
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *cur_task = specific->current_task;

	printf("GOMP_taskwait()\n");

	/*
	 * Wait until all the children tasks are done running.
	 */
	taskbatch_dispatch(cur_task->children_batch, false);
}

void GOMP_taskgroup_start(void)
{
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *cur_task = specific->current_task;

	printf("GOMP_taskgroup_start()\n");

	assert(cur_task->taskgroup == NULL);

	cur_task->taskgroup = taskgroup_create();
	assert(cur_task->taskgroup != NULL);

}

void GOMP_taskgroup_end(void)
{
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *cur_task = specific->current_task;

	printf("GOMP_taskgroup_end()\n");

	if (cur_task->taskgroup == NULL)
		return;

	taskgroup_dispatch(cur_task->taskgroup);
	taskgroup_destroy(cur_task->taskgroup);

	cur_task->taskgroup = NULL;
}
