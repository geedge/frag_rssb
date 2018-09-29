/*
	Businessman Data Exchange System Top Level structure
*/
#include <sys/syscall.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>//using memset
#include <pthread.h>
#include <errno.h>	//errorno
#include "businessman_limit.h"
#include "businessman_packet.h"
#include "businessman_error.h"
#include "businessman_time.h"
#include "businessman_memory.h"
#include "smooth_send.h"
#include "reliable_send.h"
#include "reliable_recv.h"
#include "stream_recover.h"
#include "network_layer.h"
#include "businessman_core.h"
#include "bizman.h"
int assign_task(void *(*worker)(void *),void * params)
{
	pthread_t	 thread_desc;
	pthread_attr_t attr;
	memset(&thread_desc, 0, sizeof(thread_desc));
	memset(&attr, 0, sizeof(attr));	
	
	if(0 != pthread_attr_init(&(attr))) {
		set_user_error("pthread_attr_init(): %d %s", errno, strerror(errno));
		return -1;
	}
	if(0 != pthread_attr_setdetachstate(&(attr), PTHREAD_CREATE_DETACHED)) {
		set_user_error("pthread_attr_setdetachstate(): %d %s", errno, strerror(errno));
		return -1;
	}
	if(0 != pthread_create(&(thread_desc), &(attr), (void *(*)(void *))(worker), (void *)(params))) {
		set_user_error("pthread_create(): %d %s", errno, strerror(errno));
		pthread_attr_destroy(&(attr));
		return -1;
	}
	pthread_attr_destroy(&(attr));
	return 0;

}
unsigned long long make_app_id_by_dest(unsigned int dest_ip,unsigned short dest_port)
{
	unsigned long long app_id;
	app_id=dest_ip;
	app_id<<=32;
	app_id+=dest_port;
	return app_id;
}
external_send_task_t* create_external_send_task(const char* data,int len,uint64 app_id,uint16 flags,const uint32* dest_ip,const uint16*dest_port,uint32 dest_num)
{
	external_send_task_t* send_task=NULL;
	send_task=(external_send_task_t*)b_malloc(sizeof(external_send_task_t));
	send_task->data=data;
	send_task->len=len;
	send_task->app_id=app_id;
	send_task->flags=flags;
	send_task->dest_num=dest_num;
	send_task->dest_ip=(unsigned int *)b_malloc(dest_num*sizeof(*dest_ip));
	memcpy(send_task->dest_ip,dest_ip,dest_num*sizeof(*dest_ip));
	send_task->dest_port=(unsigned short *)b_malloc(dest_num*sizeof(*dest_port));
	memcpy(send_task->dest_port,dest_port,dest_num*sizeof(*dest_port));
	return send_task;
}
void destroy_external_send_task(external_send_task_t* send_task)
{
	free(send_task->dest_ip);
	send_task->dest_ip=NULL;
	free(send_task->dest_port);
	send_task->dest_port=NULL;
	send_task->dest_num=0;
	free(send_task);
}
char* bizman_get_error_string(void){
	return get_error_string();
}

