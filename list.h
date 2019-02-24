#ifndef _LIST_H
#define _LIST_H

#include <stdbool.h>

struct list_entry {
	struct list_entry *next;
	struct list_entry *prev;
};

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

static inline bool list_is_empty(struct list_entry *head)
{
	return head->next == head;
}

#endif /* _LIST_H */
