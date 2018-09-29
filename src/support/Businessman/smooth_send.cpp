/***************************************************
  *Businessman Data Exchange System Smooth Layer Implement
  *Author :zhengchao@iie.ac.cn
  *This program cache and smooth the user app's send data;
  *Else it will cache for a while to wait more data pieces.
****************************************************/
#include <math.h>//ceil
#include "businessman_error.h"
#include "businessman_limit.h"
#include "businessman_time.h"
#include "businessman_memory.h"
#include "businessman_packet.h"
#include "bizman.h"
#include "smooth_send.h"

const time_tick_t TIMEOUT_SMOOTH_STREAM_TICK=TIMEOUT_SMOOTH_STREAM_USEC/TIME_TICK_STEP_USEC;

const unsigned int SMOOTH_HASH_SIZE=1024*8;

void free_stream_data(void*p)
{
	smooth_stream_t *p_stream=(smooth_stream_t *)p;
	free(p_stream->sub_stream_id);
	free(p_stream->dest_ip);
	free(p_stream->dest_port);
	p_stream->smooth_block=NULL;//block is in timeout queue,need not to free 
	free(p);
}
void make_key( unsigned char  *key,uint64 app_id,char is_reliable)
{
	
	memcpy(key,(char*)(&app_id),sizeof(app_id));
	memcpy(key+sizeof(app_id),&is_reliable,sizeof(is_reliable));
}
int smooth_hash_expire(void*p)
{
	smooth_stream_t *p_stream=(smooth_stream_t *)p;
	if(get_time_tick()-p_stream->last_edit_tick >TIMEOUT_SMOOTH_STREAM_TICK && p_stream->active_block_num==0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
int create_smooth_hash(hash_tab_t *stream_table)
{
	return _bizman_kernel_hash_create(stream_table, SMOOTH_HASH_SIZE ,NULL,NULL, smooth_hash_expire, free_stream_data);
}
int destroy_smooth_hash(hash_tab_t *stream_table)
{
	return _bizman_kernel_hash_destroy(stream_table, NULL);
}
smooth_stream_t* find_smooth_hash(hash_tab_t*stream_table,uint64 app_id,char is_reliable)
{
	unsigned char key[9];//8+1
	make_key(key, app_id, is_reliable);
	return (smooth_stream_t*)_bizman_kernel_hash_sel(stream_table, key,sizeof(key));
}
int add_smooth_hash(hash_tab_t*stream_table,uint64 app_id,char is_reliable,smooth_stream_t* data)
{
	unsigned char key[9];//8+1
	make_key(key, app_id, is_reliable);
	return _bizman_kernel_hash_add(stream_table, key, sizeof(key), (void *)data);
}
int delete_smooth_hash(hash_tab_t*stream_table,uint64 app_id,char is_reliable)
{
	unsigned char key[9];//8+1
	make_key(key, app_id, is_reliable);
	return _bizman_kernel_hash_del(stream_table, key, sizeof(key),NULL);
}
int expire_smooth_hash(hash_tab_t*stream_table)
{
	return _bizman_kernel_hash_expire(stream_table);
}
smooth_space_t* create_smooth_send_space(){
	smooth_space_t* smooth_space=NULL;
	smooth_space=(smooth_space_t*)b_malloc(sizeof(smooth_space_t));
	if(NULL==smooth_space)
	{
		return NULL;
	}
	memset(smooth_space,0,sizeof(smooth_space_t));
	create_smooth_hash(&(smooth_space->stream_hash));
	bizman_init_lock(&(smooth_space->thread_lock));
	smooth_space->last_expire_time=get_time_tick();
	smooth_space->smooth_timeout_tick=DEFAULT_TIMEOUT_SMOOTH_BLOCK_USEC/TIME_TICK_STEP_USEC;
	smooth_space->stat_dropped_pkts_num=0;
	smooth_space->cache_smooth_pkts_limit=0;
	return smooth_space;
}
void destroy_smooth_send_space(smooth_space_t* thread_smooth_space)
{
	list_index_t *pl_index=NULL;
	smooth_block_t* block=NULL;
	//free smooth ready queue
	pl_index=q_get_head(&(thread_smooth_space->smooth_ready_queue));
	while(pl_index!=NULL)
	{
		block=(smooth_block_t*)(pl_index->quiddity);
		free(block->buff);
		free(block);
		pl_index=q_get_head(&(thread_smooth_space->smooth_ready_queue));
	}
	//free timeout 
	pl_index=q_get_head(&(thread_smooth_space->smooth_timeout_queue));
	while(pl_index!=NULL)
	{
		block=(smooth_block_t*)(pl_index->quiddity);
		free(block->buff);
		free(block);
		pl_index=q_get_head(&(thread_smooth_space->smooth_timeout_queue));
	}
	//free smooth hash;
	_bizman_kernel_hash_destroy(&(thread_smooth_space->stream_hash), NULL);
	free(thread_smooth_space);
	
}
smooth_block_t* create_smooth_block(const smooth_stream_t* smooth_stream){
	smooth_block_t* new_block=NULL;
	bizman_packet_t* p_pkt=NULL;
	new_block=(smooth_block_t *)b_malloc(sizeof(smooth_block_t));
	new_block->buff=(char*)b_malloc(MTU_SIZE);
	new_block->buff_size=MTU_SIZE;
	new_block->used_size=LOAD_HEADER_LEN;
	new_block->hook.quiddity=(void *)new_block;
	new_block->first_filled_tick=get_time_tick();

	new_block->father_stream=(void*)smooth_stream;
	p_pkt=(bizman_packet_t *)new_block->buff;
	init_packet(p_pkt,smooth_stream->is_reliable,smooth_stream->stream_id,LOAD_HEADER_LEN);
#ifdef DEPLOY_YSP_SYSTEM
	memcpy(&(p_pkt->app_id),&(smooth_stream->app_id),sizeof(p_pkt->app_id));
#endif
	if(smooth_stream->is_reliable==TRUE)
	{
		new_block->is_reliable=TRUE;
	}

	return new_block;
}

smooth_stream_t* create_smooth_stream(const external_send_task_t *send_task){
	unsigned int i=0;
	smooth_stream_t* new_stream=NULL;
	new_stream=(smooth_stream_t*)b_malloc(sizeof(smooth_stream_t));
	new_stream->stream_id=generate_stream_id();
	new_stream->sub_stream_id=(unsigned int*)b_malloc(send_task->dest_num*sizeof(new_stream->stream_id));
	new_stream->sub_stream_id[0]=new_stream->stream_id;
	for(i=1;i<send_task->dest_num;i++)
	{
		new_stream->sub_stream_id[i]=generate_stream_id();
	}
	new_stream->app_id=send_task->app_id;
	new_stream->dest_num=send_task->dest_num;
	new_stream->dest_ip=(unsigned int*)b_malloc(send_task->dest_num*sizeof(*(new_stream->dest_ip)));
	memcpy(new_stream->dest_ip,send_task->dest_ip,send_task->dest_num*sizeof(*(new_stream->dest_ip)));
	new_stream->dest_port=(unsigned short*)b_malloc(send_task->dest_num*sizeof(*(new_stream->dest_port)));
	memcpy(new_stream->dest_port,send_task->dest_port,send_task->dest_num*sizeof(*(new_stream->dest_port)));
	if(send_task->flags&FLAG_NEED_ACK)
	{
		new_stream->is_reliable=TRUE;
	}
	new_stream->active_block_num=0;
	return new_stream;
}

void make_stream_block_ready(smooth_space_t* smooth_space,smooth_stream_t * target_stream)
{
	smooth_block_t*target_block=target_stream->smooth_block;
	
	if(0<q_is_item_in_list_quick(&(smooth_space->smooth_timeout_queue), &(target_block->hook)))//only block in timeout queue could put in to
	{
		q_leave_list(&(smooth_space->smooth_timeout_queue), &(target_block->hook));
		q_join_tail(&(smooth_space->smooth_ready_queue), &(target_block->hook));
		target_stream->smooth_block=NULL;
	}
	else
	{
		;//error occured
	}
}
void make_stream_block_timeout(smooth_space_t* smooth_space,smooth_stream_t* target_stream)
{
	smooth_block_t*target_block=target_stream->smooth_block;
	if(target_block->hook.nextele==NULL&&target_block->hook.preele==NULL)//only unindexed block could put in to
	{
		q_join_tail(&(smooth_space->smooth_timeout_queue), &(target_block->hook));
		target_stream->active_block_num++;
	}
	else
	{
		;//error occured
	}
}
smooth_block_t* get_smooth_timeout_block(smooth_space_t* smooth_space)
{
	queue_head_t *time_out_queue=&(smooth_space->smooth_timeout_queue);
	list_index_t* tmp_list_index=NULL;
	smooth_block_t* target_block=NULL;
	smooth_stream_t* father_stream=NULL;
	smooth_block_t* timeout_wait_block=NULL;
	tmp_list_index=q_read_head(time_out_queue);
	if(NULL==tmp_list_index)
	{
		return NULL;
	}
	timeout_wait_block=(smooth_block_t*)(tmp_list_index->quiddity);
	if(get_time_tick()-timeout_wait_block->first_filled_tick>smooth_space->smooth_timeout_tick)
	{
		tmp_list_index=q_get_head(&(smooth_space->smooth_timeout_queue));
		if(tmp_list_index!=NULL)
		{
			target_block=(smooth_block_t*)(tmp_list_index->quiddity);
			father_stream=(smooth_stream_t*)(target_block->father_stream);
			father_stream->smooth_block=NULL;
			return target_block;
		}
	}
	
	return NULL;
	
}
smooth_block_t* get_smooth_ready_block(smooth_space_t* smooth_space)
{
	list_index_t* target_list_index=NULL;
	smooth_block_t* target_block=NULL;

	
	target_list_index=q_get_head(&(smooth_space->smooth_ready_queue));
	if(target_list_index!=NULL)
	{
		target_block=(smooth_block_t*)(target_list_index->quiddity);
		return target_block;
	}
	else
	{
		return NULL;
	}
}
/**/
void remove_stream(smooth_space_t* smooth_space,smooth_stream_t* target_stream)
{
	if(target_stream->smooth_block!=NULL)
	{
		make_stream_block_ready(smooth_space, target_stream);
	}
	delete_smooth_hash(&(smooth_space->stream_hash), target_stream->app_id, target_stream->is_reliable);
}
/*return len write*/
int add_slice_to_block(smooth_block_t *block,const char* data,int len,unsigned int offset){
	
	
	int empty_space_len=block->buff_size-block->used_size;
	int write_len=MIN((int)(len+SLICE_HEADER_LEN),empty_space_len)-SLICE_HEADER_LEN;
	bizman_packet_t* p_pkt=(bizman_packet_t*)(block->buff);
	bizman_slice_t*p_slice=(bizman_slice_t*)(block->buff+block->used_size);
	char* p_slice_data=(char*)p_slice+SLICE_HEADER_LEN;
	if(block->buff_size-block->used_size <= SLICE_HEADER_LEN){
		return 0;
	}
	if(p_pkt->slice_num==MTU_SIZE/(SLICE_HEADER_LEN+sizeof(uint32)*2))//avoid 8 bytes ack fragment;
	{
		return 0;
	}	
	memcpy(p_slice_data,data,write_len);
	
	p_slice->slice_len=write_len+SLICE_HEADER_LEN;
//	p_slice->offset=offset;
	if(write_len==len)
	{
		p_slice->flags=SLICE_LAST_FRAG;
	}
	p_pkt->slice_num++;
	p_pkt->len+=p_slice->slice_len;
	
	block->used_size+=p_slice->slice_len;
	return write_len;
}

int smooth_block(smooth_space_t * thread_smooth_space,const external_send_task_t *send_task)
{
	smooth_stream_t* this_stream=NULL;
	int len=send_task->len;
	int write_len_ret=0,already_written_len=0;
	const char* data=send_task->data;
	char is_reliable=FALSE;
	if(send_task->flags&FLAG_NEED_ACK)
	{
		is_reliable=TRUE;
	}
	bizman_lock(&(thread_smooth_space->thread_lock));
	if(thread_smooth_space->cache_smooth_pkts_limit>0 &&
		thread_smooth_space->smooth_ready_queue.listcount+thread_smooth_space->smooth_timeout_queue.listcount >thread_smooth_space->cache_smooth_pkts_limit)
	{
		thread_smooth_space->stat_dropped_pkts_num++;
		bizman_unlock(&(thread_smooth_space->thread_lock));	
		return -1;
	}
	if(get_time_tick()-thread_smooth_space->last_expire_time>TIMEOUT_SMOOTH_STREAM_TICK)
	{
		expire_smooth_hash(&(thread_smooth_space->stream_hash));
		thread_smooth_space->last_expire_time=get_time_tick();
	}
	this_stream=find_smooth_hash(&(thread_smooth_space->stream_hash),send_task->app_id,is_reliable);
	
	//new stream id to current thread
	if(NULL==this_stream)
	{
		this_stream=create_smooth_stream(send_task);
		add_smooth_hash(&(thread_smooth_space->stream_hash), send_task->app_id,is_reliable ,this_stream);
	}
	
	this_stream->last_edit_tick=get_time_tick();
	if(this_stream->smooth_block==NULL)
	{
		this_stream->smooth_block=create_smooth_block(this_stream);
		make_stream_block_timeout(thread_smooth_space, this_stream);
	}
	
	while(already_written_len<len)
	{
		write_len_ret=add_slice_to_block(this_stream->smooth_block, data+already_written_len,len-already_written_len, this_stream->already_smoothed_bytes);
		if(write_len_ret>0)
		{
			already_written_len+=write_len_ret;
			this_stream->already_smoothed_bytes+=write_len_ret;
		}
		else //block is full
		{
			make_stream_block_ready(thread_smooth_space,this_stream);
			this_stream->smooth_block=create_smooth_block(this_stream);
			make_stream_block_timeout(thread_smooth_space, this_stream);
		}
	}
	if(send_task->flags&FLAG_LAST_PKT)
	{
		set_packet_flag_last((bizman_packet_t*)(this_stream->smooth_block->buff));
		make_stream_block_ready(thread_smooth_space,this_stream);
		remove_stream(thread_smooth_space, this_stream);
	}
	else if(send_task->flags&BIZMAN_PUSH_SEND)
	{
		make_stream_block_ready(thread_smooth_space,this_stream);
	}
	bizman_unlock(&(thread_smooth_space->thread_lock));
	return 0;
}
int get_smoothed_task(smooth_space_t * thread_smooth_space,internal_send_task_t * task)
{
	smooth_block_t* target=NULL;
	smooth_stream_t* father_stream=NULL;
	if(NULL==task)
	{
		return 0;
	}
	bizman_lock(&(thread_smooth_space->thread_lock));
	
	target=get_smooth_ready_block(thread_smooth_space);
	if(target==NULL)
	{
		target=get_smooth_timeout_block(thread_smooth_space);
	}
	
	bizman_unlock(&(thread_smooth_space->thread_lock));
	
	if(target==NULL)
	{
		return 0;
	}
	else
	{
		bizman_lock(&(thread_smooth_space->thread_lock));
		father_stream=(smooth_stream_t*)target->father_stream;
		task->stream_id=father_stream->stream_id;
		
		
		task->dest_num=father_stream->dest_num;
		task->dest_ip=(unsigned int*)b_malloc(father_stream->dest_num*sizeof(*(task->dest_ip)));
		memcpy(task->dest_ip,father_stream->dest_ip,father_stream->dest_num*sizeof(*(task->dest_ip)));
		task->dest_port=(unsigned short*)b_malloc(father_stream->dest_num*sizeof(*(father_stream->dest_port)));
		memcpy(task->dest_port,father_stream->dest_port,father_stream->dest_num*sizeof(*(task->dest_port)));
		task->sub_stream_id=(unsigned int*)b_malloc(father_stream->dest_num*sizeof(*(father_stream->sub_stream_id)));
		memcpy(task->sub_stream_id,father_stream->sub_stream_id,father_stream->dest_num*sizeof(*(father_stream->sub_stream_id)));
		
		task->data=create_send_data_refer(target->buff, target->used_size, task->dest_num);
		task->is_reliable=target->is_reliable;
		
		father_stream->active_block_num--;
		bizman_unlock(&(thread_smooth_space->thread_lock));
		
		free(target);
		return 1;
	}	
}