void * bizman_core_send_thread(void* param)
{
	bizman_handle_t* handle=(bizman_handle_t*)param;
	bizman_send_space_t* send_space=handle->send_space;
//	smooth_block_t* already_smoothed_block=NULL;
	internal_send_task_t already_smoothed_task;
	sub_send_task_t sub_already_smoothed_task,reliable_send_task;
	void *reliable_send_transition_handle=NULL;
	char ack_buf[MTU_SIZE];
	int i=0;
	unsigned int j=0;
	int recv_ack_len=0;
	unsigned int ack_src_ip;
	unsigned short  ack_src_port;
	int is_thread_idle=TRUE;
/*
	int s=0;
	cpu_set_t cpuset;
	pthread_t thread;
	thread = pthread_self();

	CPU_ZERO(&cpuset);
	CPU_SET(8, &cpuset);
	s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (s != 0)
		printf("bizman:pthread_setaffinity_np failed.\n");

	s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (s != 0)
		printf("bizman:pthread_getaffinity_np failed.\n");

	printf("Set returned by pthread_getaffinity_np() contained:\n");
	for (i = 0; i < CPU_SETSIZE; i++)
		if (CPU_ISSET(i, &cpuset))
			printf("    CPU %d\n", i);
*/
	while(handle->should_sender_active==TRUE)
	{
		is_thread_idle=TRUE;
		memset(&reliable_send_task,0,sizeof(reliable_send_task));
		//put smoothed block to reliable send space
		for(i=0;i<MAX_THREAD_SUPPORT;i++)
		{
			if(send_space->thread_smooth_space[i]!=NULL)
			{
				memset(&already_smoothed_task,0,sizeof(already_smoothed_task));
				while(get_smoothed_task(send_space->thread_smooth_space[i],&already_smoothed_task)!=0)
				{
					for(j=0;j<already_smoothed_task.dest_num;j++)
					{
						memset(&sub_already_smoothed_task,0,sizeof(sub_already_smoothed_task));
						sub_already_smoothed_task.stream_id=already_smoothed_task.sub_stream_id[j];
						sub_already_smoothed_task.dest_ip=already_smoothed_task.dest_ip[j];
						sub_already_smoothed_task.dest_port=already_smoothed_task.dest_port[j];
						sub_already_smoothed_task.flags=already_smoothed_task.flags;
						sub_already_smoothed_task.is_reliable=already_smoothed_task.is_reliable;
						sub_already_smoothed_task.refer_index=already_smoothed_task.data;
						if(0>join_to_reliable_send(send_space->core_reliable_space,&sub_already_smoothed_task))
						{
								free_refer_and_data(already_smoothed_task.data);
								sub_already_smoothed_task.refer_index=NULL;
						}
							
					}
					free(already_smoothed_task.dest_ip);
					free(already_smoothed_task.dest_port);
					free(already_smoothed_task.sub_stream_id);
					memset(&already_smoothed_task,0,sizeof(already_smoothed_task));
					is_thread_idle=FALSE;			
				}
			}
		}
		//do reliable send
		for(i=0;i<MAX_DAT_SEND_PER_LOOP;i++)
		{
			reliable_send_transition_handle=get_reliable_send_task(send_space->core_reliable_space, &reliable_send_task);
			if(reliable_send_transition_handle!=NULL)
			{
				reliable_send_task.refer_index->packet->id=reliable_send_task.stream_id;
				do_actual_send(handle->network_layer,reliable_send_task.dest_ip, reliable_send_task.dest_port, (const char*)(reliable_send_task.refer_index->packet), reliable_send_task.refer_index->data_len);
				recycle_transition_handle(send_space->core_reliable_space,reliable_send_transition_handle);
				is_thread_idle=FALSE;
			}
			else
			{
				break;
			}
		}
		//do ack received operation
		for(i=0;i<MAX_ACK_RECV_PER_LOOP;i++){
			memset(ack_buf,0,sizeof(ack_buf));
			recv_ack_len=do_actual_recv(handle->network_layer, &ack_src_ip, &ack_src_port, ack_buf, sizeof(ack_buf));
			if(recv_ack_len>0&&TRUE==is_bizman_packet((bizman_packet_t*)ack_buf))
			{
				is_thread_idle=FALSE;
				handle_ack(send_space->core_reliable_space,(bizman_packet_t*)ack_buf);
			}
			else
			{
				break;
			}
		}
		if(is_thread_idle==TRUE)
		{
			usleep(10);
		}
	}
	return NULL;
}
void * bizman_core_recv_thread(void* param)
{
	bizman_handle_t* handle=(bizman_handle_t*)param;
	bizman_recv_space_t* recv_space=handle->recv_space;
	int i=0;
	unsigned int data_src_ip;
	unsigned short data_src_port;
	char* dat_packet_buf=NULL;
	ack_task_t ack_task;
	external_send_task_t* ack_to_smooth_task=NULL;
	internal_send_task_t ack_smoothed_task;
	uint64 ack_app_id=0;
	char ack_packet_buf[MTU_SIZE];
	int dat_packet_length=0;
	int is_thread_idle=TRUE;
	while(handle->should_receiver_active==TRUE)
	{
		is_thread_idle=TRUE;
		for(i=0;i<MAX_DAT_RECV_PER_LOOP;i++)
		{
			dat_packet_buf=(char*)b_malloc(sizeof(char)*MTU_SIZE);
			dat_packet_length=do_actual_recv(handle->network_layer,&data_src_ip, &data_src_port,dat_packet_buf, MTU_SIZE);
			if(dat_packet_length>0)
			{	
				is_thread_idle=FALSE;
				handle->recv_space->stat_packet_recv_num++;
				if(TRUE==is_bizman_packet((bizman_packet_t*) dat_packet_buf))
				{
					add_raw_packet(recv_space->reliable_recv_space,data_src_ip,data_src_port,(bizman_packet_t*) dat_packet_buf,dat_packet_length);
				}
				else
				{
					handle->recv_space->stat_invalid_packet_recv_num++;
					free(dat_packet_buf);
				}
			}
			else
			{
				free(dat_packet_buf);
				//break;
			}
		}
		for(i=0;i<MAX_ACK_RECV_PER_LOOP;i++)
		{
			memset(&ack_task,0,sizeof(ack_task));
			if(get_ack_task(recv_space->reliable_recv_space, &ack_task))
			{
#ifdef BIZMAN_DEBUG_DETAILS
				printf("Send ACK %llu:stream=%d seq=%d\n",get_time_tick(),ack_task.stream_id,ack_task.sequence);
#endif
				is_thread_idle=FALSE;
				ack_app_id=make_app_id_by_dest(ack_task.dest_ip,ack_task.dest_port);
				memcpy(ack_packet_buf,(char*)(&(ack_task.stream_id)),sizeof(ack_task.stream_id));
				memcpy(ack_packet_buf+sizeof(ack_task.stream_id),(char*)(&(ack_task.sequence)),sizeof(ack_task.sequence));
				ack_to_smooth_task=create_external_send_task(ack_packet_buf,
															sizeof(ack_task.stream_id)+sizeof(ack_task.sequence),
															ack_app_id,
															0,
															&(ack_task.dest_ip),
															&(ack_task.dest_port),
															1);
				smooth_block(recv_space->ack_send_smooth_space, ack_to_smooth_task);
				destroy_external_send_task(ack_to_smooth_task);
			}
			else
			{
				break;
			}
		}
		for(i=0;i<MAX_ACK_RECV_PER_LOOP;i++)
		{
			if(get_smoothed_task(recv_space->ack_send_smooth_space,&ack_smoothed_task))
			{
				is_thread_idle=FALSE;
				ack_smoothed_task.data->packet->flags|=FLAG_ACK_PKT;
				do_actual_send(handle->network_layer, ack_smoothed_task.dest_ip[0],
													ack_smoothed_task.dest_port[0], 
													(const char*)(ack_smoothed_task.data->packet),
													ack_smoothed_task.data->data_len);
				handle->recv_space->stat_ack_send_num++;
				free(ack_smoothed_task.dest_ip);
				free(ack_smoothed_task.dest_port);
				free(ack_smoothed_task.sub_stream_id);
				free_refer_and_data(ack_smoothed_task.data);
				memset(&ack_smoothed_task,0,sizeof(ack_smoothed_task));		
			}
			else
			{
				break;
			}
		}
		if(is_thread_idle==TRUE)
		{
			usleep(10);
		}
	}
	return NULL;
}

