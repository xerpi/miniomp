#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

typedef struct miniomp_list_t {
	struct miniomp_list_t *next;
	struct miniomp_list_t *prev;
} miniomp_list_t;

void miniomp_list_init(miniomp_list_t *list);
bool miniomp_list_is_empty(const miniomp_list_t *list);
void miniomp_list_insert(miniomp_list_t *list, miniomp_list_t *elem);
void miniomp_list_remove(miniomp_list_t *elem);

#define container_of(ptr, sample, member) \
	(__typeof__(sample))((uintptr_t)(ptr) - \
		((uintptr_t)&(sample)->member - (uintptr_t)(sample)))

#define miniomp_list_front(list, sample, member) \
		container_of((list)->next, sample, member);

#define miniomp_list_pop_front(list, sample, member) \
	({ \
		miniomp_list_t *next = (miniomp_list)->next; \
		miniomp_list_remove(next); \
		container_of(next, sample, member); \
	})

#define miniomp_list_for_each(it, list, link) \
	for (it = container_of(it->link, it, link); \
	     &it->link != miniomp_list; \
	     it = container_of(it->link.next, it, link))
#endif
