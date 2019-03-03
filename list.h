#ifndef _LIST_H
#define _LIST_H

#include <stdbool.h>
#include <stdint.h>

struct list_entry {
	struct list_entry *next;
	struct list_entry *prev;
};

#define list_container_of(ptr, type, member) ((type *)((uintptr_t)(ptr) - offsetof(type, member)))

#define list_entry(ptr, type, member) list_container_of(ptr, type, member)
#define list_fist_entry(head, type, member) list_entry((head)->next, type, member)

static inline bool list_is_empty(const struct list_entry *head)
{
	return head->next == head;
}

static inline void list_head_init(struct list_entry *head)
{
	head->next = head;
	head->prev = head;
}

static inline void list_add_head(struct list_entry *head, struct list_entry *entry)
{
	entry->next = head->next;
	entry->prev = head;
	head->next->prev = entry;
	head->next = entry;
}

static inline void list_add(struct list_entry *head, struct list_entry *entry)
{
	list_add_head(head, entry);
}

static inline void list_del(struct list_entry *entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

static inline struct list_entry *list_pop_head(struct list_entry *head)
{
	struct list_entry *entry;

	if (list_is_empty(head))
		return NULL;

	entry = head->next;
	list_del(entry);

	return entry;
}

static inline struct list_entry *list_pop(struct list_entry *head)
{
	return list_pop_head(head);
}

#endif /* _LIST_H */