bizman_send_space_t *bizman_create_sender(unsigned int flags){

	bizman_send_space_t* sender=NULL;
	
	sender=(bizman_send_space_t*)b_malloc(sizeof(bizman_send_space_t));
	if(sender==NULL)
	{
		set_error(ERROR_TYPE_OUTOFMEM);
		return NULL;
	}
	sender->core_reliable_space=create_reliable_send_space();
	if(sender->core_reliable_space==NULL)
	{
		return NULL;
	}
	sender->send_smooth_timeout_tick=DEFAULT_TIMEOUT_SMOOTH_BLOCK_USEC/TIME_TICK_STEP_USEC;
	return sender;
	
}
void bizman_destroy_sender(bizman_send_space_t * sender)
{
	int i=0;
	for(i=0;i<MAX_THREAD_SUPPORT;i++){
		if(sender->thread_smooth_space[i]!=NULL)
		{
			destroy_smooth_send_space(sender->thread_smooth_space[i]);
		}
	}
	destroy_reliable_send_space(sender->core_reliable_space);
	free(sender);
}
bizman_recv_space_t *bizman_create_receiver(unsigned int flags)
{
	bizman_recv_space_t*receiver=NULL;
	receiver=(bizman_recv_space_t *)b_malloc(sizeof(bizman_recv_space_t));
	if(receiver==NULL)
	{
		set_error(ERROR_TYPE_OUTOFMEM);
		return NULL;
	}
	receiver->reliable_recv_space=create_reliable_recv_space();
	if(receiver->reliable_recv_space==NULL)
	{
		return NULL;
	}
	receiver->ack_send_smooth_space=create_smooth_send_space();
	
	receiver->stream_recover_space=get_stream_recover_space();
	if(receiver->stream_recover_space==NULL)
	{
		//return NULL;
	}
	if(flags&BIZMAN_CHUNK_RECEIVE)
	{
		receiver->reliable_recv_space->chunk_mode=TRUE;
	}
	if(flags&BIZMAN_CHOKE_RECEIVE)
	{
		receiver->reliable_recv_space->choke_mode=TRUE;
	}
	return receiver;
}
void bizman_destroy_receiver(bizman_recv_space_t *receiver)
{
	reliable_recv_space_t* reliable_recv_space=receiver->reliable_recv_space;
	destroy_reliable_recv_space(reliable_recv_space);
	
}
/*return 0 when success ,return -1 when failed*/
int bizman_listen(void* raw_handle,unsigned short port){
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	if(NULL==handle)
	{
		set_error(ERROR_TYPE_INVALID_PARAMETER);
		return -1;
	}
	if(FALSE==handle->should_receiver_active)
	{
//		handle->should_receiver_active=TRUE;
	}
	return set_network_listen_port(handle->network_layer, port);
	
}
/*return NULL when failed*/
void *bizman_get_handle(void){
	bizman_handle_t* handle=NULL;
	handle=(bizman_handle_t*)b_malloc(sizeof(bizman_handle_t));
	if(handle==NULL)
	{
		set_error(ERROR_TYPE_OUTOFMEM);
		return NULL;
	}
	handle->network_layer=get_network_handle();
	if(handle->network_layer==NULL)
	{
		return NULL;
	}
	return (void*)handle;
}
void bizman_destroy_handle(void * raw_handle)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	if(handle->recv_space!=NULL)
	{
		handle->should_receiver_active=FALSE;
		bizman_destroy_receiver(handle->recv_space);
	}
	if(handle->send_space!=NULL)
	{
		handle->should_sender_active=FALSE;
		bizman_destroy_sender(handle->send_space);
	}
	destroy_network_handle(handle->network_layer);
}
/*return 0 when success ,return -1 when failed*/
int bizman_set_handle(void* raw_handle,unsigned short flags)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	if(NULL==handle)
	{
		set_error(ERROR_TYPE_INVALID_PARAMETER);
		return -1;
	}
	handle->flags|=flags;
	if(flags&BIZMAN_SENDER_ACTIVE)
	{
		handle->send_space=bizman_create_sender(handle->flags);
		handle->should_sender_active=TRUE;
		
	}
	if(flags&BIZMAN_RECEIVER_ACTIVE)
	{
		handle->recv_space=bizman_create_receiver(handle->flags);
		handle->should_receiver_active=TRUE;
	}
	return 0;
}
int bizman_set_handle_parameter(void * raw_handle, int type, unsigned int value)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	if(NULL==handle)
	{
		set_error(ERROR_TYPE_INVALID_PARAMETER);
		return -1;
	}
	if(handle->send_space==NULL&&handle->recv_space==NULL)
	{
		set_error(ERROR_TYPE_INVALID_PARAMETER);
		return -1;				
	}
	switch(type)
	{
		case BIZMAN_PARAMETER_ACK_ACCUMULATE_MSEC:
			handle->recv_space->reliable_recv_space->accumulate_ack_timeout_tick=value*1000/TIME_TICK_STEP_USEC;
			break;
		case BIZMAN_PARAMETER_ACK_ACCUMULATE_NUM:
			handle->recv_space->reliable_recv_space->accumulate_ack_num=value;
			break;
		case BIZMAN_PARAMETER_RECV_WINDOWSIZE:
			handle->recv_space->reliable_recv_space->receive_window_size=value;
			break;
		case BIZMAN_PARAMETER_ACK_SMOOTH_TIME_MSEC:
			handle->recv_space->ack_send_smooth_space->smooth_timeout_tick=value;
			break;
		case BIZMAN_PARAMETER_RESEND_BASE_MSEC:
			handle->send_space->core_reliable_space->resend_base_tick=value*1000/TIME_TICK_STEP_USEC;
			break;
		case BIZMAN_PARAMETER_RESEND_TIMES:
			if(value<=MAX_RESEND_TIMES)
			{
				handle->send_space->core_reliable_space->resend_times=value;
			}
			else
			{
				set_error(ERROR_TYPE_INVALID_PARAMETER);
				return -1;
			}
			break;
		case BIZMAN_PARAMETER_SEND_WINDOWSIZE:
			handle->send_space->core_reliable_space->send_widow_size=value;
			break;
		case BIZMAN_PARAMETER_SEND_SMOOTH_TIME_MSEC:
			handle->send_space->send_smooth_timeout_tick=value*1000/TIME_TICK_STEP_USEC;
			break;
		case BIZMAN_PARAMETER_SEND_CACHE_PKTS_LIMIT:
			handle->send_space->core_reliable_space->cached_pkts_num_limit=value;
			break;
		case BIZMAN_PARAMETER_CHOKE_RECEIVE:
			handle->recv_space->reliable_recv_space->choke_mode=TRUE;
			break;
		case BIZMAN_PARAMETER_CHUNK_RECEIVE:
			handle->recv_space->reliable_recv_space->chunk_mode=TRUE;
			break;
		case BIZMAN_PARAMETER_FROZEN_STREAM_MSEC:
			handle->send_space->stream_frozen_stream_time_tick=value*1000/TIME_TICK_STEP_USEC;
			break;
		case BIZMAN_PARAMETER_SMOOTH_CACHE_PKTS_LIMIT:
			handle->send_space->cached_smooth_pkts_limit=value;
			break;
		case BIZMAN_PARAMETER_CHOKE_RECV_TIMEOUT_MSEC:
			handle->recv_space->reliable_recv_space->choke_timeout_mili_seconds=value;
			break;
		default:
			set_error(ERROR_TYPE_INVALID_PARAMETER);
			return -1;
	}
	return 0;
}
int bizman_set_handle_option(void* raw_handle,int option_type, const char* option_value)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	int ret=0;
	switch(option_type)
	{
		case BIZMAN_OPT_SOCKET_INTERFACE_NAME:
			set_network_interface(handle->network_layer, BIZMAN_INTERFACE_SOCKET,option_value);
			break;
		case BIZMAN_OPT_PAG_INTERFACE_NAME:
			set_network_interface(handle->network_layer, BIZMAN_INTERFACE_PAG,NULL);
			break;
		default:
			set_user_error("invalid option type.");
			ret=-1;
			break;
	}
	return ret;
}
int bizman_set_pkts_cached_limit(void* raw_handle,unsigned long long cached_pkts_limit)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	handle->sender_cached_pkts_limit=cached_pkts_limit;
	return 1;
}
/*return 0 when success ,return -1 when failed*/
int bizman_init_handle(void* raw_handle)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	if(NULL==handle)
	{
		set_error(ERROR_TYPE_INVALID_PARAMETER);
		return -1;
	}
	if(0>init_network_layer(handle->network_layer))
	{
		return -1;
	}
	bizman_timer_start();
	if(handle->should_sender_active)
	{
		if(NULL==handle->send_space)
		{
			return -1;
		}
		if(0>assign_task((void *(*)(void *))(bizman_core_send_thread), (void *)handle))
		{
			return -1;
		}
	}
	if(handle->should_receiver_active)
	{
		if(NULL==handle->recv_space)
		{
			return -1;
		}
		if(0>assign_task((void *(*)(void *))(bizman_core_recv_thread), (void *)handle))
		{
			return -1;;
		}
	}

	return 0;
}

