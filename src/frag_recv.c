#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <net/if.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>

#include "MESA_handle_logger.h"

#include "main.h"
#include "AV_interface.h"
#include "frag_reassembly.h"
#include "frag_recv.h"
#include "my_socket.h"
#include "bizman.h"
#include "log.h"
#include "frag_voip.h"
#include "wiredLB.h"
#include "service.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;

void heart_beat(unsigned int thread_id,char *buf, uint32_t size, uint32_t src_ip)
{
	msg_header_t *mh = (msg_header_t*)buf;
	mh->msg_type = MSG_LIVE_RESPONS;
	bizman_send(g_frag_run.answer_sapp_bizman, thread_id, src_ip,  g_frag_cfg.qd_msg_port,  (const char*)buf, size, 1, BIZMAN_SMOOTH_DEST|BIZMAN_PUSH_SEND);
}

void proc_data_msg(uint32_t thread_id, uint32_t src_ip, char* packet, int size)
{
	msg_header_t*	mhead = (msg_header_t*)packet;
	msg_metainfo_t*	minfo = (msg_metainfo_t*)(packet + MSG_HEADER_LEN);
	msg_data_t*		mdata = (msg_data_t*)(packet + MSG_HEADER_LEN);
	char*			data = (char*)mdata + MSG_DATA_HEAD_LEN;	
	char*			opt_ptr = NULL;
	
	/* Ignore invalid packet */
	if(PROTO_MAGICNUM != mhead->magic_num || PROTO_VERSION != mhead->version|| size <= (int)MSG_HEADER_LEN || size>1600)
	{
		atomic_inc(&g_frag_stat.stat_info[INVALID_RECV][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[INVALID_RECV][TOTAL_BYTES],size);
		return;
	}	
	/*prco msg*/
	switch(mhead->msg_type)
	{
		case MSG_DATA_META:
			/*write log*/
			if(RLOG_LV_INFO>=g_frag_run.frag_loglevel)
			{
				int 	buf_len = 32;
				char	pbuf[32] = {0};
				char	qbuf[32] = {0};
				inet_ntop(AF_INET, &src_ip, pbuf, buf_len);
				inet_ntop(AF_INET, &minfo->cap_IP, qbuf, buf_len);
				MESA_handle_runtime_log(g_frag_run.frag_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    			"{%s:%d} %s:%hd RECV META [PID:%llu, media_len:%llu, media_type:0x%02x, cap_IP:%s]", 
						    			__FILE__,__LINE__, pbuf, g_frag_cfg.bizman_port, *(uint64_t*)minfo->prog_id, minfo->prog_len, minfo->media_type, qbuf);
			} 
			/*avoid uncomplete chunk*/
			if(mhead->cont_len-MSG_HEADER_LEN>2000)	return;
			/*for frag_reassembly*/
			opt_ptr = packet + MSG_HEADER_LEN + sizeof(msg_metainfo_t);
			if(g_frag_cfg.all_hit_monitor_switch==ALL_HIT_MONITOR_SWITCH)
			{
#if K_PROJECT
				minfo->hitservice = 0x181;
#else
				minfo->hitservice = 0x81;
#endif
				minfo->data_flag = 4;
			}			
			add_media_info(minfo, opt_ptr, src_ip, thread_id);			
			atomic_inc(&g_frag_stat.stat_info[META_RECV][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.stat_info[META_RECV][TOTAL_BYTES],size);
			break;
			
		case MSG_DATA_BODY:	
		case MSG_DATA_TAIL:			
			/*write log*/		
			if(RLOG_LV_DEBUG>=g_frag_run.frag_loglevel)
			{
				int 	buf_len = 32;
				char	pbuf[32] = {0};
				inet_ntop(AF_INET, &src_ip, pbuf, buf_len);
				/*打印VOIP数据中的SEQ*/
				if(g_frag_cfg.cpz_type==CPZ_VOIP)
				{
					int rtp_seq = ntohl(*(int*)(data+VOIP_DATA_SEQ_OFFSET));
					MESA_handle_runtime_log(g_frag_run.frag_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
						    				"{%s:%d} %s:%hd RECV DATA [PID:%llu, offset:%llu, rtp_seq:%d, datalen:%u]", 
						    				__FILE__,__LINE__, pbuf, g_frag_cfg.bizman_port, *(uint64_t*)mdata->prog_id, mdata->offset, rtp_seq, mhead->cont_len-MSG_DATA_HEAD_LEN);
				}
				else
				{
					MESA_handle_runtime_log(g_frag_run.frag_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
							    			"{%s:%d} %s:%hd RECV DATA [PID:%llu, offset:%llu, datalen:%u]", 
						   	 				__FILE__,__LINE__, pbuf, g_frag_cfg.bizman_port, *(uint64_t*)mdata->prog_id, mdata->offset, mhead->cont_len-MSG_DATA_HEAD_LEN);
				}
			} 		
			/*??? protocol lost debug for uncomplete chunk*/
			if(mhead->cont_len-MSG_DATA_HEAD_LEN>2000)	return;
			if(mdata->offset>1000000000000)	return;
			/*for frag_reassembly*/			
			add_frag(*(uint64_t*)mdata->prog_id,mdata->offset,data,mhead->cont_len-MSG_DATA_HEAD_LEN,0,src_ip,thread_id);
			atomic_inc(&g_frag_stat.stat_info[DATA_RECV][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.stat_info[DATA_RECV][TOTAL_BYTES],size);
			break;

		case MSG_LIVE_CHECK:			
			heart_beat(thread_id, packet, size, src_ip);
			break;
			
		default:
			atomic_inc(&g_frag_stat.stat_info[OTHER_RECV][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.stat_info[OTHER_RECV][TOTAL_BYTES],size);
			
			return;
	}
}

void free_recv_data(bizman_recv_data_t* recv_data)
{		
	if(NULL!=recv_data)
	{			
		if(NULL!=recv_data->data)
		{
			free(recv_data->data);
			recv_data->data = NULL;
		}				
		free(recv_data);			
	}
}

void* bizman_recv_data_from_queue(void *param)
{
	long				tid = (long)param;	
	bizman_recv_data_t*	recv_data = NULL;
	long 				recv_data_size = sizeof(recv_data);
	
	while(1)
	{			
		recv_data = NULL;
		int rec = MESA_lqueue_get_head(g_frag_run.recv_bizman_lq[tid], &recv_data, &recv_data_size);
		if (rec<0)
		{	
			usleep(10);
			continue;
		}	
		else
		{
			if(NULL!=recv_data && NULL!=recv_data->data)
			{					
				/*get from queue stat */							
				atomic_inc(&g_frag_stat.sysinfo_stat[BIZMAN_RECV_QUEUE][QUEUE_OUT]);
				proc_data_msg(tid, recv_data->src_ip, recv_data->data, recv_data->size);				
			}
			free_recv_data(recv_data);
		}
	}
	return NULL;
}

void* bizman_recv_data_to_queue(void *param)
{	
	char		buf[BIZMAN_RECV_BUFFSIZE] = {0};
	int			size = 0;
	uint32_t	src_ip = 0;
	uint16_t	src_port = 0;
	uint32_t 	is_complete = 0, stream_id = 0;
	long		tid = (long)param;	
 	int 		lq_rec = 0;
	bizman_recv_data_t* recv_data = NULL;
	
	while(1)
	{		
		size = bizman_recv(g_frag_run.recv_bizman[tid],(char*)buf,sizeof(buf),&src_ip,&src_port,&stream_id,&is_complete);
		if(is_complete&BIZMAN_READ_CHUNK && size>0)
		{
			
			if(g_frag_stat.sysinfo_stat[BIZMAN_RECV_QUEUE][QUEUE_CURRENT]<g_frag_cfg.bizman_queue_maxnum)
			{
				recv_data = (bizman_recv_data_t*)calloc(1, sizeof(bizman_recv_data_t));
				recv_data->size = size;
				recv_data->src_ip = src_ip;			
				recv_data->data = (char*)malloc(size);
				memcpy(recv_data->data, buf, size);			
				tid = recv_data->data[15]%g_frag_cfg.thread_num;				
				lq_rec = MESA_lqueue_join_tail(g_frag_run.recv_bizman_lq[tid], &recv_data, sizeof(recv_data));				
				if(lq_rec==MESA_QUEUE_RET_OK)
				{
					atomic_inc(&g_frag_stat.sysinfo_stat[BIZMAN_RECV_QUEUE][QUEUE_IN]); 
				}
				else
				{
					free_recv_data(recv_data);
					atomic_inc(&g_frag_stat.stat_info[RECV_DROP][TOTAL_PKTS]);
					atomic_add(&g_frag_stat.stat_info[RECV_DROP][TOTAL_BYTES], size);
				}
			}	
			else
			{
				atomic_inc(&g_frag_stat.stat_info[RECV_DROP][TOTAL_PKTS]);
				atomic_add(&g_frag_stat.stat_info[RECV_DROP][TOTAL_BYTES], size);
			
			}
			atomic_inc(&g_frag_stat.stat_info[RECV][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.stat_info[RECV][TOTAL_BYTES], size);
		}
	}
	return NULL;
}

void* bizman_recv_data(void *param)
{
	long		tid = (long)param;	
	char		buf[BIZMAN_RECV_BUFFSIZE] = {0};
	int			size = 0;
	uint32_t	src_ip = 0;
	uint16_t	src_port = 0;
	uint32_t 	is_complete = 0, stream_id = 0;
	
	while(1)
	{	
		size = bizman_recv(g_frag_run.recv_bizman[tid],(char*)buf,sizeof(buf),&src_ip,&src_port,&stream_id,&is_complete);
		if(is_complete&BIZMAN_READ_CHUNK && size>0)
		{
			proc_data_msg(tid, src_ip, buf, size);
			atomic_inc(&g_frag_stat.stat_info[RECV][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.stat_info[RECV][TOTAL_BYTES], size);			
		}		
	}
	return NULL;
}

void* wlb_report(void *param)
{
	uint64_t start_bytes = g_frag_stat.stat_info[RECV][TOTAL_BYTES];
	uint64_t start_pkts = g_frag_stat.stat_info[RECV][TOTAL_PKTS];
	while(1)
	{
		wiredLB_report(g_frag_cfg.rssb_wlb_handle, g_frag_stat.stat_info[RECV][TOTAL_BYTES] - start_bytes,g_frag_stat.stat_info[RECV][TOTAL_PKTS]- start_pkts, "I am fine!");
		start_bytes = g_frag_stat.stat_info[RECV][TOTAL_BYTES];
		start_pkts = g_frag_stat.stat_info[RECV][TOTAL_PKTS];		
		sleep(g_frag_cfg.wlb_report_interval);					
	}
	return NULL;	
}

