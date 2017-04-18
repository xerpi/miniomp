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
	task_dispatch(cur_task, false);
}

void GOMP_taskgroup_start(void)
{
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *task = specific->current_task;

	printf("GOMP_taskgroup_start()\n");

	assert(taskbatch_is_empty(task->taskgroup_batch));

	task->in_taskgroup = true;
}

void GOMP_taskgroup_end(void)
{
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *task = specific->current_task;

	printf("GOMP_taskgroup_end()\n");

	task_dispatch(task, true);

	task->in_taskgroup = false;
}
