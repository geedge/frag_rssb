
/*list.cpp
 * extract from linux kernel
 * zhengchao add list integrity check function
 *   for list control
 */

#include "list.h"
list_index_t *q_read_head(queue_head_t *qhead_obj)
{
	queue_head_t	*qhead=qhead_obj;
	list_index_t	*p=NULL;
	
	if(NULL == qhead->head) 
		return p;
	p = qhead->head;
	return p;
}

// return a ~NULL pointer of list_item_t for success
list_index_t *q_get_head(queue_head_t *qhead_obj)
{
	queue_head_t	*qhead=qhead_obj;
	list_index_t	*p=NULL;
	
	if(NULL == qhead->head) 
		return p;
	p = qhead->head;
	if(qhead->head == qhead->tail) {
		qhead->head = qhead->tail = NULL;
		qhead->listcount = 0;
	}
	else
	{
		qhead->head = p->nextele; 
		p->nextele->preele = NULL;
		qhead->listcount--;
	}
	p->nextele = NULL;
	p->preele=NULL;
#ifdef BIZMAN_DEBUG_LIST
	int check_result=q_list_check(qhead_obj,NULL);
	if(check_result<0)
	{
		fprintf(stderr,"Queue Check Error:%d.\n",check_result);
	}
#endif
	return p;
}

// return a ~NULL pointer of list_item_t for success
list_index_t *q_get_tail(queue_head_t *qhead_obj)
{
	queue_head_t	*qhead=qhead_obj;
	list_index_t	*p=NULL;
	
	if(NULL == qhead->tail) 
		return p;
	p = qhead->tail;
	if(qhead->head == qhead->tail) {
		qhead->head = qhead->tail = NULL;
		qhead->listcount = 0;
		return p;
	}
	qhead->tail = p->preele; 
	p->preele->nextele = NULL;
	p->preele = NULL;
	
	qhead->listcount--;
	return p;
}

// always success
list_index_t *q_join_tail(queue_head_t *qhead_obj, list_index_t *lindex_obj)
{
	queue_head_t	*qhead=qhead_obj;
	list_index_t	*p=lindex_obj;

	if(NULL == p)
		return p;
	p->nextele = p->preele = NULL;
	if(NULL == qhead->tail) {
		qhead->head = qhead->tail = p;
		qhead_obj->listcount = 1;
		return p;
	}
	p->preele = qhead->tail;
	qhead->tail->nextele = p;
	qhead->tail = p;

	qhead->listcount++;
#ifdef BIZMAN_DEBUG_LIST
	int check_result=q_list_check(qhead_obj,NULL);
	if(check_result<0)
	{
		fprintf(stderr,"Queue Check Error:%d.\n",check_result);
	}
#endif
	return p;
}

// always success
list_index_t *q_join_head(queue_head_t *qhead_obj, list_index_t *lindex_obj)
{
	queue_head_t	*qhead=qhead_obj;
	list_index_t	*p=lindex_obj;

	if(NULL == p)
		return p;
	p->nextele = p->preele = NULL;
	if(NULL == qhead->head) {
		qhead->head = qhead->tail = p;
		qhead->listcount = 1;
		return p;
	}
	p->nextele = qhead->head;
	qhead->head->preele = p;
	qhead->head = p;
	
	qhead->listcount++;
	return p;
}

list_index_t *q_leave_list(queue_head_t *qhead_obj, list_index_t *lindex_obj)
{
#ifdef BIZMAN_DEBUG_LIST
	if(0>q_is_item_in_list_quick(qhead_obj,lindex_obj))
	{
		fprintf(stderr,"Obj not in Queue.\n");
	}
#endif

	queue_head_t 	*qhead=qhead_obj;
	list_index_t	*p=NULL;

	if(NULL == lindex_obj)
		return p;
	if(NULL == qhead->head)
		return p;
	
	p = lindex_obj;
	if(NULL == p->preele && NULL == p->nextele) {//this action is damn danger!!!
		qhead->tail = qhead->head = NULL;
		qhead->listcount = 0;
		return p;
	}
	if(NULL == p->preele) {
		qhead->head = p->nextele;
		p->nextele->preele = NULL;
		p->nextele = NULL;
		qhead->listcount--;
		return p;
	}
	
	if(NULL == p->nextele) {
		qhead->tail = p->preele;
		p->preele->nextele = NULL;
		p->preele = NULL;
		qhead->listcount--;
		return p;
	}
	
	p->nextele->preele = p->preele;
	p->preele->nextele = p->nextele;
	p->nextele = p->preele = NULL;
	qhead->listcount--;
#ifdef BIZMAN_DEBUG_LIST
	int check_result=q_list_check(qhead_obj,NULL);
	if(check_result<0)
	{
		fprintf(stderr,"Queue Check Error:%d.\n",check_result);
	}
#endif
	return p;

}

