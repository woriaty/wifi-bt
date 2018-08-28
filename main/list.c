#include "list.h"

struct list_head *find_list_tail(struct list_head *head)
{
	while(head->next != NULL){
		head = head->next;
	}
	return head;
}

void list_add_tail(struct list_head *head, struct list_head *new)
{
	struct list_head *tail = find_list_tail(head);
	tail->next = new;
	new->prev = tail;
	new->next = NULL;
}

void list_init(struct list_head *head)
{
	head->prev = NULL;
	head->next = NULL;
}

static void list_del_entry(struct list_head *prev,struct list_head *next)
{
	if(prev != NULL)
		prev->next = next;
	if(next != NULL)
		next->prev = prev;
}

/*
 *	remove entry node from list
 */
void list_remove(struct list_head *entry)
{
	list_del_entry(entry->prev,entry->next);
}

/*
 * 	insert new list node between prev node and next node
 */
void list_insert(struct list_head *prev,struct list_head *next,struct list_head *new)
{
	if(prev != NULL)
		prev->next = new;
	if(next != NULL)
		next->prev = new;
	new->prev = prev;
	new->next = next;
}
