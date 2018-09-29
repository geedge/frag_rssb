#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <sys/un.h>
#include </usr/include/stdint.h>

/*just for hash callback in proc_resp_checkresult */
typedef struct temp_buf_s
{
	char* 					buf;
	uint32_t 				size;
}temp_buf_t;


typedef struct monitor_hash_node_s
{
	uint64_t 				porg_id;
	int 					hit_service;
}monitor_hash_node_t;


#ifdef __cplusplus
extern "C" {
#endif

void* 	recv_response_msg(void *param);
void 	free_monitor_hash_node(void* data);
int 	expire_monitor_hash_node(void *data, int eliminate_type);


#ifdef __cplusplus
}
#endif

#endif
