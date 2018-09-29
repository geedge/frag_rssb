#include <stdlib.h>
#include "list.h"
#include "hash.h"
#include "businessman_memory.h"
#include "businessman_error.h"
#include "businessman_limit.h"
#include "businessman_time.h"
#include "reliable_send.h"


const time_tick_t TIMEOUT_SEND_STREAM_TICK=TIMEOUT_SEND_STREAM_USEC/TIME_TICK_STEP_USEC;


const unsigned int RELIABLE_SEND_HASH_SIZE=1024*1024;

send_data_index_t* create_send_data_index(send_data_refer_t* refer)
{
	send_data_index_t* block=NULL;
	block=(send_data_index_t*)b_malloc(sizeof(send_data_index_t));
	block->hook.quiddity=(void*)block;
	block->refer_index=refer;
	return block;
}
void free_index_and_data(send_data_index_t* p_data_index){
		free_refer_and_data(p_data_index->refer_index);
		free(p_data_index);
}
send_stream_t* create_send_stream(sub_send_task_t* smoothed_task,unsigned int window_size)
{
	send_stream_t* stream=NULL;
	unsigned int i=0;
	stream=(send_stream_t*)b_malloc(sizeof(send_stream_t));
	stream->stream_id=smoothed_task->stream_id;
	stream->dest_ip=smoothed_task->dest_ip;
	stream->dest_port=smoothed_task->dest_port;
	stream->is_relaible=smoothed_task->is_reliable;
	stream->last_edit_tick=get_time_tick();
	stream->bigger_sequence=START_SEQ_NUM;//sequence form 1,no zero
	stream->task=(stream_send_pane_t*)b_malloc(sizeof(stream_send_pane_t)*window_size);
	for(i=0;i<window_size;i++)
	{
		stream->task[i].order=i;
		stream->task[i].hook.quiddity=(void*)(&(stream->task[i]));
		stream->task[i].is_relaible=stream->is_relaible;
		stream->task[i].father_stream=(void*)stream;
	}
	stream->send_window_size=window_size;
	return stream;
}
int add_send_hash(hash_tab_t* send_hash,unsigned int stream_id,send_stream_t* stream)
{
	return _bizman_kernel_hash_add(send_hash, (uchar*)(&stream_id), sizeof(stream_id), (void*)stream);
}
int send_stream_hash_expire(void* p)
{
	send_stream_t*p_stream=(send_stream_t *)p;
	if(get_time_tick()-p_stream->last_edit_tick >TIMEOUT_SEND_STREAM_TICK&&p_stream->in_use_task_num==0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
send_stream_t* find_send_hash(hash_tab_t*stream_table,unsigned int stream_id)
{
	return (send_stream_t*)_bizman_kernel_hash_sel(stream_table, (uchar*)(&stream_id),sizeof(stream_id));
}
void free_send_stream(void* p)
{
	send_stream_t* stream=(send_stream_t*)p;
	list_index_t*p_list=NULL;
	send_data_index_t*p_data;
	//stream pane must leave timeout queue and task queue first before destroy the stream
	free(stream->task);
	while(NULL!=(p_list=q_get_head(&(stream->data_queue))))
	{
		p_data=(send_data_index_t*)(p_list->quiddity);
		free_index_and_data(p_data);
	}
	free(stream);
}

int creat_reliable_send_hash(hash_tab_t* reliable_send_hash)
{
	return _bizman_kernel_hash_create(reliable_send_hash, RELIABLE_SEND_HASH_SIZE, NULL, NULL, send_stream_hash_expire, free_send_stream);
}
int expire_send_hash(hash_tab_t* reliable_send_hash)
{
	return _bizman_kernel_hash_expire(reliable_send_hash);
}

void add_data_to_stream(send_stream_t* target_stream,send_data_index_t* data_index)
{
	if(target_stream->bigger_sequence==START_SEQ_NUM)
	{
		data_index->refer_index->packet->flags|=FLAG_EST_STREAM;
	}
	data_index->refer_index->packet->sequence=target_stream->bigger_sequence;
	target_stream->bigger_sequence++;
	q_join_tail(&(target_stream->data_queue), &(data_index->hook));
}
//when new smoothed data block add, do update to assign new block to idle task
void update_stream_task(reliable_send_space_t* send_space,send_stream_t* target_stream)
{
	unsigned int i=0;
	list_index_t* p_list=NULL;
	if(target_stream->in_use_task_num==send_space->send_widow_size)
		return;
	for(i=0;i<send_space->send_widow_size;i++){
		if(target_stream->task[i].state==PANE_STATE_SPARE)
		{
			p_list=q_get_head(&(target_stream->data_queue));
			if(p_list!=NULL)
			{
				target_stream->task[i].dest_ip=target_stream->dest_ip;
				target_stream->task[i].dest_port=target_stream->dest_port;
				target_stream->task[i].index_data=(send_data_index_t*)p_list->quiddity;
				target_stream->task[i].state=PANE_STATE_WAIT_SEND;
				
				q_join_tail(&(send_space->to_send_queue),&(target_stream->task[i].hook));

				target_stream->in_use_task_num++;
			}
			else
			{
				break;
			}
		}
	}
}
void reuse_stream_pane(reliable_send_space_t* send_space,stream_send_pane_t *idle_pane){
	send_stream_t* father_stream=NULL;
	list_index_t* pl_next=NULL;
	father_stream=(send_stream_t*)idle_pane->father_stream;
	free_index_and_data(idle_pane->index_data);
	send_space->stat_cached_packet_num--;
	pl_next=q_get_head(&(father_stream->data_queue));
	if(pl_next==NULL)
	{
		idle_pane->state=PANE_STATE_SPARE;
		idle_pane->index_data=NULL;
		idle_pane->send_times=0;
		father_stream->in_use_task_num--;
	}
	else
	{
		idle_pane->state=PANE_STATE_WAIT_SEND;
		idle_pane->index_data=(send_data_index_t*)(pl_next->quiddity);
		idle_pane->send_times=0;
		q_join_tail(&(send_space->to_send_queue), &(idle_pane->hook));
	}
}
void timeout_stream_pane(reliable_send_space_t* send_space,stream_send_pane_t *sended_pane)
{
	send_stream_t* father_stream=NULL;
	father_stream=(send_stream_t*)sended_pane->father_stream;
	sended_pane->last_send_time=get_time_tick();
	sended_pane->state=PANE_STATE_TIME_OUT;
	sended_pane->send_times++;
	q_join_tail(&(send_space->timeout_queue[sended_pane->send_times-1]), &(sended_pane->hook));
	

}
void clear_send_stream(reliable_send_space_t* send_space,send_stream_t* target_stream)
{
	list_index_t* pl_data=NULL;
	send_data_index_t* p_data_index=NULL;
	unsigned int i=0;
	pl_data=q_get_head(&(target_stream->data_queue));
	while(pl_data!=NULL)
	{
		p_data_index=(send_data_index_t*)(pl_data->quiddity);
		free_index_and_data(p_data_index); 
		pl_data=q_get_head(&(target_stream->data_queue));
		send_space->stat_cached_packet_num--;
		send_space->stat_frozen_pkts_num++;
	}
	for(i=0;i<send_space->send_widow_size;i++){
		switch (target_stream->task[i].state)
		{
			case PANE_STATE_SPARE:
				break;
			case PANE_STATE_WAIT_SEND:
				q_leave_list(&(send_space->to_send_queue),&(target_stream->task[i].hook));
				free_index_and_data(target_stream->task[i].index_data);
				target_stream->task[i].index_data=NULL;
				target_stream->in_use_task_num--;
				send_space->stat_cached_packet_num--;
				send_space->stat_frozen_pkts_num++;
				break;
			case PANE_STATE_TIME_OUT:
				q_leave_list(&(send_space->timeout_queue[target_stream->task[i].send_times-1]), &(target_stream->task[i].hook));
				free_index_and_data(target_stream->task[i].index_data);
				target_stream->task[i].index_data=NULL;
				target_stream->in_use_task_num--;
				send_space->stat_frozen_pkts_num++;
				send_space->stat_cached_packet_num--;
				break;
			default:
#ifdef BIZMAN_DEBUG
				fprintf(stderr,"error state at %s:%d\n",__FILE__,__LINE__);
#endif
				break;
				
		}
		target_stream->task[i].state=PANE_STATE_SPARE;
		target_stream->task[i].send_times=0;
		target_stream->task[i].last_send_time=0;
	}
	target_stream->last_edit_tick=get_time_tick();
	target_stream->is_unreachable=TRUE;
	target_stream->forzen_time_stamp=target_stream->last_edit_tick;
	target_stream->bigger_sequence=START_SEQ_NUM;
	target_stream->in_use_task_num=0;
}

stream_send_pane_t *get_pane_from_to_send(reliable_send_space_t* reliable_send_space)
{
	list_index_t *p_list=NULL;
	stream_send_pane_t* pane=NULL;
	p_list=q_get_head(&(reliable_send_space->to_send_queue));
	if(NULL==p_list)
	{
		return NULL;
	}
	else
	{
		pane=(stream_send_pane_t*)(p_list->quiddity);
		return pane;
	}
}
reliable_send_space_t* create_reliable_send_space()
{
	reliable_send_space_t* core_reliable_send_space=NULL;
	core_reliable_send_space=(reliable_send_space_t*)b_malloc(sizeof(reliable_send_space_t));
	if(NULL==core_reliable_send_space)
	{
		set_error(ERROR_TYPE_OUTOFMEM);
		return NULL;
	}
	creat_reliable_send_hash(&(core_reliable_send_space->stream_hash));
	
	core_reliable_send_space->cached_pkts_num_limit=0;
	core_reliable_send_space->resend_base_tick=DEFAULT_TIMEOUT_RELIABLE_SEND_BASE_TICK;
	core_reliable_send_space->send_widow_size=DEFALUT_SEND_WINDOW_SIZE;
	core_reliable_send_space->frozen_stream_interval=DEFAULT_STREAM_FROZEN_TICK_INTERVAL;
	core_reliable_send_space->resend_times=MAX_RESEND_TIMES;
	return core_reliable_send_space;
}
void destroy_reliable_send_space(reliable_send_space_t* reliable_send_space)
{
	list_index_t* pl_index=NULL;
	stream_send_pane_t *pane=NULL;
	unsigned int i=0;
	pl_index=q_get_head(&(reliable_send_space->to_send_queue));
	while(pl_index!=NULL)
	{
		pane=(stream_send_pane_t *)(pl_index->quiddity);
		free_index_and_data(pane->index_data);
		pl_index=q_get_head(&(reliable_send_space->to_send_queue));
	}
	for(i=0;i<reliable_send_space->resend_times;i++){
		pl_index=q_get_head(&(reliable_send_space->timeout_queue[i]));
		while(pl_index!=NULL)
		{
			pane=(stream_send_pane_t *)(pl_index->quiddity);
			free_index_and_data(pane->index_data);
			pl_index=q_get_head(&(reliable_send_space->timeout_queue[i]));
		}
	}
	_bizman_kernel_hash_destroy(&(reliable_send_space->stream_hash),NULL);
}
int join_to_reliable_send(reliable_send_space_t*reliable_space,sub_send_task_t* smoothed_task)
{
	send_stream_t* this_stream=NULL;
	send_data_index_t* index=NULL;
	
	if(reliable_space->cached_pkts_num_limit!=0&&reliable_space->stat_cached_packet_num>reliable_space->cached_pkts_num_limit)
	{
		reliable_space->stat_dropped_pkts_num++;
		return -1;
	}
	if(get_time_tick()-reliable_space->last_expire_time>TIMEOUT_SEND_STREAM_TICK)
	{
		expire_send_hash(&(reliable_space->stream_hash));
		reliable_space->last_expire_time=get_time_tick();
	}
	this_stream=find_send_hash(&(reliable_space->stream_hash),smoothed_task->stream_id);
	if(this_stream==NULL)
	{
		this_stream=create_send_stream(smoothed_task,reliable_space->send_widow_size);
		add_send_hash(&(reliable_space->stream_hash),smoothed_task->stream_id, this_stream);
	}
	this_stream->last_edit_tick=get_time_tick();
	if(this_stream->is_unreachable==TRUE)
	{
#ifdef BIZMAN_DEBUG
	fprintf(stderr,"stream %d was set unreachable \n",smoothed_task->stream_id);
#endif
		if(this_stream->last_edit_tick-this_stream->forzen_time_stamp>reliable_space->frozen_stream_interval)
		{
			this_stream->is_unreachable=FALSE;
		}
		else
		{
			reliable_space->stat_frozen_pkts_num++;
			return -1;
		}
	}
	index=create_send_data_index(smoothed_task->refer_index);
	
	add_data_to_stream(this_stream, index);
	reliable_space->stat_cached_packet_num++;
	update_stream_task(reliable_space, this_stream);

#ifdef BIZMAN_DEBUG
	for(int i=0;i<reliable_space->resend_times;i++)
	{
		if(0>q_list_check(&(reliable_space->timeout_queue[i]), NULL))
		{
			fprintf(stderr,"Timeout Queue %d Check Error.\n",i);
		}	
	}
	if(0>q_list_check(&(reliable_space->to_send_queue), NULL))
	{
		fprintf(stderr,"To send Queue Check Error.\n");
	}
#endif 
	//check expire stream id in hash
	return 0;
}
stream_send_pane_t* get_pane_from_timeout(reliable_send_space_t*reliable_space)
{
	stream_send_pane_t* tmp_pane=NULL,*target_pane=NULL;
	list_index_t* p_list_head=NULL;
	int i=0;
	for(i=reliable_space->resend_times-1;i>=0;i--)
	{
		p_list_head=q_read_head(&(reliable_space->timeout_queue[i]));
		if(p_list_head!=NULL)
		{
			tmp_pane=(stream_send_pane_t*)(p_list_head->quiddity);
			if(get_time_tick()-tmp_pane->last_send_time > reliable_space->resend_base_tick*TIMEOUT_RELIABLE_SEND_MULTIPLE[i])
			{
				p_list_head=q_get_head(&(reliable_space->timeout_queue[i]));
				target_pane=(stream_send_pane_t*)(p_list_head->quiddity);
				break;
			}
		}	
	}
	return target_pane;
}
void* get_reliable_send_task(reliable_send_space_t*reliable_space,sub_send_task_t*send_task)
{
	stream_send_pane_t* target_pane=NULL;
	send_stream_t* father_stream=NULL;
	int flag=0;
	target_pane=get_pane_from_timeout(reliable_space);
	if(target_pane==NULL)
	{
		target_pane=get_pane_from_to_send(reliable_space);		
		flag++;//for debug
	}
	if(target_pane==NULL)
	{
		return NULL;
	}
	reliable_space->stat_send_packet_count[target_pane->send_times]++;
	if(target_pane->index_data->refer_index->packet->sequence==START_SEQ_NUM)
	{
		if(target_pane->send_times==0)
		{
			reliable_space->stat_send_stream_num++;
		}
		else if(target_pane->send_times==1)
		{
			reliable_space->stat_resend_stream_num++;
		}
	}
	if(target_pane->send_times>=reliable_space->resend_times)
	{
		//clear the pane
		free_index_and_data(target_pane->index_data);
		target_pane->state=PANE_STATE_SPARE;
		reliable_space->stat_cached_packet_num--;
		//then clear the whole stream
		clear_send_stream(reliable_space, (send_stream_t*)(target_pane->father_stream));
		//set_unreachable(s);
		return NULL;
	}
	send_task->refer_index=target_pane->index_data->refer_index;
	send_task->dest_ip=target_pane->dest_ip;
	send_task->dest_port=target_pane->dest_port;
	father_stream=(send_stream_t*)(target_pane->father_stream);
	send_task->stream_id=father_stream->stream_id;
	return (void*)target_pane;
	
}
void roll_send_window(reliable_send_space_t* reliable_space,uint32 stream_id,uint32 sequence)
{
		unsigned int i=0;
		send_stream_t* this_stream=find_send_hash(&(reliable_space->stream_hash),stream_id);
		
#ifdef BIZMAN_DEBUG_DETAILS
		printf("Recv ACK %llu :stream=%d seq=%d\n",get_time_tick(),stream_id,sequence);
#endif
		if(this_stream==NULL)
		{
#ifdef BIZMAN_DEBUG
			fprintf(stderr,"ACK:unrecognized stream id=%u\n",stream_id);
#endif
			return;
		}
		if(TRUE==this_stream->is_unreachable)
		{
#ifdef BIZMAN_DEBUG
			fprintf(stderr,"ACK:stream id=%u was set unreachable.\n",stream_id);
#endif
			return;
		}
		if((uint32)this_stream->bigger_sequence<sequence||this_stream->bigger_sequence-sequence>reliable_space->send_widow_size+this_stream->data_queue.listcount)
		{
			reliable_space->stat_outwindow_ack++;
			return;
		}
		for(i=0;i<reliable_space->send_widow_size;i++)
		{
			if(this_stream->task[i].state==PANE_STATE_TIME_OUT && this_stream->task[i].index_data->refer_index->packet->sequence <= sequence)
			{
				q_leave_list(&(reliable_space->timeout_queue[this_stream->task[i].send_times-1]),&(this_stream->task[i].hook) );
				reuse_stream_pane(reliable_space,&(this_stream->task[i]));
			}
		}
		return;
}
int handle_ack(reliable_send_space_t*reliable_space,bizman_packet_t* ack_packet){
	int each_ack_slice=0;
	uint32 stream_id=0,sequence=0;
	bizman_slice_t* slice=NULL;
	reliable_space->stat_ack_pkts_cnt++;
	if(!(ack_packet->flags&FLAG_ACK_PKT))
	{
		return -1;
	}
	if(ack_packet->slice_num==0)//old version, unsmoothed ack packet;
	{
		roll_send_window(reliable_space,ack_packet->id,ack_packet->sequence);
		return 0;
	}
	slice=(bizman_slice_t*)((char*)ack_packet+LOAD_HEADER_LEN);
	for(each_ack_slice=0;each_ack_slice<ack_packet->slice_num;each_ack_slice++)
	{
		if(slice->slice_len!=sizeof(uint32)*2+SLICE_HEADER_LEN)
		{
			return -1;
		}
		stream_id=*(uint32*)((char*)slice+SLICE_HEADER_LEN);
		sequence=*(uint32*)((char*)slice+SLICE_HEADER_LEN+sizeof(uint32));
		slice=(bizman_slice_t* )((char*)slice+slice->slice_len);
		
		roll_send_window(reliable_space,stream_id,sequence);
	}
	return 0;
}

void recycle_transition_handle(reliable_send_space_t*reliable_space,void *p){
	stream_send_pane_t* tmp_pane=(stream_send_pane_t*)p;
#ifdef BIZMAN_DEBUG_DETAILS
	printf("Send DAT %llu:stream=%d seq=%d\n",get_time_tick(),tmp_pane->index_data->refer_index->packet->id,tmp_pane->index_data->refer_index->packet->sequence);
#endif
	if(tmp_pane->is_relaible==FALSE)
	{
		
		reuse_stream_pane(reliable_space,tmp_pane);
	}
	else
	{
		timeout_stream_pane(reliable_space,tmp_pane);
	}
}
int set_send_cached_pkts_limit(reliable_send_space_t*reliable_space,unsigned long long max_cached)
{
	reliable_space->cached_pkts_num_limit=max_cached;
	return 1;
}
unsigned long long stat_first_resend_packet_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stat_send_packet_count[1];
}
unsigned long long stat_first_send_packet_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stat_send_packet_count[0];
}
unsigned long long stat_last_resend_packet_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stat_send_packet_count[reliable_space->resend_times];
}
unsigned long long stat_dropped_packet_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stat_dropped_pkts_num;
}
unsigned long long stat_frozen_packet_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stat_frozen_pkts_num;
}
unsigned long long  stat_ack_packet_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stat_ack_pkts_cnt;
}
unsigned long long stat_frist_send_stream_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stat_send_stream_num;
}
unsigned long long stat_frist_resend_stream_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stat_resend_stream_num;
}
unsigned long long stat_active_stream_count(reliable_send_space_t*reliable_space)
{
	return reliable_space->stream_hash.elem_count;
}

