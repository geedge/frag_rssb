#ifndef _BIZMAN_RELIABLE_RECV_H_INCLUDE_
#define _BIZMAN_RELIABLE_RECV_H_INCLUDE_
#include "list.h"
#include "hash.h"
#include "businessman_lock.h"
#include "businessman_time.h"
#include "businessman_packet.h"
#include "businessman_base.h"
typedef struct _businessman_recv_data_index_t{
	int len;
	bizman_packet_t *packet;
	list_index_t hook;
	uint32 src_ip;
	uint16 src_port;
}recv_data_index_t;
typedef struct _businessman_relaible_recv_lattice_t{
	int state;
	uint32 sequence;
	recv_data_index_t *data_index;
}stream_recv_lattice_t;
typedef struct _businessman_recv_ack_task_t{
	unsigned int dest_ip;
	unsigned short dest_port;
	uint32 stream_id;
	uint32 sequence;
	uint8 flags;
    void* father_stream;
	list_index_t hook;

}ack_task_t;
typedef struct _businessman_reliable_recv_record_t{
	unsigned int stream_id;
	time_tick_t record_create_tick;
	time_tick_t last_edit_tick;
	uint32 max_ordered_seq;
	uint32 latest_ack_seq;
	unsigned int ordered_unack_num;	
	stream_recv_lattice_t *ruler;
	queue_head_t orderd_data_queue;
	
	bizman_lock_t lock_ready_operate; //protect in queue and out queue operation
	int is_in_ready_queue;
	list_index_t ready_hook;//hook to ready stream index queue
	
	int is_in_ack_timeout_queue;
	list_index_t ack_timeout_hook;
	ack_task_t *timeout_ack; //use stream's hook to index timeout_ack

	int read_offset;//only access by user app thread
	int chunk_num;//un handled chunk_num;
	int is_recv_complete;//received every packet of the stream
	int is_read_complete;//read every packet of the stream,its empty already.
	unsigned int recv_window_size;
}recv_stream_record_t;

typedef struct _businessman_reliable_recv_space_t
{
	hash_tab_t stream_hash;
	queue_head_t to_ack_queue;
	queue_head_t ack_timeout_queue;
	queue_head_t ready_stream_index_queue; //only user app thread can get head from this queue
	unsigned int chunk_mode;
	unsigned int choke_mode;
	unsigned int choke_timeout_mili_seconds;
	bizman_lock_t mutexlock_ready_stream_index_queue;
	bizman_cond_t cond_have_data_to_read;
	time_tick_t last_expire_time;
	//parameter
	unsigned int receive_window_size;
	unsigned int accumulate_ack_num;
	time_tick_t	accumulate_ack_timeout_tick;
	//time_tick_t	ack_smooth_timeout_tick;
}reliable_recv_space_t;
reliable_recv_space_t* create_reliable_recv_space(void);
void destroy_reliable_recv_space(reliable_recv_space_t* reliable_recv_space);
int add_raw_packet(reliable_recv_space_t* reliable_recv_handle,unsigned int src_ip,unsigned short src_port,bizman_packet_t* packet_data,int packet_len);

int copy_recv_data(reliable_recv_space_t* reliable_recv_handle,char* buf,int buf_len,unsigned int *src_ip,unsigned short *src_port,unsigned int *stream_id,unsigned int* is_complete);
/*return ack packet length(>0) when success
 return -1 when failed
 return 0 when no ack task left*/
int get_ack_task(reliable_recv_space_t* reliable_recv_handle,ack_task_t* send_ack_task);
#endif //_BIZMAN_RELIABLE_RECV_H_INCLUDE_

