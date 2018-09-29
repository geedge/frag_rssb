#include <stdlib.h>
#include <string.h>//memcpy
#include <assert.h>
#include "list.h"
#include "hash.h"
#include "businessman_memory.h"
#include "businessman_error.h"
#include "businessman_limit.h"
#include "businessman_time.h"
#include "businessman_lock.h"
#include "bizman.h"
#include "reliable_recv.h"

const unsigned int RELIABLE_STREAM_HASH_SIZE=1024*8;

const time_tick_t TIMEOUT_RECV_STREAM_TICK=TIMEOUT_RECV_STREAM_USEC/TIME_TICK_STEP_USEC;
const time_tick_t DEFAULT_ACK_COMULATE_TICK=DEFAULT_ACK_COMULATE_USEC/TIME_TICK_STEP_USEC;

const int LATTICE_STATE_EMPTY=0;
const int LATTICE_STATE_OCCUPIED=1;




int count_last_frag_num_in_packet(const bizman_packet_t* packet)
{
	int i=0,last_frag_num=0;
	bizman_slice_t* slice=(bizman_slice_t*)((char*)packet+LOAD_HEADER_LEN);
	for(i=0;i<packet->slice_num;i++)
	{
		if(slice->flags&SLICE_LAST_FRAG)
		{
			last_frag_num++;
		}
		slice=(bizman_slice_t* )((char*)slice+slice->slice_len);

	}
	return last_frag_num;
}
recv_data_index_t* create_recv_data_index(bizman_packet_t* packet,int len,unsigned int src_ip,unsigned short src_port)
{
	recv_data_index_t* index=NULL;
	index=(recv_data_index_t*)b_malloc(sizeof(recv_data_index_t));
	index->hook.quiddity=(void*)index;
	index->packet=packet;
	index->len=len;
	index->src_ip=src_ip;
	index->src_port=src_port;
	return index;
}
void free_recv_data_index(recv_data_index_t* target)
{
	free(target->packet);
	free(target);
}
ack_task_t* create_ack_task(unsigned int dst_ip,unsigned short dst_port,uint32 stream_id,uint32 sequence){
	ack_task_t* ack_task=NULL;
	ack_task=(ack_task_t*)b_malloc(sizeof(ack_task_t));
	ack_task->hook.quiddity=(void*)ack_task;
	ack_task->dest_ip=dst_ip;
	ack_task->dest_port=dst_port;
	ack_task->sequence=sequence;
	ack_task->stream_id=stream_id;
	ack_task->flags|=FLAG_ACK_PKT;
	return ack_task;
}
void make_recv_key(unsigned char *key_buf,unsigned int src_ip,unsigned short src_port,uint32 stream_id)
{
	memcpy(key_buf,&src_ip,sizeof(src_ip));
	memcpy(key_buf+sizeof(src_ip),&src_port,sizeof(src_port));
	memcpy(key_buf+sizeof(src_ip)+sizeof(src_port),&stream_id,sizeof(stream_id));
}
recv_stream_record_t* find_stream_record_in_hash(hash_tab_t *hash,unsigned int src_ip,unsigned short src_port,uint32 stream_id)
{
	unsigned char key[sizeof(src_ip)+sizeof(src_port)+sizeof(stream_id)];
	make_recv_key(key,src_ip,src_port, stream_id);
	return (recv_stream_record_t*)_bizman_kernel_hash_sel(hash,key,sizeof(key));
}
int del_stream_record_in_hash(hash_tab_t *hash,unsigned int src_ip,unsigned short src_port,uint32 stream_id)
{
	unsigned char key[sizeof(src_ip)+sizeof(src_port)+sizeof(stream_id)];
	make_recv_key(key,src_ip,src_port, stream_id);
	return _bizman_kernel_hash_del(hash,key,sizeof(key), NULL);
}
void add_record_to_hash(hash_tab_t *hash,unsigned int src_ip,unsigned short src_port,uint32 stream_id,recv_stream_record_t* value)
{
	unsigned char key[sizeof(src_ip)+sizeof(src_port)+sizeof(stream_id)];
	make_recv_key(key,src_ip,src_port, stream_id);
	_bizman_kernel_hash_add(hash, key, sizeof(key), value);
}
int recv_stream_hash_expire(void *p)
{
	recv_stream_record_t* record=(recv_stream_record_t*)p;
	int have_unread_data=FALSE;
	if(0==bizman_trylock(&(record->lock_ready_operate))){
		if(record->is_in_ready_queue==TRUE)
		{
			have_unread_data=TRUE;
		}
		bizman_unlock(&(record->lock_ready_operate));
	}
	else
	{
		return 0;
	}
	if(have_unread_data==TRUE)
	{
		return 0;
	}
	if(get_time_tick()-record->last_edit_tick >TIMEOUT_RECV_STREAM_TICK||record->is_read_complete==TRUE)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
void free_recv_stream_value(void *p)
{
	recv_stream_record_t* record=(recv_stream_record_t*)p;
	
	unsigned int i=0;
	for(i=0;i<record->recv_window_size;i++)
	{
		if(LATTICE_STATE_OCCUPIED==record->ruler[i].state)
		{
			free(record->ruler[i].data_index->packet);
			free(record->ruler[i].data_index);
		}
	}
	free(record->ruler);
	//record already leave ack_timeout_queue and ready queue;
	//and core_recv_thread had stopped;
	list_index_t* p_list_index=NULL;
	recv_data_index_t* packet_index=NULL;
	
	p_list_index=q_get_head(&(record->orderd_data_queue));
	while(NULL!=p_list_index)
	{
		packet_index=(recv_data_index_t*)(p_list_index->quiddity);
		free(packet_index->packet);
		free(packet_index);
		packet_index=NULL;
		p_list_index=q_get_head(&(record->orderd_data_queue));
	}

	free(record);
}
void create_recv_stream_hash(hash_tab_t *hash)
{
	_bizman_kernel_hash_create(hash, RELIABLE_STREAM_HASH_SIZE, NULL, NULL, recv_stream_hash_expire, free_recv_stream_value);
}
void destroy_recv_stream_hash(hash_tab_t *hash)
{
	_bizman_kernel_hash_destroy(hash, NULL);
}
void expire_recv_stream_hash(hash_tab_t*hash)
{
	_bizman_kernel_hash_expire(hash);
}
recv_stream_record_t* create_recv_record(unsigned int stream_id,unsigned int window_size)
{
	recv_stream_record_t* record=NULL;
	record=(recv_stream_record_t*)b_malloc(sizeof(recv_stream_record_t));
	record->last_edit_tick=get_time_tick();
	record->record_create_tick=record->last_edit_tick;
	record->ready_hook.quiddity=(void*)record;
	record->ack_timeout_hook.quiddity=(void*)record;
	record->stream_id=stream_id;
	record->recv_window_size=window_size;
	record->ruler=(stream_recv_lattice_t*)b_malloc(sizeof(stream_recv_lattice_t)*window_size);
	bizman_init_lock(&(record->lock_ready_operate));
	return record;
}

reliable_recv_space_t* create_reliable_recv_space(void)
{
	reliable_recv_space_t* reliable_recv_space=NULL;
	reliable_recv_space=(reliable_recv_space_t*)b_malloc(sizeof(reliable_recv_space_t));
	create_recv_stream_hash(&(reliable_recv_space->stream_hash));
	bizman_init_lock(&(reliable_recv_space->mutexlock_ready_stream_index_queue));
	bizman_init_cond(&(reliable_recv_space->cond_have_data_to_read));
	reliable_recv_space->last_expire_time=get_time_tick();
	reliable_recv_space->accumulate_ack_num=DEFAULT_COMULATE_ACK_NUM;
	reliable_recv_space->accumulate_ack_timeout_tick=DEFAULT_ACK_COMULATE_TICK;
	reliable_recv_space->receive_window_size=DEFALUT_RECV_WINDOW_SIZE;
	return reliable_recv_space;
}
void isolate_recv_record(reliable_recv_space_t* reliable_recv_space,recv_stream_record_t* stream_record)
{
	
	bizman_lock(&(reliable_recv_space->mutexlock_ready_stream_index_queue));
	bizman_lock(&(stream_record->lock_ready_operate));
			
	if(TRUE==stream_record->is_in_ready_queue)
	{
		q_leave_list(&(reliable_recv_space->ready_stream_index_queue), &(stream_record->ready_hook));
		stream_record->is_in_ready_queue=FALSE;
	}
	if(TRUE==stream_record->is_in_ack_timeout_queue)
	{
		q_leave_list(&(reliable_recv_space->ack_timeout_queue),&(stream_record->ack_timeout_hook));
		stream_record->is_in_ack_timeout_queue=FALSE;
	}
	bizman_unlock(&(stream_record->lock_ready_operate));
	bizman_unlock(&(reliable_recv_space->mutexlock_ready_stream_index_queue));
	
}
void destroy_reliable_recv_space(reliable_recv_space_t* reliable_recv_space)
{
	list_index_t* p_index_ready=NULL;
	list_index_t* pl_timeout_ack_record=NULL,*pl_to_ack=NULL;
	ack_task_t* ack_task=NULL;
	recv_stream_record_t* record=NULL;
	//leave ready queue
	p_index_ready=q_get_head(&(reliable_recv_space->ready_stream_index_queue));
	while(p_index_ready!=NULL)
	{
		record=(recv_stream_record_t*)(p_index_ready->quiddity);
		bizman_lock(&(record->lock_ready_operate));
		record->is_in_ready_queue=FALSE;
		bizman_unlock(&(record->lock_ready_operate));
		p_index_ready=q_get_head(&(reliable_recv_space->ready_stream_index_queue));
	}
	// free timeout ack
	pl_timeout_ack_record=q_get_head(&(reliable_recv_space->ack_timeout_queue));
	while(pl_timeout_ack_record!=NULL)
	{
		record=(recv_stream_record_t*)(pl_timeout_ack_record->quiddity);
		record->is_in_ack_timeout_queue=FALSE;
		free(record->timeout_ack);
		record->timeout_ack=NULL;
		pl_timeout_ack_record=q_get_head(&(reliable_recv_space->ack_timeout_queue));
	}
	//free ack_task
	pl_to_ack=q_get_head(&(reliable_recv_space->to_ack_queue));
	while(pl_to_ack!=NULL)
	{
		ack_task=(ack_task_t*)(pl_to_ack->quiddity);
		free(ack_task);
		pl_to_ack=q_get_head(&(reliable_recv_space->to_ack_queue));
	}
	destroy_recv_stream_hash(&(reliable_recv_space->stream_hash));
	free(reliable_recv_space);
	
}
int add_raw_packet(reliable_recv_space_t* reliable_recv_handle,unsigned int src_ip,unsigned short src_port,bizman_packet_t* packet_data,int packet_len)
{

	recv_stream_record_t* stream_record=NULL;
	recv_data_index_t* packet_index=NULL;
	int record_index=packet_data->sequence%reliable_recv_handle->receive_window_size;
	ack_task_t *ack_task=NULL;
	unsigned int i=0;
	int last_frag_num;
	int flag_stream_complete=FALSE;
#ifdef BIZMAN_DEBUG_DETAILS
	printf("Recv DAT:stream=%d seq=%d\n",packet_data->id,packet_data->sequence);
#endif
	//locate stream record in hash

	if(packet_data->flags&FLAG_EST_STREAM&&packet_data->sequence==START_SEQ_NUM)
	{
		stream_record=find_stream_record_in_hash(&(reliable_recv_handle->stream_hash),src_ip,src_port,packet_data->id);
		if(stream_record!=NULL)
		{
			if(get_time_tick()-stream_record->record_create_tick>FORCE_EST_INTERVAL/TIME_TICK_STEP_USEC)
			{
				isolate_recv_record(reliable_recv_handle,stream_record);
				if(0!=del_stream_record_in_hash(&(reliable_recv_handle->stream_hash),src_ip,src_port,packet_data->id))
				{
#ifdef BIZMAN_DEBUG_DETAILS
					printf("Del:stream=%d seq=%d failed\n",packet_data->id,packet_data->sequence);
#endif
				}
				stream_record=NULL;
			}
		}
		
	}


	stream_record=find_stream_record_in_hash(&(reliable_recv_handle->stream_hash),src_ip,src_port,packet_data->id);
	if(stream_record==NULL)
	{
		stream_record=create_recv_record(packet_data->id,reliable_recv_handle->receive_window_size);
		add_record_to_hash(&(reliable_recv_handle->stream_hash), src_ip,src_port,packet_data->id, stream_record);
	}
	if(packet_data->flags&FLAG_NEED_ACK)
	{
		if(packet_data->sequence==0&&stream_record->max_ordered_seq==0)//0 is not a valid sequence at begining
		{

			free(packet_data);//error
			return -1;
		}
		if((packet_data->sequence > stream_record->max_ordered_seq||packet_data->sequence==0) && //when uint32 seq overflow,its turn to 0
			packet_data->sequence <= stream_record->latest_ack_seq+reliable_recv_handle->receive_window_size&&
			stream_record->ruler[record_index].state==LATTICE_STATE_EMPTY)
		{

			packet_index=create_recv_data_index(packet_data, packet_len, src_ip, src_port);
			stream_record->ruler[record_index].state=LATTICE_STATE_OCCUPIED;
			stream_record->ruler[record_index].sequence=packet_data->sequence;
			stream_record->ruler[record_index].data_index=packet_index;

			record_index=(stream_record->max_ordered_seq+1)%reliable_recv_handle->receive_window_size;
			for(i=0;i<reliable_recv_handle->receive_window_size;i++)
			{
				if(stream_record->ruler[record_index].state==LATTICE_STATE_OCCUPIED)
				{
					if(stream_record->ruler[record_index].data_index->packet->flags&FLAG_LAST_PKT)
					{
						flag_stream_complete=TRUE;				
					}
					last_frag_num=count_last_frag_num_in_packet(stream_record->ruler[record_index].data_index->packet);
					bizman_lock(&(stream_record->lock_ready_operate));
					stream_record->chunk_num+=last_frag_num;
					q_join_tail(&(stream_record->orderd_data_queue),&(stream_record->ruler[record_index].data_index->hook));
					stream_record->ruler[record_index].data_index=NULL;
					stream_record->ruler[record_index].state=LATTICE_STATE_EMPTY;
					stream_record->ordered_unack_num++;
					stream_record->max_ordered_seq++;
					stream_record->is_recv_complete=flag_stream_complete;
					//no chunk_mode  or chunk_mode and have last frag
					if(FALSE==stream_record->is_in_ready_queue
						&&(reliable_recv_handle->chunk_mode!=TRUE||last_frag_num>0))
					{
						
						stream_record->is_in_ready_queue=TRUE;
						bizman_unlock(&(stream_record->lock_ready_operate));
						
						bizman_lock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
						q_join_tail(&(reliable_recv_handle->ready_stream_index_queue), &(stream_record->ready_hook));
						
						if(reliable_recv_handle->choke_mode==TRUE)
						{
							bizman_cond_signal(&(reliable_recv_handle->cond_have_data_to_read));
						}
						bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
						
					}
					else	
					{					
						bizman_unlock(&(stream_record->lock_ready_operate));
					}

				}
				else
				{
					break;///uncontinous window
				}
				record_index=(record_index+1)%reliable_recv_handle->receive_window_size;
			}
		//	if((packet_data->flags&FLAG_LAST_PKT&&stream_record->ordered_unack_num>0)||
				if(stream_record->ordered_unack_num>=reliable_recv_handle->accumulate_ack_num)//ack immediately
			{
				ack_task=create_ack_task(src_ip, src_port, stream_record->stream_id, stream_record->max_ordered_seq);
                ack_task->father_stream=stream_record;
				q_join_tail(&(reliable_recv_handle->to_ack_queue), &(ack_task->hook));
				stream_record->ordered_unack_num=0;
				stream_record->latest_ack_seq=stream_record->max_ordered_seq;
				if(stream_record->is_in_ack_timeout_queue)
				{
					q_leave_list(&(reliable_recv_handle->ack_timeout_queue),&(stream_record->ack_timeout_hook));
					free(stream_record->timeout_ack);
					stream_record->timeout_ack=NULL;
					stream_record->is_in_ack_timeout_queue=FALSE;
				}
			}
			else if(stream_record->ordered_unack_num>0)//ack later
			{
				if(stream_record->is_in_ack_timeout_queue==FALSE)//no ack in time wait
				{
					stream_record->timeout_ack=create_ack_task(src_ip, src_port, stream_record->stream_id, stream_record->max_ordered_seq);
                    stream_record->timeout_ack->father_stream=stream_record;
					q_join_tail(&(reliable_recv_handle->ack_timeout_queue),&(stream_record->ack_timeout_hook));
					stream_record->is_in_ack_timeout_queue=TRUE;
				}
				else
				{
					stream_record->timeout_ack->sequence=stream_record->max_ordered_seq;
				}
			}
			else//nothing to ack
			{
				;
			}
			stream_record->last_edit_tick=get_time_tick();
		}
		else//resend ack
		{
			ack_task=create_ack_task(src_ip, src_port,stream_record->stream_id, stream_record->max_ordered_seq);
            ack_task->father_stream=stream_record;
			stream_record->latest_ack_seq=stream_record->max_ordered_seq;
			q_join_tail(&(reliable_recv_handle->to_ack_queue),&(ack_task->hook));
			free(packet_data);
		}
	}
	else
	{
		packet_index=create_recv_data_index(packet_data, packet_len, src_ip, src_port);
		last_frag_num=count_last_frag_num_in_packet(packet_data);		
		bizman_lock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
		bizman_lock(&(stream_record->lock_ready_operate));
		stream_record->chunk_num+=last_frag_num;
		q_join_tail(&(stream_record->orderd_data_queue),&(packet_index->hook));
		if(FALSE==stream_record->is_in_ready_queue)
		{		
			if(reliable_recv_handle->chunk_mode!=TRUE||last_frag_num>0)
			{
				
				q_join_tail(&(reliable_recv_handle->ready_stream_index_queue), &(stream_record->ready_hook));
				stream_record->is_in_ready_queue=TRUE;
				if(reliable_recv_handle->choke_mode==TRUE)
				{
					bizman_cond_signal(&(reliable_recv_handle->cond_have_data_to_read));
				}
				
			}
		}
		stream_record->is_recv_complete=flag_stream_complete;
		bizman_unlock(&(stream_record->lock_ready_operate));
		bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
	}
	if(get_time_tick()-reliable_recv_handle->last_expire_time>TIMEOUT_RECV_STREAM_TICK)
	{
		expire_recv_stream_hash(&(reliable_recv_handle->stream_hash));
		reliable_recv_handle->last_expire_time=get_time_tick();
	}
	return 0;
}
ack_task_t* get_timewait_ack(reliable_recv_space_t* reliable_recv_handle)
{
	list_index_t *p_list=NULL;
	recv_stream_record_t* father_stream=NULL;
	ack_task_t* target=NULL;
	p_list=q_read_head(&(reliable_recv_handle->ack_timeout_queue));
	if(p_list!=NULL)
	{
		father_stream=(recv_stream_record_t*)(p_list->quiddity);
		if(get_time_tick()-father_stream->last_edit_tick > reliable_recv_handle->accumulate_ack_timeout_tick)
		{
			q_get_head(&(reliable_recv_handle->ack_timeout_queue));
			
			father_stream->is_in_ack_timeout_queue=FALSE;
			father_stream->latest_ack_seq=father_stream->max_ordered_seq;
			target=father_stream->timeout_ack;
			father_stream->timeout_ack=NULL;
        }

	}
	return target;
}
ack_task_t* get_definate_ack(reliable_recv_space_t* reliable_recv_handle)
{
	list_index_t *p_list=NULL;
	ack_task_t* target=NULL;
	recv_stream_record_t* father_stream=NULL;
	p_list=q_get_head(&(reliable_recv_handle->to_ack_queue));
	if(p_list!=NULL)
	{
		target=(ack_task_t*)p_list->quiddity;
        	father_stream=(recv_stream_record_t*)target->father_stream;
	}
	return target;
}
int get_ack_task(reliable_recv_space_t* reliable_recv_handle,ack_task_t* send_ack_task)
{
	ack_task_t* target=NULL;
    static uint32 s62_ack_seq=0;
	target=get_timewait_ack(reliable_recv_handle);
	if(target==NULL)
		target=get_definate_ack(reliable_recv_handle);
	if(target==NULL)
	{
		return 0;
	}
	else
	{
        if(target->stream_id==62)
        {
            s62_ack_seq=target->sequence;
        }
		send_ack_task->dest_ip=target->dest_ip;
		send_ack_task->dest_port=target->dest_port;
		memcpy((char*)(send_ack_task),(char*)target,sizeof(ack_task_t));
		free(target);
		return 1;
	}
}
int read_slice_from_packet(bizman_packet_t*packet,char* buf,int buf_len,int *offset,int *is_chunk_end)
{
	int i=0;
	bizman_slice_t* start_slice=NULL;
	int read_len=0,slice_left_len=0;

	if(*offset>=packet->len)
		return 0;
	if(0==*offset)
	{
		*offset=LOAD_HEADER_LEN+SLICE_HEADER_LEN;
	}
	if(0<*offset<LOAD_HEADER_LEN+SLICE_HEADER_LEN)
	{
		//fatal error;
	}
	start_slice=(bizman_slice_t*)((char*)packet+LOAD_HEADER_LEN);
	for(i=0;i<packet->slice_num;i++)
	{
		if(((char*)start_slice+start_slice->slice_len)-(char*)packet > *offset)
			break;
		else
			start_slice=(bizman_slice_t*)((char*)start_slice+start_slice->slice_len);
	}
	slice_left_len=(char*)start_slice-(char*)packet+start_slice->slice_len-*offset;
	read_len=MIN(buf_len,slice_left_len);
	memcpy(buf,(char*)packet+*offset,read_len);
	if(slice_left_len>buf_len){
		*offset+=buf_len;
		read_len=buf_len;
	}
	else
	{
		*offset+=slice_left_len+SLICE_HEADER_LEN;
		read_len=slice_left_len;
	}
	if(start_slice->flags&SLICE_LAST_FRAG&&read_len==slice_left_len)
		*is_chunk_end=TRUE;
	else
		*is_chunk_end=FALSE;
	return read_len;
}
/*
is_complete=0 insufficent buffer,copy buf_len content;
is_complete=1 stream and chunk complete;
is_complete=2 chunk complete;
*/
//lock space->lock stream
int copy_recv_data(reliable_recv_space_t* reliable_recv_handle,char* buf,int buf_len,unsigned int *src_ip,unsigned short *src_port,unsigned int *stream_id,unsigned int* is_complete)
{
	list_index_t *stream_list=NULL,*packet_index_list=NULL;
	recv_stream_record_t* ready_stream=NULL;
	recv_data_index_t* ready_data_index=NULL;
	bizman_packet_t* packet=NULL;
	int already_read_len=0,this_read_len=0;
	int is_chunk_end=FALSE,is_no_data_left=FALSE;
	*is_complete=BIZMAN_READ_CRAM;
	struct timespec recv_timeout;
	
	bizman_lock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
	while(stream_list==NULL){
		
		stream_list=q_read_head(&(reliable_recv_handle->ready_stream_index_queue));
		if(NULL==stream_list)
		{
			if(reliable_recv_handle->choke_mode==TRUE)
			{
				if(reliable_recv_handle->choke_timeout_mili_seconds==0)
				{
					bizman_cond_wait(&(reliable_recv_handle->cond_have_data_to_read),&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
				}
				else
				{
					recv_timeout.tv_sec=time(NULL)+reliable_recv_handle->choke_timeout_mili_seconds/1000;
					recv_timeout.tv_nsec=(reliable_recv_handle->choke_timeout_mili_seconds %1000)*1000000;
					if(0!=bizman_cond_timedwait(&(reliable_recv_handle->cond_have_data_to_read),&(reliable_recv_handle->mutexlock_ready_stream_index_queue),&recv_timeout))
					{
						bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
						return 0;
					}
				}
			}
			else
			{
				bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
				return 0;
			}
		}
	}
	ready_stream=(recv_stream_record_t*)(stream_list->quiddity);
	bizman_lock(&(ready_stream->lock_ready_operate));
//	bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));

	packet_index_list=q_read_head(&(ready_stream->orderd_data_queue));
	if(NULL==packet_index_list)
	{
		bizman_unlock(&(ready_stream->lock_ready_operate));
		bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
		return 0;
	}
	ready_data_index=(recv_data_index_t*)(packet_index_list->quiddity);
	packet=ready_data_index->packet;
	*src_ip=ready_data_index->src_ip;
	*src_port=ready_data_index->src_port;
	*stream_id=ready_stream->stream_id;
	
	while(FALSE==is_chunk_end && FALSE==is_no_data_left && already_read_len<buf_len)
	{
		this_read_len=read_slice_from_packet(packet, buf+already_read_len, buf_len-already_read_len,&(ready_stream->read_offset), &is_chunk_end);
		
		if(this_read_len==0)
		{
			
			ready_stream->read_offset=0;
			packet_index_list=q_get_head(&(ready_stream->orderd_data_queue));
			free_recv_data_index((recv_data_index_t*)(packet_index_list->quiddity));
			packet_index_list=q_read_head(&(ready_stream->orderd_data_queue));
			if(NULL==packet_index_list)//no data left in stream
			{
//				bizman_lock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
				stream_list=q_get_head(&(reliable_recv_handle->ready_stream_index_queue));//remove from ready stream queue
//				bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
				ready_stream->is_in_ready_queue=FALSE;
				if(TRUE==ready_stream->is_recv_complete)//stream is finished
				{
					*is_complete|=BIZMAN_READ_STREAM;
					ready_stream->is_read_complete=TRUE;
				}				
				is_no_data_left=TRUE;				
			}
			else
			{
				ready_data_index=(recv_data_index_t*)(packet_index_list->quiddity);
				packet=ready_data_index->packet;
			}
		}
		else
		{
			already_read_len+=this_read_len;

		}
	}
	if(is_chunk_end==TRUE)
	{
		*is_complete|=BIZMAN_READ_CHUNK;
		ready_stream->chunk_num--;
		if(reliable_recv_handle->chunk_mode==TRUE&&ready_stream->chunk_num==0)
		{
//			bizman_lock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
			stream_list=q_get_head(&(reliable_recv_handle->ready_stream_index_queue));//remove from ready stream queue
//			bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
			ready_stream->is_in_ready_queue=FALSE;
		}
	}
	else
	{
		printf("uncomplete chunk\n");
	}
	bizman_unlock(&(reliable_recv_handle->mutexlock_ready_stream_index_queue));
	bizman_unlock(&(ready_stream->lock_ready_operate));
	return already_read_len;
}

