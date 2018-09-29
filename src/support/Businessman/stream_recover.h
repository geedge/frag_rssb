#ifndef _BIZMAN_STREAM_RECOVER_H_INCLUDE_
#define _BIZMAN_STREAM_RECOVER_H_INCLUDE_
#include "list.h"
#include "hash.h"
#include "businessman_packet.h"
typedef struct _businessman_stream_recover_space_t
{
	hash_tab_t  stream_hash;
	queue_head_t recover_ready;
	queue_head_t recover_timeout;
}recover_space_t;
recover_space_t* get_stream_recover_space(void);
int add_block_to_stream(recover_space_t* recover_space,bizman_packet_t* data_block);
int get_ready_stream();
#endif //_BIZMAN_STREAM_RECOVER_H_INCLUDE_

