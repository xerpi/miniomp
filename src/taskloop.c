#include "libminiomp.h"

#define GOMP_TASK_FLAG_UNTIED           (1 << 0)
#define GOMP_TASK_FLAG_FINAL            (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE        (1 << 2)
#define GOMP_TASK_FLAG_DEPEND           (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY         (1 << 4)
#define GOMP_TASK_FLAG_UP               (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE        (1 << 9)
#define GOMP_TASK_FLAG_IF               (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP          (1 << 11)

void
GOMP_taskloop(void (*fn)(void *), void *data, void (*cpyfn)(void *, void *),
	      long arg_size, long arg_align, unsigned flags,
	      unsigned long num_tasks, int priority,
	      long start, long end, long step)
{
	int i;
	uint32_t grainsize;
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *cur_task = specific->current_task;
	miniomp_tasklist_t *cur_tasklist = cur_task->tasklist;

	dbgprintf("GOMP_taskloop with step=%ld, ", step);

	if (flags & GOMP_TASK_FLAG_GRAINSIZE) {
		grainsize = num_tasks;
		num_tasks = (end - start + (step - 1)) / step;
	} else {
		if (num_tasks == 0)
			num_tasks = omp_get_num_threads();
		grainsize = (end - start + (num_tasks - 1)) / num_tasks;
	}

	dbgprintf("grainsize=%d, num_tasks=%ld\n", grainsize, num_tasks);

	for (i = 0; i < num_tasks; i++) {
		miniomp_task_t *new_task;
		void *task_data;

		if (posix_memalign(&task_data, max(arg_align, sizeof(void *)), arg_size)) {
			errprintf("Error: can't allocate memory for the task data\n");
			continue;
		}

		if (__builtin_expect(cpyfn != NULL, 0))
			cpyfn(task_data, data);
		else
			memcpy(task_data, data, arg_size);

		((long *)task_data)[0] = start + grainsize * i;
		((long *)task_data)[1] = ((long *)task_data)[0] + grainsize;

		/*
		 * By default, the taskloop construct executes as if
		 * it was enclosed in a taskgroup construct.
		 */
		new_task = task_create(cur_task, cur_tasklist, fn, task_data, true);
		if (!new_task) {
			free(task_data);
			errprintf("Error: can't allocate memory for the task\n");
			continue;
		}

		tasklist_insert(cur_tasklist, new_task);
	}

	tasklist_dispatch_for_task(cur_tasklist, cur_task,
				   TASKLIST_DISPATCH_FOR_TASKGROUP);
}
