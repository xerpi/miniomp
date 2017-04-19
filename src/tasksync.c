#include <assert.h>
#include "libminiomp.h"

void GOMP_taskwait(void)
{
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *task = specific->current_task;
	miniomp_tasklist_t *tasklist = task->tasklist;

	dbgprintf("GOMP_taskwait()\n");

	/*
	 * Wait until all the children tasks are done running.
	 */
	tasklist_dispatch_for_task(tasklist, task,
				   TASKLIST_DISPATCH_FOR_CHILDREN);
}

void GOMP_taskgroup_start(void)
{
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *task = specific->current_task;

	dbgprintf("GOMP_taskgroup_start()\n");

	task_lock(task);
	task->in_taskgroup = true;
	task_unlock(task);
}

void GOMP_taskgroup_end(void)
{
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *task = specific->current_task;
	miniomp_tasklist_t *tasklist = task->tasklist;

	dbgprintf("GOMP_taskgroup_end()\n");

	task_lock(task);
	task->in_taskgroup = false;
	task_unlock(task);

	tasklist_dispatch_for_task(tasklist, task,
				   TASKLIST_DISPATCH_FOR_TASKGROUP);
}
