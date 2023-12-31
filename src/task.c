#include <assert.h>
#include "libminiomp.h"

static void task_new_descendant(miniomp_task_t *task, bool is_child,
				bool created_in_taskgroup)
{
	if (!task)
		return;

	task_lock(task);

	task->descendant_count++;

	if (is_child)
		task->children_count++;

	if (created_in_taskgroup)
		task->taskgroup_count++;

	task_unlock(task);

	task_new_descendant(task->parent, false, task->created_in_taskgroup);
}

miniomp_task_t *task_create(miniomp_task_t *parent, miniomp_tasklist_t *tasklist,
			    void (*fn)(void *), void *data, bool created_in_taskgroup)
{
	miniomp_task_t *task = malloc(sizeof(*task));
	if (!task)
		return NULL;

	memset(task, 0, sizeof(*task));

	task->parent = parent;
	task->tasklist = tasklist;
	task->fn = fn;
	task->data = data;
	task->refs = 0;
	task->descendant_count = 0;
	task->children_count = 0;
	task->taskgroup_count = 0;
	task->has_run = false;
	task->in_taskgroup = false;
	task->created_in_taskgroup = created_in_taskgroup;

	pthread_mutex_init(&task->mutex, NULL);
	pthread_cond_init(&task->cond, NULL);

	task_new_descendant(parent, true, created_in_taskgroup);

	return task;
}

void task_destroy(miniomp_task_t *task)
{
	pthread_cond_destroy(&task->cond);
	pthread_mutex_destroy(&task->mutex);
	/*
	 * Don't free parallel's implicit task (parent = NULL) data
	 */
	if (task->parent && task->data)
		free(task->data);
	free(task);
}

bool task_is_valid(const miniomp_task_t *task)
{
	return task && task->fn;
}

int task_lock(miniomp_task_t *task)
{
	return pthread_mutex_lock(&task->mutex);
}

int task_unlock(miniomp_task_t *task)
{
	return pthread_mutex_unlock(&task->mutex);
}

uint32_t task_ref_get(miniomp_task_t *task)
{
	return task->refs++;
}

uint32_t task_ref_put(miniomp_task_t *task)
{
	return task->refs--;
}

static void task_descendant_done_run(miniomp_task_t *task, bool is_child,
				     bool child_created_in_taskgroup)
{
	bool created_in_taskgroup;
	bool destroy = false;

	if (!task)
		return;

	task_lock(task);

	created_in_taskgroup = task->created_in_taskgroup;

	if (task->descendant_count-- == 1) {
		if (task->refs == 0)
			destroy = true;
	}

	if (is_child)
		task->children_count--;

	if (child_created_in_taskgroup)
		task->taskgroup_count--;

	pthread_cond_broadcast(&task->cond);

	task_descendant_done_run(task->parent, false, created_in_taskgroup);

	task_unlock(task);

	if (destroy)
		task_destroy(task);

}

void task_run(miniomp_task_t *task)
{
	uint32_t refs;
	bool destroy;
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *old_task = specific->current_task;
	miniomp_task_t *parent = task->parent;
	bool created_in_taskgroup = task->created_in_taskgroup;

	specific->current_task = task;

	task_lock(task);
	task_ref_get(task);
	task_unlock(task);

	task->fn(task->data);

	task_lock(task);
	task->has_run = true;
	refs = task_ref_put(task);
	destroy = task->descendant_count == 0 && refs == 1;
	if (!destroy)
		pthread_cond_broadcast(&task->cond);
	task_unlock(task);

	if (destroy)
		task_destroy(task);

	task_descendant_done_run(parent, true, created_in_taskgroup);

	specific->current_task = old_task;
}

miniomp_tasklist_t *tasklist_create(void)
{
	miniomp_tasklist_t *tasklist = malloc(sizeof(*tasklist));
	if (!tasklist)
		return NULL;

	miniomp_list_init(&tasklist->list);
	pthread_mutex_init(&tasklist->mutex, NULL);

	return tasklist;
}

void tasklist_destroy(miniomp_tasklist_t *tasklist)
{
	pthread_mutex_destroy(&tasklist->mutex);
	free(tasklist);
}

void tasklist_insert(miniomp_tasklist_t *tasklist, miniomp_task_t *task)
{
	pthread_mutex_lock(&tasklist->mutex);
	miniomp_list_insert(&tasklist->list, &task->link);
	pthread_mutex_unlock(&tasklist->mutex);
}

