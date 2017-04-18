#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

typedef struct list_t list_t;

list_t *list_create(void);
void list_destroy(list_t *list);
bool list_is_empty(const list_t *list);
bool list_is_full(const list_t *list);
bool list_enqueue(list_t *list, void *element);
void *list_dequeue(list_t *list);

#endif