int bizman_send_stream(bizman_send_space_t* sender,unsigned int thread_id,const unsigned int *dest_ip,const unsigned short* dest_port,unsigned int dest_num,
					const char*data,unsigned int len,unsigned long long app_id,unsigned short flags)
{
	smooth_space_t* thread_smooth_space=NULL;

	external_send_task_t *send_task;
	int ret;
	if(sender==NULL)
	{
		set_error(ERROR_TYPE_INVALID_PARAMETER);
		return -1;
	}

	if(sender->thread_smooth_space[thread_id%MAX_THREAD_SUPPORT]==NULL)
	{
		thread_smooth_space=create_smooth_send_space();
		thread_smooth_space->smooth_timeout_tick=sender->send_smooth_timeout_tick;
		thread_smooth_space->cache_smooth_pkts_limit=sender->cached_smooth_pkts_limit;
		if(thread_smooth_space==NULL)
		{
			return -1;
		}
		sender->thread_smooth_space[thread_id%MAX_THREAD_SUPPORT]=thread_smooth_space;
	}
	else
	{
		thread_smooth_space=sender->thread_smooth_space[thread_id%MAX_THREAD_SUPPORT];
	}
	send_task=create_external_send_task(data,len,app_id, flags, dest_ip,dest_port,dest_num);
	
	ret=smooth_block(thread_smooth_space, send_task);
	destroy_external_send_task(send_task);
	send_task=NULL;
	if(ret<0)
	{
		return -1;
	}
	return len;
}

