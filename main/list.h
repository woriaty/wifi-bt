#ifndef __LIST_H__
#define __LIST_H__

#include <stdio.h>

#define container_of(ptr, type, member)		(type *)((char *)ptr-(unsigned long)&((type *)0)->member)

struct list_head{
	struct list_head *prev;
	struct list_head *next;
};

struct list_head *find_list_tail(struct list_head *head);
void list_add_tail(struct list_head *head, struct list_head *new);
void list_init(struct list_head *head);
void list_del(struct list_head *entry);
void list_insert(struct list_head *prev,struct list_head *next,struct list_head *new);

#define list_entry(ptr,type,member)	container_of(ptr,type,member)
/*
 *	add new list node befor entry node
 */
#define list_insert_befor(entry,new)	list_insert(entry->prev,entry,new)

#define INIT_LIST(name)	struct list_head *name; list_init(name);

#endif
