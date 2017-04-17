#include "libminiomp.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))

miniomp_taskqueue_t *miniomp_taskqueue;

struct miniomp_task_t {
	void (*fn)(void *);
	void *data;
};

struct miniomp_taskqueue_t {
	int max_elements;
	int count;
	int head;
	int tail;
	miniomp_task_t **queue;
};

miniomp_task_t *task_create(void (*fn)(void *), void *data)
{
	miniomp_task_t *task = malloc(sizeof(*task));
	if (!task)
		return NULL;

	task->fn = fn;
	task->data = data;

	return task;
}

void task_destroy(miniomp_task_t *task)
{
	free(task);
}

bool task_is_valid(const miniomp_task_t *task)
{
	return task && task->fn;
}

void task_run(const miniomp_task_t *task)
{
	task->fn(task->data);
}

miniomp_taskqueue_t *taskqueue_create(int max_elements)
{
	miniomp_taskqueue_t *taskqueue;

	taskqueue = malloc(sizeof(*taskqueue));
	if (!taskqueue)
		return NULL;

	taskqueue->queue = malloc(max_elements * sizeof(*taskqueue->queue));
	if (!taskqueue->queue)
		goto error_queue_alloc;

	taskqueue->max_elements = max_elements;
	taskqueue->count = 0;
	taskqueue->head = 0;
	taskqueue->tail = 0;

	return taskqueue;

error_queue_alloc:
	free(taskqueue);
	return NULL;
}

void taskqueue_destroy(miniomp_taskqueue_t *taskqueue)
{
	free(taskqueue->queue);
	free(taskqueue);
}

bool taskqueue_is_empty(const miniomp_taskqueue_t *taskqueue)
{
	return taskqueue->count == 0;
}

bool taskqueue_is_full(const miniomp_taskqueue_t *taskqueue)
{
	return taskqueue->count == taskqueue->max_elements;
}

bool taskqueue_enqueue(miniomp_taskqueue_t *taskqueue, miniomp_task_t *task)
{
	if (taskqueue_is_full(taskqueue))
		return false;

	taskqueue->queue[taskqueue->tail] = task;
	taskqueue->count++;
	taskqueue->tail = (taskqueue->tail + 1) % taskqueue->max_elements;

	return true;
}

miniomp_task_t *taskqueue_dequeue(miniomp_taskqueue_t *taskqueue)
{
	miniomp_task_t *task;

	if (taskqueue_is_empty(taskqueue))
		return NULL;

	task = taskqueue->queue[taskqueue->head];
	taskqueue->count--;
	taskqueue->head = (taskqueue->head + 1) % taskqueue->max_elements;

	return task;
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
	miniomp_task_t *task;
	void *task_data;

	printf("GOMP_task called, size: %ld, align: %ld\n", arg_size, arg_align);

	if (posix_memalign(&task_data, max(arg_align, sizeof(void *)), arg_size)) {
		printf("Error: can't allocate memory for the task data\n");
		return;
	}

	if (__builtin_expect(cpyfn != NULL, 0))
		cpyfn(task_data, data);
	else
		memcpy(task_data, data, arg_size);

	task = task_create(fn, task_data);
	if (!task) {
		free(task_data);
		printf("Error: can't allocate memory for the task\n");
		return;
	}

	pthread_mutex_lock(&miniomp_global_data.shared->mutex);
	taskqueue_enqueue(miniomp_taskqueue, task);
	pthread_mutex_unlock(&miniomp_global_data.shared->mutex);

	/*
	 * Wake waiting threads at the implicit parallel barrier
	 */
	pthread_cond_signal(&miniomp_global_data.shared->cond);
}
