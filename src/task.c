#include <assert.h>
#include "libminiomp.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))

miniomp_task_t *task_create(void (*fn)(void *), void *data)
{
	miniomp_task_t *task = malloc(sizeof(*task));
	if (!task)
		return NULL;

	task->children_batch = taskbatch_create();
	if (!task->children_batch) {
		free(task);
		return NULL;
	}

	task->taskgroup_batch = taskbatch_create();
	if (!task->taskgroup_batch) {
		taskbatch_destroy(task->children_batch);
		free(task);
		return NULL;
	}

	miniomp_list_init(&task->children_link);
	miniomp_list_init(&task->taskgroup_link);

	task->fn = fn;
	task->data = data;
	task->refs = 0;
	task->in_taskgroup = false;

	pthread_mutex_init(&task->mutex, NULL);
	pthread_cond_init(&task->cond, NULL);

	return task;
}

void task_destroy(miniomp_task_t *task)
{
	taskbatch_destroy(task->children_batch);
	taskbatch_destroy(task->taskgroup_batch);
	pthread_mutex_destroy(&task->mutex);
	pthread_cond_destroy(&task->cond);
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

void task_run(miniomp_task_t *task)
{
	miniomp_task_t *old_task = miniomp_get_specific()->current_task;

	miniomp_get_specific()->current_task = task;
	task->fn(task->data);
	miniomp_get_specific()->current_task = old_task;
}

int task_ref_get(miniomp_task_t *task)
{
	return task->refs++;
}

int task_ref_put(miniomp_task_t *task)
{
	return task->refs--;
}

void task_dispatch(miniomp_task_t *task, bool recursively)
{
	while (1) {
		uint32_t cur_refs;
		miniomp_task_t *child = NULL;

		task_lock(task);

		/*
		 * taskgroup tasks have priority over regular children
		 */
		if (!taskbatch_is_empty(task->taskgroup_batch)) {
			child = taskbatch_front(task->taskgroup_batch,
						child, taskgroup_link);
		} else if (!taskbatch_is_empty(task->children_batch)) {
			child = taskbatch_front(task->children_batch,
						child, children_link);
		}

		cur_refs = task_ref_get(task);

		if (task_is_valid(child)) {
			miniomp_list_remove(&child->children_link);
			if (!miniomp_list_is_empty(&child->taskgroup_link))
				miniomp_list_remove(&child->taskgroup_link);

			task_unlock(task);

			task_run(child);

			if (recursively)
				task_dispatch(child, true);

			task_destroy(child);

			task_lock(task);
			task_ref_put(task);
			task_unlock(task);
		} else {
			task_ref_put(task);
			if (cur_refs == 0) {
				task_unlock(task);
				pthread_cond_broadcast(&task->cond);
				break;
			} else {
				pthread_cond_wait(&task->cond, &task->mutex);
				task_unlock(task);
			}
		}
	}

#if 0
	do {
		uint32_t cur_refs;
		miniomp_task_t *task;

		taskbatch_lock(taskbatch);
		task = taskbatch_dequeue(taskbatch);
		cur_refs = taskbatch_ref_get(taskbatch);
		taskbatch_unlock(taskbatch);

		if (task_is_valid(task)) {
			task_run(task);

			if (recursively)
				taskbatch_dispatch(task->children_batch, true);

			task_destroy(task);

			taskbatch_lock(taskbatch);
			taskbatch_ref_put(taskbatch);
			taskbatch_unlock(taskbatch);
		} else {
			taskbatch_lock(taskbatch);
			taskbatch_ref_put(taskbatch);
			if (cur_refs == 0) {
				taskbatch_set_done(taskbatch, true);
				taskbatch_broadcast(taskbatch);
			} else {
				if (!taskbatch_get_done(taskbatch))
					taskbatch_wait(taskbatch);
			}
			taskbatch_unlock(taskbatch);

		}
	} while (!taskbatch_get_done(taskbatch));
#endif
}

miniomp_taskbatch_t *taskbatch_create(void)
{
	miniomp_taskbatch_t *taskbatch = malloc(sizeof(*taskbatch));
	if (!taskbatch)
		return NULL;

	miniomp_list_init(&taskbatch->task_list);
	taskbatch->done = false;

	return taskbatch;
}

void taskbatch_destroy(miniomp_taskbatch_t *taskbatch)
{
	free(taskbatch);
}

void taskbatch_insert(miniomp_taskbatch_t *taskbatch, miniomp_list_t *task_link)
{
	miniomp_list_insert(&taskbatch->task_list, task_link);
}

void taskbatch_remove(miniomp_list_t *task_link)
{
	miniomp_list_remove(task_link);
}

bool taskbatch_is_empty(const miniomp_taskbatch_t *taskbatch)
{
	return miniomp_list_is_empty(&taskbatch->task_list);
}

void taskbatch_set_done(miniomp_taskbatch_t *taskbatch, bool done)
{
	taskbatch->done = done;
}

bool taskbatch_get_done(const miniomp_taskbatch_t *taskbatch)
{
	return taskbatch->done;
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

	//printf("GOMP_task called, size: %ld, align: %ld\n", arg_size, arg_align);

	if (posix_memalign(&task_data, max(arg_align, sizeof(void *)), arg_size)) {
		printf("Error: can't allocate memory for the task data\n");
		return;
	}

	if (__builtin_expect(cpyfn != NULL, 0))
		cpyfn(task_data, data);
	else
		memcpy(task_data, data, arg_size);

	new_task = task_create(fn, task_data);
	if (!new_task) {
		free(task_data);
		printf("Error: can't allocate memory for the task\n");
		return;
	}

	//printf("Current task: %p\n", cur_task);
	//printf("New task: %p\n", new_task);

	task_lock(cur_task);

	taskbatch_insert(cur_task->children_batch,
			 &new_task->children_link);

	if (cur_task->in_taskgroup)
		taskbatch_insert(cur_task->taskgroup_batch,
			         &new_task->taskgroup_link);

	task_unlock(cur_task);

	/*
	 * Wake up threads waiting for the signal.
	 */
	pthread_cond_signal(&cur_task->cond);
}
