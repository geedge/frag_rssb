#ifndef _BIZMAN_SMOOTH_SEND_H_INCLUDE_
#define _BIZMAN_SMOOTH_SEND_H_INCLUDE_
#include "hash.h"
#include "list.h"
#include "businessman_packet.h"
#include "businessman_lock.h"
#include "businessman_time.h"
#include "businessman_base.h"

typedef struct _bizman_smooth_block{
//	uint64 app_id;
	unsigned int stream_id;
	char is_reliable;
	list_index_t hook;
	uint32 buff_size;
	uint32 used_size;
	time_tick_t first_filled_tick;
	char* buff;
	void* father_stream;
}smooth_block_t;
typedef struct _bizman_smooth_stream{
	unsigned int already_smoothed_bytes;
	smooth_block_t *smooth_block;
	unsigned int stream_id;//bizman generate
	unsigned int *sub_stream_id;//for multi cast
	uint64	app_id;
	char is_reliable;
	uint32 *dest_ip;
	uint16 *dest_port;
	uint32 dest_num;
	time_tick_t last_edit_tick;
	unsigned long long active_block_num;
}smooth_stream_t;
typedef struct _bizman_smooth_space{
	//int is_active;
	bizman_lock_t thread_lock;
//	hash_tab_t app_hash;
	hash_tab_t stream_hash;
	time_tick_t last_expire_time;
	queue_head_t smooth_timeout_queue;
	queue_head_t smooth_ready_queue;
	time_tick_t	smooth_timeout_tick;
	unsigned long long stat_dropped_pkts_num;
	signed long long cache_smooth_pkts_limit;
}smooth_space_t;

smooth_space_t* create_smooth_send_space();
void destroy_smooth_send_space(smooth_space_t* smooth_space);
/*return 0 if success,return -1 when failed*/
int smooth_block(smooth_space_t* thread_smooth_space,const external_send_task_t* send_task);

/*return 1 if success,return 0 when nothing ready,return -1 when error ocurrs*/
int get_smoothed_task(smooth_space_t * thread_smooth_space,internal_send_task_t * task);
#endif //_BIZMAN_SMOOTH_SEND_H_INCLUDE_
