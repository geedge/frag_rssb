#ifndef _LIST_H_INCLUDED
#define _LIST_H_INCLUDED
#include<stdio.h> //use NULL
typedef struct list_index {
	struct list_index	*nextele;
	struct list_index	*preele;
	void			*quiddity;
}list_index_t;

typedef struct queue_head {
	list_index_t		*head;
	list_index_t		*tail;
	int			listcount;
}queue_head_t;


list_index_t *q_get_head(queue_head_t *qhead_obj);
list_index_t *q_get_tail(queue_head_t *qhead_obj);
list_index_t *q_join_tail(queue_head_t *qhead_obj, list_index_t *lindex_obj);
list_index_t *q_join_head(queue_head_t *qhead_obj, list_index_t *lindex_obj);
list_index_t *q_leave_list(queue_head_t *qhead_obj, list_index_t *lindex_obj);
list_index_t *q_join_list_n(queue_head_t *qhead_obj, list_index_t *lobj_next, list_index_t *lindex_obj);
list_index_t *q_join_list_p(queue_head_t *qhead_obj, list_index_t *lobj_pre, list_index_t *lindex_obj);
list_index_t *q_read_head(queue_head_t *qhead_obj);
int q_list_check(const queue_head_t *qhead_obj,int(*quiddity_check)(const void *));
int q_is_item_in_list(queue_head_t *qhead_obj, list_index_t *lindex_obj);
int q_is_item_in_list_quick(queue_head_t *qhead_obj, list_index_t *lindex_obj);
#endif /* _LIST_H_INCLUDED */

