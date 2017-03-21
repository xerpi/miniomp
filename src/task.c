#include "libminiomp.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))

miniomp_taskqueue_t *miniomp_taskqueue;

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

	if (pthread_mutex_init(&taskqueue->mutex, NULL) != 0)
		goto error_mutex_init;

	return taskqueue;

error_mutex_init:
	free(taskqueue->queue);
error_queue_alloc:
	free(taskqueue);
	return NULL;
}

void taskqueue_destroy(miniomp_taskqueue_t *taskqueue)
{
	pthread_mutex_destroy(&taskqueue->mutex);
	free(taskqueue->queue);
	free(taskqueue);
}

bool task_is_valid(miniomp_task_t *task)
{
	return false;
}

bool taskqueue_is_empty(miniomp_taskqueue_t *taskqueue)
{
	return taskqueue->count == 0;
}

bool taskqueue_is_full(miniomp_taskqueue_t *taskqueue)
{
	return taskqueue->count == taskqueue->max_elements;
}

bool taskqueue_enqueue(miniomp_taskqueue_t *taskqueue, miniomp_task_t *task)
{
	pthread_mutex_lock(&taskqueue->mutex);

	if (taskqueue_is_full(taskqueue)) {
		pthread_mutex_unlock(&taskqueue->mutex);
		return false;
	}

	taskqueue->queue[taskqueue->tail] = task;
	taskqueue->count++;
	taskqueue->tail = (taskqueue->tail + 1) % taskqueue->max_elements;

	pthread_mutex_unlock(&taskqueue->mutex);

	return true;
}

miniomp_task_t *taskqueue_dequeue(miniomp_taskqueue_t *taskqueue)
{
	miniomp_task_t *task;

	pthread_mutex_lock(&taskqueue->mutex);

	if (taskqueue_is_empty(taskqueue)) {
		pthread_mutex_unlock(&taskqueue->mutex);
		return NULL;
	}

	task = taskqueue->queue[taskqueue->head];
	taskqueue->count--;
	taskqueue->head = (taskqueue->head + 1) % taskqueue->max_elements;

	pthread_mutex_unlock(&taskqueue->mutex);

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

	printf("GOMP_task called, size: %d, align: %d\n", arg_size, arg_align);

	task = malloc(sizeof(*task));
	if (!task) {
		printf("Error: can't allocate memory for the task\n");
		return;
	}

	if (posix_memalign(&task_data, max(arg_align, sizeof(void *)), arg_size)) {
		printf("Error: can't allocate memory for the task data\n");
		free(task);
		return;
	}

	if (__builtin_expect(cpyfn != NULL, 0))
		cpyfn(task_data, data);
	else
		memcpy(task_data, data, arg_size);

	task->fn = fn;
	task->data = task_data;

	taskqueue_enqueue(miniomp_taskqueue, task);
}
