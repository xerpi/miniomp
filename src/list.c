#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "list.h"

#define LIST_DEFAULT_SIZE 16

typedef struct list_t {
	uint32_t size;
	uint32_t count;
	uint32_t head;
	uint32_t tail;
	void **elements;
} list_t;

list_t *list_create(void)
{
	list_t *list;

	list = malloc(sizeof(*list));
	if (!list)
		return NULL;

	list->elements = malloc(LIST_DEFAULT_SIZE * sizeof(*list->elements));
	if (!list->elements)
		goto err_free;

	list->size = LIST_DEFAULT_SIZE;
	list->count = 0;
	list->head = 0;
	list->tail = 0;

	return list;

err_free:
	free(list);
	return NULL;
}

void list_destroy(list_t *list)
{
	free(list->elements);
	free(list);
}

bool list_is_empty(const list_t *list)
{
	return list->count == 0;
}

bool list_is_full(const list_t *list)
{
	return list->count == list->size;
}

bool list_enqueue(list_t *list, void *element)
{
	if (list_is_full(list)) {
		void *new_elements = realloc(list->elements,
					     2 * list->size * sizeof(*list->elements));
		if (!new_elements)
			return false;

		list->elements = new_elements;
		list->size *= 2;
	}

	list->elements[list->tail] = element;
	list->count++;
	list->tail = (list->tail + 1) % list->size;

	return true;
}

void *list_dequeue(list_t *list)
{
	void *task;

	if (list_is_empty(list))
		return NULL;

	task = list->elements[list->head];
	list->count--;
	list->head = (list->head + 1) % list->size;

	return task;
}
