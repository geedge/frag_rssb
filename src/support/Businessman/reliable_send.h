#ifndef _BIZMAN_RELIABLE_SEND_H_INCLUDE_
#define _BIZMAN_RELIABLE_SEND_H_INCLUDE_
#include "list.h"
#include "hash.h"
#include "businessman_time.h"
#include "businessman_packet.h"
#include "businessman_base.h"
#include "businessman_limit.h"
//const unsigned int TIMEOUT_QUEUE_NUM=MAX_RESEND_TIMES;
enum PANE_STATE
{
	PANE_STATE_SPARE=0,
	PANE_STATE_WAIT_SEND,
	PANE_STATE_TIME_OUT	
};

typedef struct _businessman_send_pane_t{
	int		order;
	uint32	dest_ip;
	uint16	dest_port;
	int is_relaible;
	time_tick_t last_send_time;
	unsigned int	send_times;
	void* father_stream;
	send_data_index_t* index_data;
	PANE_STATE state;
	list_index_t hook;
}stream_send_pane_t;
typedef struct _businessman_send_stream_t{
	unsigned int stream_id;
	uint32	dest_ip;
	uint16	dest_port;
	int is_relaible;
	queue_head_t	data_queue;
	unsigned int in_use_task_num;
	uint32 bigger_sequence; //grow when new data joined,new data use this as its sequence
	stream_send_pane_t *task;
//	list_index_t hook;
	time_tick_t last_edit_tick;
	unsigned int is_unreachable;//destination unreachable;
	time_tick_t forzen_time_stamp;
	unsigned int send_window_size;
}send_stream_t;
typedef struct _bizman_reliable_send_space_t{
	hash_tab_t stream_hash;
	queue_head_t to_send_queue;
	queue_head_t	timeout_queue[MAX_RESEND_TIMES];
	time_tick_t last_expire_time;
	unsigned long long stat_send_packet_count[MAX_RESEND_TIMES+1];
	unsigned long long stat_outwindow_ack;
	unsigned long long stat_ack_pkts_cnt;
	unsigned long long stat_cached_packet_num;
	unsigned long long cached_pkts_num_limit;
	unsigned long long stat_dropped_pkts_num;
	unsigned long long stat_frozen_pkts_num;
	unsigned long long stat_send_stream_num;
	unsigned long long stat_resend_stream_num;
	//parameter
	unsigned int send_widow_size;
	unsigned int resend_times;
	time_tick_t	resend_base_tick;
	time_tick_t frozen_stream_interval;
	

}reliable_send_space_t;


reliable_send_space_t* create_reliable_send_space();
void destroy_reliable_send_space(reliable_send_space_t* reliable_send_space);
/*return 0 when success,return -1 when failed*/
int join_to_reliable_send(reliable_send_space_t*reliable_space,sub_send_task_t* smoothed_task);

/*return a  transition handle(not null) when success ,return NULL when failed*/
void *get_reliable_send_task(reliable_send_space_t*reliable_space,sub_send_task_t*send_task);

void recycle_transition_handle(reliable_send_space_t*reliable_space,void *p);
/*handle a received ack;return 0 when success,return -1 when failed*/
int handle_ack(reliable_send_space_t*reliable_space,bizman_packet_t* ack_packet);
unsigned long long stat_first_resend_packet_count(reliable_send_space_t*reliable_space);
unsigned long long stat_first_send_packet_count(reliable_send_space_t*reliable_space);
unsigned long long stat_last_resend_packet_count(reliable_send_space_t*reliable_space);
unsigned long long stat_dropped_packet_count(reliable_send_space_t*reliable_space);
unsigned long long stat_frozen_packet_count(reliable_send_space_t*reliable_space);
unsigned long long stat_frist_send_stream_count(reliable_send_space_t*reliable_space);
unsigned long long stat_frist_resend_stream_count(reliable_send_space_t*reliable_space);
unsigned long long stat_active_stream_count(reliable_send_space_t*reliable_space);
unsigned long long  stat_ack_packet_count(reliable_send_space_t*reliable_space);

int set_send_cached_pkts_limit(reliable_send_space_t*reliable_space,unsigned long long max_cached);
#endif //_BIZMAN_RELIABLE_SEND_H_INCLUDE_