bool tasklist_is_empty(const miniomp_tasklist_t *tasklist)
{
	return miniomp_list_is_empty(&tasklist->list);
}

miniomp_task_t *tasklist_pop_front(miniomp_tasklist_t *tasklist)
{
	miniomp_task_t *task;

	pthread_mutex_lock(&tasklist->mutex);

	if (tasklist_is_empty(tasklist)) {
		pthread_mutex_unlock(&tasklist->mutex);
		return NULL;
	}

	task = miniomp_list_front(&tasklist->list, task, link);
	miniomp_list_remove(&task->link);

	pthread_mutex_unlock(&tasklist->mutex);

	return task;
}

bool task_meets_dispatch_flags(miniomp_task_t *task,
			       miniomp_tasklist_dispatch_flags_t flags)
{
	if ((flags & TASKLIST_DISPATCH_WAIT_RUN) && !task->has_run)
		return false;

	if (flags & TASKLIST_DISPATCH_FOR_DESCENDANTS)
		return task->descendant_count == 0;
	else if (flags & TASKLIST_DISPATCH_FOR_CHILDREN)
		return task->children_count == 0;
	else if (flags & TASKLIST_DISPATCH_FOR_TASKGROUP)
		return task->taskgroup_count == 0;

	return true;
}

void tasklist_dispatch_for_task(miniomp_tasklist_t *tasklist, miniomp_task_t *task,
				miniomp_tasklist_dispatch_flags_t flags)
{
	while (1) {
		miniomp_task_t *dispatch_task;
		bool valid;

		dispatch_task = tasklist_pop_front(tasklist);

		valid = task_is_valid(dispatch_task);
		if (valid)
			task_run(dispatch_task);

		task_lock(task);

		if (task_meets_dispatch_flags(task, flags)) {
			task_unlock(task);
			break;
		} else if (!valid) {
			pthread_cond_wait(&task->cond, &task->mutex);

			if (task_meets_dispatch_flags(task, flags)) {
				task_unlock(task);
				break;
			}
		}

		task_unlock(task);
	}
}

#define GOMP_TASK_FLAG_UNTIED           (1 << 0)
#define GOMP_TASK_FLAG_FINAL            (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE        (1 << 2)
#define GOMP_TASK_FLAG_DEPEND           (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY         (1 << 4)
#define GOMP_TASK_FLAG_UP               (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE        (1 << 9)
#define GOMP_TASK_FLAG_IF               (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP          (1 << 11)

// Called when encountering an explicit task directive. Arguments are:
//      1. void (*fn) (void *): the generated outlined function for the task body
//      2. void *data: the parameters for the outlined function
//      3. void (*cpyfn) (void *, void *): copy function to replace the default memcpy() from
//                                         function data to each task's data
//      4. long arg_size: specify the size of data
//      5. long arg_align: alignment of the data
//      6. bool if_clause: the value of if_clause. true --> 1, false -->0; default is set to 1 by compiler
//      7. unsigned flags: untied (1) or not (0)

void
GOMP_task(void (*fn)(void *), void *data, void (*cpyfn)(void *, void *),
	  long arg_size, long arg_align, bool if_clause, unsigned flags,
	  void **depend, int priority)
{
	miniomp_task_t *new_task;
	void *task_data;
	miniomp_specific_t *specific = miniomp_get_specific();
	miniomp_task_t *cur_task = specific->current_task;
	miniomp_tasklist_t *tasklist = cur_task->tasklist;

	dbgprintf("GOMP_task called, size: %ld, align: %ld\n", arg_size, arg_align);

	if (posix_memalign(&task_data, max(arg_align, sizeof(void *)), arg_size)) {
		errprintf("Error: can't allocate memory for the task data\n");
		return;
	}

	if (__builtin_expect(cpyfn != NULL, 0))
		cpyfn(task_data, data);
	else
		memcpy(task_data, data, arg_size);

	new_task = task_create(cur_task, tasklist, fn, task_data, cur_task->in_taskgroup);
	if (!new_task) {
		free(task_data);
		errprintf("Error: can't allocate memory for the task\n");
		return;
	}

	tasklist_insert(tasklist, new_task);
}