list_index_t *q_join_list_n(queue_head_t *qhead_obj, list_index_t *lobj_next, list_index_t *lindex_obj) 
{
	list_index_t	*p=lindex_obj, *next=lobj_next;
	queue_head_t	*pq=qhead_obj;

	if(NULL == p)
		return p;
	
	if(NULL == next) {
		q_join_tail(pq, p);
		return p;
	}
	else if(NULL == next->preele) {
		q_join_head(pq, p);
	}
	else {
		qhead_obj->listcount++;
		
		p->nextele = next;
		p->preele = next->preele;

		next->preele = p;
		p->preele->nextele = p;
	}

	return p;
}

list_index_t *q_join_list_p(queue_head_t *qhead_obj, list_index_t *lobj_pre, list_index_t *lindex_obj) 
{
	list_index_t	*p=lindex_obj, *pre=lobj_pre;
	queue_head_t	*pq=qhead_obj;

	if(NULL == p)
		return p;
	
	if(NULL == pre) {
		q_join_head(pq, p);
		return p;
	}
	else if(NULL == pre->nextele) {
		q_join_tail(pq, p);
	}
	else {
		qhead_obj->listcount++;
		
		p->preele = pre;
		p->nextele = pre->nextele;
		
		pre->nextele = p;
		p->nextele->preele = p;
	}

	return p;
}
/*Function:Check the intergrity of the list,to dectect memory mess.
*Input: Queue Head,data check call back function
*Output: return a number below zero,if the queue got a problem,else return 0;
*Author:zhengchao@iie.ac.cn at 20100913
*/
int q_list_check(const queue_head_t *qhead_obj,int(*quiddity_check)(const void *)){
//	list_index_t	*p=NULL, *pre=NULL;
//	int linked_ele_number=0;
	int ret=0;
//	return 0;
	if(qhead_obj->listcount==0)
	{
		if(qhead_obj->head!=NULL||qhead_obj->head!=NULL)
		{
			ret=-1;
		}
		goto exit;
	}
	
	if(qhead_obj->head==NULL||qhead_obj->tail==NULL)
	{
		ret=-2;
		goto exit;
	}
	if(qhead_obj->listcount>1){
		if(qhead_obj->head->preele!=NULL||qhead_obj->head->nextele==NULL)
		{
			ret=-3;
			goto exit;
		}
		if(qhead_obj->tail->preele==NULL||qhead_obj->tail->nextele!=NULL)
		{
			ret=-4;
			goto exit;
		}
	}
	/*
	pre=qhead_obj->head;
	p=pre->nextele;
	while(p!=NULL){
		linked_ele_number++;
		
		//Is the declared size equal to element linked number;
		if(linked_ele_number > qhead_obj->listcount)
		{
			ret=-5;
			goto exit;
		}
		
		//Is element's preele pointer equal to its preele
		if(pre!=p->preele)
		{
			ret=-6;
			goto exit;
		}
		if(quiddity_check!=NULL){
			if(0>quiddity_check(p->quiddity))
			{
				ret =-7;
				goto exit;
			}
		}
		pre=p;
		p=p->nextele;
	}
	//Is the last element equal to tail
	if(pre!=qhead_obj->tail)
	{
		ret=-8;
		goto exit;
	}
	if(linked_ele_number !=qhead_obj->listcount-1)
	{
		ret=-9;
		goto exit;
	}
	*/
exit:
	if(ret<0)
	{
		return ret;
	}
	else
	{
		return 1;
	}
	
}
/*Simple check,not raversal*/
int q_is_item_in_list(queue_head_t *qhead_obj, list_index_t *lindex_obj){
//	list_index_t* pre=lindex_obj->preele;
//	list_index_t*next=lindex_obj->nextele;
	list_index_t*p=NULL;
	if(qhead_obj->listcount==0)
		return -1;
	if(lindex_obj->nextele==NULL&&lindex_obj->preele==NULL&&qhead_obj->listcount>1)
	{
		return -1;
	}
	int i=0;
	p=qhead_obj->head;
	for(i=0;i<qhead_obj->listcount;i++)
	{
		if(p==lindex_obj)
		{
			return 1;
		}
		p=p->nextele;
	}
	
	return -1;
	
}
/*every list_index_t leaves list,its pre and next will be set NULL
 * In Configuration Transmiiter project, not null element will consider in list,
 * pre and next is NULL only if there's one element in list only*/
int q_is_item_in_list_quick(queue_head_t *qhead_obj, list_index_t *lindex_obj)
{
	//empty list
	if(qhead_obj->listcount==0)
		return -1;
	//one element
	if(qhead_obj->listcount==1)
	{
		if(qhead_obj->head!=lindex_obj)
			return -1;
	}
	//have one more element
	if(qhead_obj->listcount>1)
	{
		if(lindex_obj->nextele==NULL&&lindex_obj->preele==NULL)
			return -1;
	}

	return 1;

}
