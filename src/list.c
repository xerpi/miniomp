#include <stdio.h>
#include "list.h"

void miniomp_list_init(miniomp_list_t *list)
{
	list->next = list;
	list->prev = list;
}

bool miniomp_list_is_empty(const miniomp_list_t *list)
{
	return list->next == list;
}

void miniomp_list_insert(miniomp_list_t *list, miniomp_list_t *elem)
{
	elem->next = list->next;
	elem->prev = list;
	list->next = elem;
	elem->next->prev = elem;
}

void miniomp_list_remove(miniomp_list_t *elem)
{
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	elem->next = NULL;
	elem->prev = NULL;
}