int bizman_recv(void* raw_handle,char* buff,int buf_len,unsigned int *src_ip,unsigned short *src_port,unsigned int *stream_id,unsigned int* is_complete)
{	
		bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
		bizman_recv_space_t* receiver=handle->recv_space;
		return copy_recv_data(receiver->reliable_recv_space, buff,buf_len, src_ip,src_port,stream_id,is_complete);
}

int bizman_send(void* raw_handle,unsigned int thread_id,unsigned int dest_ip,unsigned short dest_port,
					const char*data,unsigned int len,unsigned long long app_id,unsigned short flags)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	bizman_send_space_t* sender=handle->send_space;
	if(flags&BIZMAN_SMOOTH_DEST)
	{
		app_id=make_app_id_by_dest(dest_ip,dest_port);
	}
	return bizman_send_stream(sender,thread_id,&dest_ip,&dest_port,1,
					data,len,app_id,flags);
}
int bizman_multi_send(void* raw_handle,unsigned int thread_id,const unsigned int *dest_ip,const unsigned short *dest_port,unsigned int dest_num,
					const char*data,unsigned int len,unsigned long long app_id,unsigned short flags)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	bizman_send_space_t* sender=handle->send_space;
	return bizman_send_stream(sender,thread_id,dest_ip,dest_port,dest_num,
					data,len,app_id,flags);
}
unsigned long long bizman_stat(void* raw_handle,int type)
{
	bizman_handle_t* handle=(bizman_handle_t*)raw_handle;
	bizman_send_space_t* sender=handle->send_space;
	unsigned long long result=0;
	switch(type)
	{
		case	BIZMAN_STAT_FIRST_SEND_PKTS:
			result=stat_first_send_packet_count(sender->core_reliable_space);
			break;
		case  	BIZMAN_STAT_FIRST_RESEND_PKTS:
			result=stat_first_resend_packet_count(sender->core_reliable_space);
			break;
		case  	BIZMAN_STAT_LAST_RESEND:
			result=stat_last_resend_packet_count(sender->core_reliable_space);
			break;
		case  	BIZMAN_STAT_MEM_DROPPED:
			result=stat_dropped_packet_count(sender->core_reliable_space);
			break;			
		case  	BIZMAN_STAT_FROZEN_DROPPED:
			result=stat_frozen_packet_count(sender->core_reliable_space);
			break;
		case  	BIZMAN_STAT_ACK_PKT_NUM:
			result=stat_ack_packet_count(sender->core_reliable_space);
			break;
		case  	BIZMAN_STAT_FIRST_SEND_STREAM:
			result=stat_frist_send_stream_count(sender->core_reliable_space);
			break;
		case  	BIZMAN_STAT_FIRST_RESEND_STREAM:
			result=stat_frist_resend_stream_count(sender->core_reliable_space);
			break;
		case  	BIZMAN_STAT_ACTIVE_STREAM:
			result=stat_active_stream_count(sender->core_reliable_space);
			break;
		default:
			printf("unkonwn stat type\n");
			break;
	}
	return result;
}


