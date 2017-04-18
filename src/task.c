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

	task->fn = fn;
	task->data = data;

	return task;
}

void task_destroy(miniomp_task_t *task)
{
	taskbatch_destroy(task->children_batch);
	free(task);
}

bool task_is_valid(const miniomp_task_t *task)
{
	return task && task->fn;
}

void task_run(const miniomp_task_t *task)
{
	miniomp_task_t *old_task = miniomp_get_specific()->current_task;

	miniomp_get_specific()->current_task = task;
	task->fn(task->data);
	miniomp_get_specific()->current_task = old_task;
}

miniomp_taskbatch_t *taskbatch_create(void)
{
	miniomp_taskbatch_t *taskbatch = malloc(sizeof(*taskbatch));
	if (!taskbatch)
		return NULL;

	taskbatch->task_list = list_create();
	if (!taskbatch->task_list) {
		free(taskbatch);
		return NULL;
	}

	taskbatch->refs = 0;
	taskbatch->done = false;
	pthread_mutex_init(&taskbatch->mutex, NULL);
	pthread_cond_init(&taskbatch->cond, NULL);

	return taskbatch;
}

void taskbatch_destroy(miniomp_taskbatch_t *taskbatch)
{
	pthread_mutex_destroy(&taskbatch->mutex);
	pthread_cond_destroy(&taskbatch->cond);
	list_destroy(taskbatch->task_list);
	free(taskbatch);
}

int taskbatch_enqueue(miniomp_taskbatch_t *taskbatch, miniomp_task_t *task)
{
	return list_enqueue(taskbatch->task_list, task);
}

miniomp_task_t *taskbatch_dequeue(miniomp_taskbatch_t *taskbatch)
{
	return list_dequeue(taskbatch->task_list);
}

int taskbatch_lock(miniomp_taskbatch_t *taskbatch)
{
	return pthread_mutex_lock(&taskbatch->mutex);
}

int taskbatch_unlock(miniomp_taskbatch_t *taskbatch)
{
	return pthread_mutex_unlock(&taskbatch->mutex);
}

int taskbatch_wait(miniomp_taskbatch_t *taskbatch)
{
	return pthread_cond_wait(&taskbatch->cond, &taskbatch->mutex);
}

int taskbatch_signal(miniomp_taskbatch_t *taskbatch)
{
	return pthread_cond_signal(&taskbatch->cond);
}

int taskbatch_broadcast(miniomp_taskbatch_t *taskbatch)
{
	return pthread_cond_broadcast(&taskbatch->cond);
}

int taskbatch_ref_get(miniomp_taskbatch_t *taskbatch)
{
	return taskbatch->refs++;
}

int taskbatch_ref_put(miniomp_taskbatch_t *taskbatch)
{
	return taskbatch->refs--;
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

	taskbatch_lock(cur_task->children_batch);
	taskbatch_enqueue(cur_task->children_batch, new_task);
	taskbatch_unlock(cur_task->children_batch);

	/*
	 * Wake waiting threads at the implicit parallel barrier
	 */
	taskbatch_signal(cur_task->children_batch);
}
