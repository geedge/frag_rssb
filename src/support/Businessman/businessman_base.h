#ifndef _BIZMAN_BASE_H_INCLUDE_
#define _BIZMAN_BASE_H_INCLUDE_
#include "businessman_memory.h"
#include "businessman_limit.h"
#include "businessman_packet.h"
typedef struct _businessman_external_send_task{
	uint32 *dest_ip;
	uint16 *dest_port;
	uint32 dest_num;
	uint64 app_id;
	const char* data;
	int len;
	uint16 flags;
}external_send_task_t;
typedef struct _businessman_internal_send_task{
	uint32 *dest_ip;
	uint16 *dest_port;
	uint32 dest_num;
	uint32 stream_id;
	uint32 *sub_stream_id;
	send_data_refer_t* data;
	int is_reliable;
	uint16 flags;
}internal_send_task_t;
typedef struct _businessman_sub_task{
	uint32 dest_ip;
	uint16 dest_port;
	uint32 stream_id;
	send_data_refer_t* refer_index;
	int is_reliable;
	uint16 flags;
}sub_send_task_t;
#endif //_BIZMAN_BASE_H_INCLUDE_

