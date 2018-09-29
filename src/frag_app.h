#ifndef _FRAG_APP_H
#define _FRAG_APP_H

#include "frag_reassembly_in.h"

typedef struct __appdtc_detail_in_s
{
	char *tuple4;
	char *user_agent;
	char *url;
	void *app_data;
	unsigned int tuple4_len;
	unsigned short app_type; //APP_TYPE_t
	unsigned int ua_len;
	unsigned int url_len;
	unsigned long long data_len;
	unsigned long long appid;	
	MESA_lqueue_head app_frg_lq;						//appÊý¾Ý»º´æ
}appdtc_detail_in_t;

#ifdef __cplusplus
extern "C" {
#endif

void app_change_pid(frag_unit_t* frg_unit);
void proc_app_data(media_t* mdi);
void* thread_send_app_data(void *param);

#ifdef __cplusplus
}
#endif


#endif
