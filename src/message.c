#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <net/if.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>

#include "cJSON.h"
#include "MESA_handle_logger.h"
#include "AV_sendback_in.h"
#include "message.h"
#include "bizman.h"
#include "my_socket.h"
#include "AV_interface.h"
#include "AV_sendback.h"
#include "frag_reassembly_in.h"
#include "frag_voip.h"
#include "frag_proc.h"
#include "frag_json.h"
#include "log.h"


extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;
extern frag_reassembly_t 			frag_rssb;     //use media hash
extern const char* 					hash_eliminate_type[3];

void resp_write_to_log(int type, resp_checkresult_t* check_res, void* param1, void* param2, uint64_t param3)
{
	struct timeval	tv;
	struct timezone tz;
	char		url[2048] = {0};	
	char		substr[2048] = {0};	
	char		buf[2048] = {0};	
	int 		buflen = 0;
	time_t		cur_time;
	struct tm	now;
	char		now_time[32] = {0};
	char		day_time[32] = {0}; 
	char		filename[MAX_PATH_LEN] = {0};
	media_t* 	mdi = NULL;
	msg_prog_sync_t* sync_msg = NULL;	

	time(&cur_time);
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &now);	
	strftime(now_time, sizeof(now_time), "%Y-%m-%d %H:%M:%S", &now);

	/*init*/
	if(NULL==g_frag_run.resp_file)
	{
		strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);		
		snprintf(filename, sizeof(filename), "%s.%s", g_frag_cfg.resp_filename, day_time);
		g_frag_run.resp_file = fopen(filename, "a+");			
		g_frag_run.cur_time = cur_time;
		localtime_r(&g_frag_run.cur_time, &g_frag_run.resp_filetime);
	}
	
	if(now.tm_mday!=g_frag_run.resp_filetime.tm_mday || now.tm_mon!=g_frag_run.resp_filetime.tm_mon || now.tm_year!=g_frag_run.resp_filetime.tm_year)
	{
		if(g_frag_run.resp_file)
		{		
			strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);	
			fclose(g_frag_run.resp_file);					
			snprintf(filename, sizeof(filename), "%s.%s", g_frag_cfg.resp_filename, day_time);
			g_frag_run.resp_file = fopen(filename, "a+");			
		}
		g_frag_run.cur_time = cur_time;
		localtime_r(&g_frag_run.cur_time, &g_frag_run.resp_filetime);
	}

	switch(type)
	{
		case RECV_RESP:
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u\n",
							  now_time, tv.tv_usec,"recv survey",
							  *(uint64_t*)check_res->prog_id, check_res->service, check_res->cfg_id, check_res->level,
							  check_res->file_header_size, check_res->file_packet_size);
			break;

		case RECV_RESP_WHITELIST:
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u\n",
							  now_time, tv.tv_usec,"survey whitelist",
							  *(uint64_t*)check_res->prog_id, check_res->service, check_res->cfg_id, check_res->level,
							  check_res->file_header_size, check_res->file_packet_size);	
			break;

		case RESP_MEDIA_NOFOUND:
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u\n",
							  now_time, tv.tv_usec,"media nofound",
							  *(uint64_t*)check_res->prog_id, check_res->service, check_res->cfg_id, check_res->level,
							  check_res->file_header_size, check_res->file_packet_size);		
			break;

		case RESP_REPEAT:
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u\n",
							  now_time, tv.tv_usec,"survey repeat",
							  *(uint64_t*)check_res->prog_id, check_res->service, check_res->cfg_id, check_res->level,
							  check_res->file_header_size, check_res->file_packet_size);
			break;

		case RESP_AUDIO_LANG_MONITOR_NEW:
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u,set level=1\n",
							  now_time, tv.tv_usec,"new lang monitor",
							  *(uint64_t*)check_res->prog_id, check_res->service, check_res->cfg_id, check_res->level,
							  check_res->file_header_size, check_res->file_packet_size);
			break;

		case RESP_AUDIO_LANG_MONITOR_OLD:
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u,set level=2\n",
							  now_time, tv.tv_usec,"old lang monitor",
							  *(uint64_t*)check_res->prog_id, check_res->service, check_res->cfg_id, check_res->level,
							  check_res->file_header_size, check_res->file_packet_size);
			break;

		case RESP_SEND_BLOCK:
			mdi = (media_t*)param2;
			if(NULL!=mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index])
			{
				memcpy(url, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value, MIN(sizeof(url)-1, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len));
				memcpy(substr, mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index]->opt_value, MIN(sizeof(substr)-1, mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index]->opt_len));
				buflen = snprintf(buf, sizeof(buf), 
								  "%s.%ld,%20s,MID:%" PRIu64 ",media_type:0x%02x,service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u,capIP:%s,qdmid:%" PRIu64 ",substr:%s,url:%s\n",
								  now_time, tv.tv_usec,"send block",
								  *(uint64_t*)check_res->prog_id, mdi->media_type,check_res->service, check_res->cfg_id, check_res->level,
								  check_res->file_header_size, check_res->file_packet_size, (char*)param1, param3, substr, url);
			}
			else
			{			
				buflen = snprintf(buf, sizeof(buf), 
							 	 "%s.%ld,%20s,MID:%" PRIu64 ",media_type:0x%02x,service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u,capIP:%s,qdmid:%" PRIu64 "\n",
								  now_time, tv.tv_usec,"send block",
								  *(uint64_t*)check_res->prog_id, mdi->media_type,check_res->service, check_res->cfg_id, check_res->level,
								  check_res->file_header_size, check_res->file_packet_size, (char*)param1, param3);
			}
			break;
			
		case RESP_SEND_BLOCK_MULTI:
			mdi = (media_t*)param2;
			if(NULL!=mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index])
			{
				memcpy(url, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value, MIN(sizeof(url)-1, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len));
				memcpy(substr, mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index]->opt_value, MIN(sizeof(substr)-1, mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index]->opt_len));
				buflen = snprintf(buf, sizeof(buf), 
								  "%s.%ld,%20s,MID:%" PRIu64 ",media_type:0x%02x,service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u,capIP:%s,qdmid:%" PRIu64 ",substr:%s,url:%s\n",
								  now_time, tv.tv_usec,"send block_multisrc",
								  *(uint64_t*)check_res->prog_id, mdi->media_type,check_res->service, check_res->cfg_id, check_res->level,
								  check_res->file_header_size, check_res->file_packet_size, (char*)param1, param3, substr, url);
			}
			else
			{			
				buflen = snprintf(buf, sizeof(buf), 
							 	 "%s.%ld,%20s,MID:%" PRIu64 ",media_type:0x%02x,service:%d,configID:%u,level:%d,file_header_size:%u,file_packet_size:%u,capIP:%s,qdmid:%" PRIu64 "\n",
								  now_time, tv.tv_usec,"send block_multisrc",
								  *(uint64_t*)check_res->prog_id, mdi->media_type, check_res->service, check_res->cfg_id, check_res->level,
								  check_res->file_header_size, check_res->file_packet_size, (char*)param1, param3);
			}
			break;

		case SEND_CONFIG_MONITOR:			
			mdi = (media_t*)param1;			
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,ip:%s,DataURL:%s\n",
							  now_time, tv.tv_usec,"send_config_monitor",
							  mdi->mid, mdi->hit_service, (char*)param2, mdi->monitor_path);
			
			break;	

		case SEND_LANG_MONITOR:
			mdi = (media_t*)param1;
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,ip:%s\n",
							  now_time, tv.tv_usec,"send_lang_monitor",
							  mdi->mid,SERVICE_AUDIO_LANG_FULL, (char*)param2);
			break;

		case RECV_PROG_SYNC:
			sync_msg = (msg_prog_sync_t*)param1;
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",sender:0x%2x,action:0x%2x\n",
							  now_time, tv.tv_usec,"recv_prog_sync",
							  *(uint64_t*)sync_msg->prog_id,sync_msg->sender, sync_msg->action);	
			break;

		case RECV_PROG_SYNC_NOFOUND:
			sync_msg = (msg_prog_sync_t*)param1;
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",sender:0x%2x,action:0x%2x\n",
							  now_time, tv.tv_usec,"sync_prog_nofound",
							  *(uint64_t*)sync_msg->prog_id,sync_msg->sender, sync_msg->action);	
			break;

		case RECV_PROG_SYNC_AUDIO_STOP:
			sync_msg = (msg_prog_sync_t*)param1;
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",sender:0x%2x,action:0x%2x\n",
							  now_time, tv.tv_usec,"sync_audio",
							  *(uint64_t*)sync_msg->prog_id,sync_msg->sender, sync_msg->action);	
			break;

		case RECV_PROG_SYNC_VIDEO_STOP:
			sync_msg = (msg_prog_sync_t*)param1;
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",sender:0x%2x,action:0x%2x\n",
							  now_time, tv.tv_usec,"sync_video",
							  *(uint64_t*)sync_msg->prog_id,sync_msg->sender, sync_msg->action);	
			break;

		case RECV_PROG_SYNC_UNKNOW:
			sync_msg = (msg_prog_sync_t*)param1;
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",sender:0x%2x,action:0x%2x\n",
							  now_time, tv.tv_usec, "prog_sync_unkonw",
							  *(uint64_t*)sync_msg->prog_id,sync_msg->sender, sync_msg->action);	
			break;
			
		default:
			break;			
				
	}
	
	if(g_frag_run.resp_file)
	{
		fwrite(buf, buflen, 1, g_frag_run.resp_file);
		fflush(g_frag_run.resp_file);
	}
}

void free_monitor_hash_node(void* data)
{
	monitor_hash_node_t* monitor_hnode = (monitor_hash_node_t*)data;
	if(NULL!=monitor_hnode)
	{
		free(monitor_hnode);
	}
}

int expire_monitor_hash_node(void *data, int eliminate_type)
{
	monitor_hash_node_t* monitor_hnode = (monitor_hash_node_t*)data;
	switch(eliminate_type)
	{
		case ELIMINATE_TYPE_NUM:			
			g_frag_stat.sysinfo_stat[MONITOR_HASH][HASH_NUM_EXPIRE]++;	
			break;
		case ELIMINATE_TYPE_TIME:
			g_frag_stat.sysinfo_stat[MONITOR_HASH][HASH_TIME_EXPIRE]++;	
			break;
		default:
			break;

	}
	MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} expire_monitor_hash_node %s: [MID: %llu]", 
							__FILE__,__LINE__, 
							hash_eliminate_type[eliminate_type],
							monitor_hnode->porg_id);	
	return 1;
}

void resp_checkresult_json_report(media_t* mdi, resp_checkresult_t* check_res)
{	
	cJSON* 		root;		
	cJSON* 		ip_array = NULL;
	cJSON* 		ip_object = NULL;
	char* 		out = NULL;	
	uint32_t	out_len = 0;
	uint32_t 	ip = 0;
	char		pbuf[32] = {0}; 
	int 		buf_len = 32;
	char		filename[32] = {0};
	FILE*       fp = NULL;
	string	topic_name;

	if(mdi->proto==AV_PROTOCOL_SIP)	
	{
		topic_name = TOPIC_VOIP_SURVEY_JSON;
	}
	else
	{
		topic_name = TOPIC_SURVEY_JSON;
	}

	root = cJSON_CreateObject();

	/*mid*/
	cJSON_AddNumberToObject(root, "mid", mdi->mid);

	/*cfg_id*/
	cJSON_AddNumberToObject(root, "cfg_id", check_res->cfg_id);	

	/*service*/
	cJSON_AddNumberToObject(root, "service", check_res->service);	

	/*level*/
	cJSON_AddNumberToObject(root, "level", check_res->level);	

	/*recv time*/	
	cJSON_AddNumberToObject(root, "recv_time", time(NULL));

	/*cpz_ip*/
	memset(pbuf, 0, sizeof(pbuf));
	inet_ntop(AF_INET, &g_frag_cfg.local_ip_nr, pbuf, buf_len); 		
	cJSON_AddStringToObject(root, "cpz_ip", pbuf);

	/*cap_ip*/	
	ip_array = cJSON_CreateArray();
	for(int i=0;i<QD_MAXNUM;i++)
	{
		ip = mdi->qd_info[i].cap_ip;
		if(!ip)
		{
			continue;
		}
		memset(pbuf, 0, sizeof(pbuf));
		inet_ntop(AF_INET, &ip, pbuf, buf_len); 		
		cJSON_AddItemToArray(ip_array, ip_object=cJSON_CreateObject());
		cJSON_AddStringToObject(ip_object, "cap_ip", pbuf);			
	}	
	cJSON_AddItemToObject(root, "cap_ip", ip_array);

	/*多源条件cap_ip*/
	ip_array = cJSON_CreateArray();
	for(int i=0;i<QD_MAXNUM;i++)
	{
		ip = mdi->qd_info_from_cpz[i].cap_ip;
		if(!ip)
		{
			continue;
		}
		memset(pbuf, 0, sizeof(pbuf));
		inet_ntop(AF_INET, &ip, pbuf, buf_len); 		
		cJSON_AddItemToArray(ip_array, ip_object=cJSON_CreateObject());
		cJSON_AddStringToObject(ip_object, "cap_ip", pbuf);			
	}	
	cJSON_AddItemToObject(root, "cap_ip", ip_array);

	out = cJSON_Print(root);
	out_len = strlen(out);
	
	int cb_ret = g_frag_run.kafka_producer->SendData(topic_name, (void *)out, (size_t)out_len);
	if(0!=cb_ret)
	{
		/*fail output*/					
		atomic_inc(&g_frag_stat.stat_info[SURVEY_JSON][FAIL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[SURVEY_JSON][FAIL_BYTES], out_len);	
	}
	else
	{
		/*succ output*/
		atomic_inc(&g_frag_stat.stat_info[SURVEY_JSON][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[SURVEY_JSON][TOTAL_BYTES], out_len);						
	}
	if(g_frag_cfg.json_local_switch)
	{
		snprintf(filename, sizeof(filename), "%s", "./log/survey.json");
		if(NULL!=(fp=fopen(filename,"a+")))
		{
			fwrite(out, strlen(out), 1, fp);
			fclose(fp);
		}
	}
	cJSON_Delete(root);
	if(NULL!=out)
	{
		free(out);
	}
}

void send_json_log(media_t* mdi, resp_checkresult_t* check_res)
{
	if((NULL == mdi)||(mdi->proto != AV_PROTOCOL_SIP))
		return;

	cJSON*	root = NULL;

	string	topic_name = TOPIC_MM_SAMPLE_VOIP_LOG;
	char pid_buf[64] = {0};
	char pbuf[32] = {0}; 

	char* outbuf = NULL;
	int len = 0;

	root = cJSON_CreateObject();

	snprintf(pid_buf, sizeof(pid_buf), "%llu", mdi->mid);
	cJSON_AddStringToObject(root, "pid", pid_buf);
	cJSON_AddNumberToObject(root, "cfg_id", check_res->cfg_id);	
	cJSON_AddNumberToObject(root, "service", check_res->service);	
	cJSON_AddNumberToObject(root, "level", check_res->level);		
	cJSON_AddNumberToObject(root, "found_time", mdi->create_time);
	cJSON_AddNumberToObject(root, "recv_time", time(NULL));
	cJSON_AddNumberToObject(root, "fd_type", FD_TYPE_ANALYSE);
	cJSON_AddStringToObject(root, "voip_protocol", SIP_PROTO_RTP);

	memset(pbuf, 0, sizeof(pbuf));
	inet_ntop(AF_INET, &g_frag_cfg.local_ip_nr, pbuf, sizeof(pbuf)); 		
	cJSON_AddStringToObject(root, "cap_ip", pbuf);

	memset(pbuf, 0, sizeof(pbuf));
	snprintf(pbuf, sizeof(pbuf), "%" PRIu64 "", mdi->lastpkt_time - mdi->create_time);
	cJSON_AddStringToObject(root, "duration", pbuf);
	
	if((NULL != mdi->sip_opt[SIP_FROM_OPT_INDEX])&&(mdi->sip_opt[SIP_FROM_OPT_INDEX]->opt_len > 0)&&(NULL != mdi->sip_opt[SIP_FROM_OPT_INDEX]->opt_value))
	{
		cJSON_AddStringToObject(root, "calling_account", mdi->sip_opt[SIP_FROM_OPT_INDEX]->opt_value);	
	}
	
	if((NULL != mdi->sip_opt[SIP_TO_OPT_INDEX])&&(mdi->sip_opt[SIP_TO_OPT_INDEX]->opt_len > 0)&&(NULL != mdi->sip_opt[SIP_TO_OPT_INDEX]->opt_value))
	{
		cJSON_AddStringToObject(root, "called_account", mdi->sip_opt[SIP_TO_OPT_INDEX]->opt_value);		
	}
	
	char      	src_ip[32] = {0};
	char      	src_port[8] = {0};
	char      	dst_ip[32] = {0};
	char      	dst_port[8] = {0};
	char      	rtp_4tuple[128] = {0};

	if((NULL != mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX])&&(mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX]->opt_len > 0)&&(NULL != mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX]->opt_value))
	{	
		memcpy(rtp_4tuple, mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX]->opt_len, 127));
		memset(src_ip, 0, sizeof(src_ip));
		memset(src_port, 0, sizeof(src_port));
		memset(dst_ip, 0, sizeof(dst_ip));
		memset(dst_port, 0, sizeof(dst_port));
		if(0==parse_sip_4tuple(rtp_4tuple,src_ip,src_port,dst_ip,dst_port))
		{
			cJSON_AddStringToObject(root, "s_ip", src_ip);
			cJSON_AddStringToObject(root, "d_ip", dst_ip);
			cJSON_AddNumberToObject(root, "s_port", atoi(src_port));
			cJSON_AddNumberToObject(root, "d_port", atoi(dst_port));
			
			cJSON_AddNumberToObject(root, "addr_type",4);//TODO
		}
		
	}

	if(mdi->re_offset==1)//caller
	{	
		cJSON_AddNumberToObject(root, "stream_dir", 0);		
	}
	else if(mdi->re_offset==2)//callee
	{
		cJSON_AddNumberToObject(root, "stream_dir", 1);				
	}
	
	char* survey = NULL;
	uint32_t survey_len = 0;
	if(mdi->sip_survey_type&SIP_SURVEY_TYPE_FD)
	{
		survey = mdi->fd_buf;
		survey_len = mdi->fd_buflen;
	}				
	if(mdi->sip_survey_type&SIP_SURVEY_TYPE_JC)
	{
		survey = mdi->jc_buf;
		survey_len = mdi->jc_buflen;
	}
	char*	locate_url = NULL;
	uint32_t locate_urllen = survey_len-sizeof(msg_header_t)-sizeof(resp_checkresult_t);	
	char locate_ipbuf[64] = {0};
	char locate_urlbuf[1024] = {0};
	char*	locate_url_pos = NULL;	

	if(locate_urllen>0)
	{
		locate_url = survey + sizeof(msg_header_t) + sizeof(resp_checkresult_t);
		locate_url_pos = (char*)memchr(locate_url, ':', locate_urllen);
		if(NULL != locate_url_pos)
		{	
		
			memcpy(locate_ipbuf, locate_url, locate_url_pos-locate_url);
			memcpy(locate_urlbuf, locate_url_pos, locate_urllen-(locate_url_pos-locate_url+1));
			if(mdi->re_offset==2)
			{
				cJSON_AddStringToObject(root, "to_from_store_ip", locate_ipbuf);
				cJSON_AddStringToObject(root, "to_from_store_url", locate_urlbuf);
			}
			else
			{	
				cJSON_AddStringToObject(root, "from_to_store_ip", locate_ipbuf);
				cJSON_AddStringToObject(root, "from_to_store_url", locate_urlbuf);
			}
		}
	}

	/*
	trans_proto
	entrance_id
	device_id
	direction
	addr_list
	log_uri
	*/

	outbuf = cJSON_Print(root);
	len = strlen(outbuf);
	int cb_ret = 0;
	cb_ret = g_frag_run.kafka_producer->SendData(topic_name, (void *)outbuf, (size_t)len);
	if(cb_ret < 0)
	{
		MESA_handle_runtime_log(g_frag_run.voip_logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
										(char*)"[%s:%d] send_json_log fail." , __FILE__,__LINE__);	
	}
	else
	{
		MESA_handle_runtime_log(g_frag_run.voip_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} send_json_log : %s", __FILE__,__LINE__, outbuf);
	}
	free(outbuf);
	outbuf = NULL;
	cJSON_Delete(root);
	root = NULL;
}

long resp_prog_sync_search_media_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{
	media_t* 			mdi = (media_t*)data;		
	msg_prog_sync_t* 	sync_msg = (msg_prog_sync_t*)user_arg;

	/*send to log receiver and forward to dynamic list,to generate black list;*/
	if(NULL==mdi)
	{		
		g_frag_stat.stat_info[RESP_PROG_SYNC_IN_NOFOUND][TOTAL_PKTS]++;	
		g_frag_stat.stat_info[RESP_PROG_SYNC_IN_NOFOUND][TOTAL_BYTES] += MSG_HEADER_LEN + sizeof(msg_prog_sync_t);						
		resp_write_to_log(RECV_PROG_SYNC_NOFOUND, NULL, sync_msg, NULL,0);
		return 0;		
	}
	else
	{
		if(sync_msg->sender==SYNC_SENDER_VIDEO_ANALYZER && sync_msg->action==SYNC_ACTION_STOP_TO_SUBSYSTEM)
		{
			FLAG_SET(mdi->wins_dest_disabled_bit, AUDIO_WINS_DISABLE);
			resp_write_to_log(RECV_PROG_SYNC_VIDEO_STOP, NULL, sync_msg, NULL,0);
			g_frag_stat.stat_info[RESP_PROG_SYNC_VIDEO][TOTAL_PKTS]++;	
			g_frag_stat.stat_info[RESP_PROG_SYNC_VIDEO][TOTAL_BYTES] += MSG_HEADER_LEN + sizeof(msg_prog_sync_t);	
		}
		else if(sync_msg->sender==SYNC_SENDER_AUDIO_ANALYZER && sync_msg->action==SYNC_ACTION_STOP_TO_SUBSYSTEM)
		{
			FLAG_SET(mdi->wins_dest_disabled_bit, VEDIO_WINS_DISABLE);
			resp_write_to_log(RECV_PROG_SYNC_AUDIO_STOP, NULL, sync_msg, NULL,0);
			g_frag_stat.stat_info[RESP_PROG_SYNC_AUDIO][TOTAL_PKTS]++;	
			g_frag_stat.stat_info[RESP_PROG_SYNC_AUDIO][TOTAL_BYTES] += MSG_HEADER_LEN + sizeof(msg_prog_sync_t);	
		}
		else
		{
			resp_write_to_log(RECV_PROG_SYNC_UNKNOW, NULL, sync_msg, NULL,0);
		}
	}
	return 0;
}

int proc_prog_sync(char* buf, uint32_t size)
{	
	/* Ignore invalid packet */
	if(size < MSG_HEADER_LEN + sizeof(msg_prog_sync_t))
	{		
		g_frag_stat.stat_info[INVALID_RESP_PROG_SYNC][TOTAL_PKTS]++;
		g_frag_stat.stat_info[INVALID_RESP_PROG_SYNC][TOTAL_BYTES] += size;
		return(-1);
	}
	
	msg_prog_sync_t* sync_msg = (msg_prog_sync_t*)(buf + MSG_HEADER_LEN);
	long  rec_cb = 0;

	resp_write_to_log(RECV_PROG_SYNC, NULL, sync_msg, NULL,0);
	g_frag_stat.stat_info[RESP_PROG_SYNC][TOTAL_PKTS]++;
	g_frag_stat.stat_info[RESP_PROG_SYNC][TOTAL_BYTES] += size;

	MESA_htable_search_cb(g_frag_run.media_hash,(const uint8_t *)&sync_msg->prog_id,sizeof(sync_msg->prog_id),
						  resp_prog_sync_search_media_cb, (void*)sync_msg, &rec_cb);
	return 0;
}

long add_monitor_service_hash_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{
	media_t* 			mdi = (media_t*)data;
	resp_checkresult_t* check_res	= (resp_checkresult_t*)user_arg;	
	if(NULL!=mdi)
	{
		check_res->level = 2;	//already got full prog file,set harm level to 2 as a flag;		
		return check_res->level;
	}
	else
	{
		check_res->level = 1;
		monitor_hash_node_t* monitor_hnode = (monitor_hash_node_t*)calloc(1, sizeof(monitor_hash_node_t));
		monitor_hnode->hit_service = check_res->service;
		monitor_hnode->porg_id = *(uint64_t*)check_res->prog_id;
		if(0>(MESA_htable_add(g_frag_run.media_monitor_hash, key, size,(const void*)monitor_hnode)))
		{
			 MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									 "{%s:%d} proc_resp_checkresult blacklist, media_monitor_hash MESA_htable_add error: [MID: %llu]", 
									 __FILE__,__LINE__, monitor_hnode->porg_id);
			 free_monitor_hash_node(monitor_hnode);
			 monitor_hnode = NULL;
			 return -1;
		}		
		return check_res->level;
	}
}

int is_ts_end(char* url)
{
	uint32_t url_len = strlen(url);
	if(0==strcasecmp(url+url_len-strlen(".ts"), ".ts"))
	{
		return 1;
	}
	return 0;
}

char* HLS_fd_substr(char* url, char* substr, uint32_t* substr_len)
{
	char* pos = strcasestr(url, ".ts");
	char* pos_1 = NULL;		
	char  buf[2048] = {0};
	
	if(NULL!=pos)
	{
		if(is_ts_end(url))
		{
			/*数据文件是.ts结尾，管控特征是.ts的前一个/之前*/
			memcpy(buf, url, MIN(sizeof(buf)-1, pos-url));
			pos_1 = rindex(buf, '/');
			if(NULL!=pos_1)
			{
				*substr_len = pos_1-buf;
				return url;
			}				
		}
		else
		{
			/*数据文件不是.ts结尾，管控特征是.ts之前的数据*/
			*substr_len = pos-url;
			return url;			
		}
	}
	return NULL;
}

char* OSMF_fd_substr(char* url, char* substr, uint32_t* substr_len)
{
	char* pos = strcasestr(url, "seg");	
	if(NULL!=pos)
	{		
		*substr_len = pos-url;
		return url;			
	}
	return NULL;
}

int cal_fd_substr(media_t* mdi)
{
	char* 		url = NULL;
	char* 		substr = NULL;
	uint32_t 	substr_len = 0;
	uint32_t 	url_len = 0;
	
	if(NULL==mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index] &&  NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
	{	
		url_len	= mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len;
		url = (char*)malloc(url_len+1);
		memcpy(url, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value, url_len);
		url[url_len] = '\0';
		switch(mdi->media_type)
		{
			case FILE_OSMF:
				substr = OSMF_fd_substr(url, substr, &substr_len);	
				break;
				
			case FILE_HLS:
				substr = HLS_fd_substr(url, substr, &substr_len);			
				break;
				
			default:
				break;
		}
		if(NULL!=substr)
		{
			mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index] = (opt_in_t*)malloc(sizeof(opt_in_t));
			mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index]->opt_type = OPT_FRAG_CTRL_SUBSTR;
			mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index]->opt_len = substr_len;
			mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index]->opt_value = (char*)malloc(substr_len);
			memcpy(mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index]->opt_value, substr, substr_len);
		}
	}

	if(NULL!=mdi->opt[MEDIA_OPT_FD_SUBSTR][mdi->url_opt_index])
	{
		return 1;
	}
	return 0;
}

long resp_checkresult_search_media_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{
	media_t* 			mdi = (media_t*)data;		
	temp_buf_t* 		temp_buf = (temp_buf_t*)user_arg;
	char* 				buf = temp_buf->buf;
	uint32_t 			buflen = temp_buf->size;
	char* 				frag_survey_ptr = NULL;
	char* 				frag_survey_buf = NULL;
	uint32_t 			frag_survey_buflen = 0;
	char*				log_url = buf+sizeof(msg_header_t)+sizeof(resp_checkresult_t);
	uint32_t			log_urllen = buflen-sizeof(msg_header_t)-sizeof(resp_checkresult_t);
	resp_checkresult_t* check_res	= (resp_checkresult_t*)(buf + MSG_HEADER_LEN);			
	static uint64_t		last_prog_id	= 0;
	static uint32_t		last_cfg_id = 0;
	static time_t		last_black_ts = 0;
	long				rec_cb = 0;
	int					i = 0;
	uint32_t 			ip = 0;
	char				pbuf[32] = {0};	
	int 				buf_len = 32;

	/*send to log receiver and forward to dynamic list,to generate black list; delete the function*/
	if(NULL==mdi || (mdi->media_service_type==MEDIA_SERVICE_TYPE_FRAG && g_frag_cfg.frag_survey_invalid))
	{	
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_NOFOUND][TOTAL_PKTS]++;	
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_NOFOUND][TOTAL_BYTES] += buflen;						
		resp_write_to_log(RESP_MEDIA_NOFOUND, check_res, NULL, NULL, 0);
		return 0;			
	}

	/*repeat send from av_analyse */
	if(*(uint64_t*)check_res->prog_id == last_prog_id && check_res->cfg_id==last_cfg_id&& (time(NULL) - last_black_ts) < 10)
	{		
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_REPEAT][TOTAL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_REPEAT][TOTAL_BYTES] += buflen;		
		resp_write_to_log(RESP_REPEAT, check_res, NULL, NULL, 0);
		last_black_ts = time(NULL);
		return -1;
	}

	/*VOIP的监测建议，由粗拼装发*/
	if(mdi->proto==AV_PROTOCOL_SIP)
	{
		if((check_res->service & 0x80)==0x80)
		{
			mdi->sip_survey_type |= SIP_SURVEY_TYPE_JC;
			mdi->jc_buf = (char*)malloc(buflen);
			memcpy(mdi->jc_buf, buf, buflen);
			mdi->jc_buflen = buflen;			
			return 0;
		}
		else
		{
			mdi->sip_survey_type |= SIP_SURVEY_TYPE_FD;
			mdi->fd_buf = (char*)malloc(buflen);
			memcpy(mdi->fd_buf, buf, buflen);
			mdi->fd_buflen = buflen;			
		}
	}	
	
	/*monitor service*/
	if(check_res->service==SERVICE_AUDIO_LANG)
	{
		mdi->hit_service=SERVICE_AUDIO_LANG;
		MESA_htable_search_cb(g_frag_run.media_monitor_hash, 
							  (const unsigned char*)check_res->prog_id, 
							  sizeof(check_res->prog_id), 
							  add_monitor_service_hash_cb, 
							  check_res, &rec_cb);
		if(2==rec_cb)
		{
			g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_MONITOR_LEVEL2][TOTAL_PKTS]++; 
			g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_MONITOR_LEVEL2][TOTAL_BYTES] += buflen;			
			resp_write_to_log(RESP_AUDIO_LANG_MONITOR_OLD, check_res, NULL, NULL, 0);
		}
		else if(1==rec_cb)
		{
			g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_MONITOR_LEVEL1][TOTAL_PKTS]++; 
			g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_MONITOR_LEVEL1][TOTAL_BYTES] += buflen;			
			resp_write_to_log(RESP_AUDIO_LANG_MONITOR_NEW, check_res, NULL, NULL, 0);			
		}
	}	
	/*block service*/
	last_black_ts = time(NULL);
	last_prog_id = *(uint64_t*)check_res->prog_id;
	last_cfg_id	= check_res->cfg_id;

	/*图片的建议，set opt_num=0*/
	if(check_res->service!=SURVEY_PIC_TYPE && check_res->service!=SURVEY_PIC_MONITOR_TYPE && check_res->service!=SURVEY_PIC_KEYWORD_TYPE)
	{
		check_res->opt_num = 0;
	}

	/*碎片化FD封堵建议需要补充管控特征选项*/
	if(mdi->media_service_type==MEDIA_SERVICE_TYPE_FRAG)
	{
		g_frag_stat.stat_info[RESP_CHECKRESULT_FRAG_SURVEY][TOTAL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_FRAG_SURVEY][TOTAL_BYTES] += buflen;
	}	
	if(mdi->media_service_type==MEDIA_SERVICE_TYPE_FRAG && mdi->opt[MEDIA_OPT_INDEX_URL][mdi->url_opt_index]!=NULL)
	{
		/*碎片化管控V1版本: 子串管控*/   
		/*
		if(cal_fd_substr(mdi))
		{
			frag_survey_buflen = buflen+mdi->opt[MEDIA_OPT_URL]->opt_len+sizeof(uint32_t)+sizeof(uint8_t)+mdi->opt[MEDIA_OPT_FD_SUBSTR]->opt_len+sizeof(uint32_t)+sizeof(uint8_t);
			frag_survey_buf = (char*)malloc(frag_survey_buflen);
			frag_survey_ptr = frag_survey_buf;

			//选项数增加两个
			check_res->opt_num += 2;
			memcpy(frag_survey_ptr, buf, sizeof(msg_header_t)+sizeof(resp_checkresult_t));
			frag_survey_ptr += sizeof(msg_header_t)+sizeof(resp_checkresult_t);

			//OPT_FRAG_URL选项
			*(unsigned int *)frag_survey_ptr = mdi->opt[MEDIA_OPT_URL]->opt_len+sizeof(uint32_t)+sizeof(uint8_t);
			frag_survey_ptr += sizeof(unsigned int);
			*(unsigned char *)frag_survey_ptr = OPT_FRAG_ORIGIN_URL;
			frag_survey_ptr += sizeof(unsigned char);
			memcpy(frag_survey_ptr, mdi->opt[MEDIA_OPT_URL]->opt_value, mdi->opt[MEDIA_OPT_URL]->opt_len);		
			frag_survey_ptr += mdi->opt[MEDIA_OPT_URL]->opt_len;

			//OPT_FRAG_SUBSTR选项	
			*(unsigned int *)frag_survey_ptr = mdi->opt[MEDIA_OPT_FD_SUBSTR]->opt_len+sizeof(uint32_t)+sizeof(uint8_t);
			frag_survey_ptr += sizeof(unsigned int);
			*(unsigned char *)frag_survey_ptr = OPT_FRAG_CTRL_SUBSTR;
			frag_survey_ptr += sizeof(unsigned char);
			memcpy(frag_survey_ptr, mdi->opt[MEDIA_OPT_FD_SUBSTR]->opt_value, mdi->opt[MEDIA_OPT_FD_SUBSTR]->opt_len);		
			frag_survey_ptr += mdi->opt[MEDIA_OPT_FD_SUBSTR]->opt_len;

			memcpy(frag_survey_ptr, buf+sizeof(msg_header_t)+sizeof(resp_checkresult_t), buflen-sizeof(msg_header_t)-sizeof(resp_checkresult_t));

			buf = frag_survey_buf; 
			buflen = frag_survey_buflen;		
		}	
		*/

		/*
		//碎片化GK V2版本: 相似串GK，只提供OPT_FRAG_URL
		frag_survey_buflen = buflen+mdi->opt[MEDIA_OPT_URL]->opt_len+sizeof(uint32_t)+sizeof(uint8_t);
		frag_survey_buf = (char*)malloc(frag_survey_buflen);
		frag_survey_ptr = frag_survey_buf;

		//选项数增加一个
		check_res->opt_num += 1;
		memcpy(frag_survey_ptr, buf, sizeof(msg_header_t)+sizeof(resp_checkresult_t));
		frag_survey_ptr += sizeof(msg_header_t)+sizeof(resp_checkresult_t);

		//OPT_FRAG_URL选项
		*(unsigned int *)frag_survey_ptr = mdi->opt[MEDIA_OPT_URL]->opt_len+sizeof(uint32_t)+sizeof(uint8_t);
		frag_survey_ptr += sizeof(unsigned int);
		*(unsigned char *)frag_survey_ptr = OPT_FRAG_ORIGIN_URL;
		frag_survey_ptr += sizeof(unsigned char);
		memcpy(frag_survey_ptr, mdi->opt[MEDIA_OPT_URL]->opt_value, mdi->opt[MEDIA_OPT_URL]->opt_len);		
		frag_survey_ptr += mdi->opt[MEDIA_OPT_URL]->opt_len;
		*/
		
		//碎片化管控V3版本: 通过索引文件GK，回传OPT_FRAG_INDEX_URL
		frag_survey_buflen = buflen+mdi->opt[MEDIA_OPT_INDEX_URL][mdi->url_opt_index]->opt_len+sizeof(uint32_t)+sizeof(uint8_t);
		frag_survey_buf = (char*)malloc(frag_survey_buflen);
		frag_survey_ptr = frag_survey_buf;

		//选项数增加一个
		check_res->opt_num += 1;
		memcpy(frag_survey_ptr, buf, sizeof(msg_header_t)+sizeof(resp_checkresult_t));
		frag_survey_ptr += sizeof(msg_header_t)+sizeof(resp_checkresult_t);

		//OPT_FRAG_INDEX_URL选项
		*(unsigned int *)frag_survey_ptr = mdi->opt[MEDIA_OPT_INDEX_URL][mdi->url_opt_index]->opt_len+sizeof(uint32_t)+sizeof(uint8_t);
		frag_survey_ptr += sizeof(unsigned int);
		*(unsigned char *)frag_survey_ptr = OPT_FRAG_INDEX_URL;
		frag_survey_ptr += sizeof(unsigned char);
		memcpy(frag_survey_ptr, mdi->opt[MEDIA_OPT_INDEX_URL][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_INDEX_URL][mdi->url_opt_index]->opt_len);		
		frag_survey_ptr += mdi->opt[MEDIA_OPT_INDEX_URL][mdi->url_opt_index]->opt_len;
					
		//*建议*/
		memcpy(frag_survey_ptr, buf+sizeof(msg_header_t)+sizeof(resp_checkresult_t), buflen-sizeof(msg_header_t)-sizeof(resp_checkresult_t));
		log_url = frag_survey_ptr;
		log_urllen = buflen-sizeof(msg_header_t)-sizeof(resp_checkresult_t);

		buf = frag_survey_buf; 
		buflen = frag_survey_buflen;
		g_frag_stat.stat_info[RESP_CHECKRESULT_INDEX_SURVEY][TOTAL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_INDEX_SURVEY][TOTAL_BYTES] += buflen;			
	}	

	/*给前端发建议*/
	for(i=0; i<QD_MAXNUM; i++)
	{
		ip = mdi->qd_info[i].cap_ip;
		if(!ip)
		{
			continue;
		}		
		inet_ntop(AF_INET, &ip, pbuf, buf_len);			
		resp_write_to_log(RESP_SEND_BLOCK, check_res, pbuf, mdi, mdi->qd_info[i].mid);
		*(uint64_t*)check_res->prog_id = mdi->qd_info[i].mid;
		bizman_send(g_frag_run.answer_sapp_bizman, mdi->thread_seq, ip, g_frag_cfg.qd_msg_port, (const char*)buf, buflen, 
					1,BIZMAN_RELIABLE_SEND|BIZMAN_SMOOTH_DEST|BIZMAN_PUSH_SEND);		
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_OUT_BLOCK][TOTAL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_OUT_BLOCK][TOTAL_BYTES] += buflen;		
	}	

	/*多源情况下，给其他粗拼装的前端发建议*/
	for(i=0; i<QD_MAXNUM; i++)
	{
		ip = mdi->qd_info_from_cpz[i].cap_ip;
		if(!ip)
		{
			continue;
		}		
		inet_ntop(AF_INET, &ip, pbuf, buf_len);			
		resp_write_to_log(RESP_SEND_BLOCK_MULTI, check_res, pbuf, mdi, mdi->qd_info_from_cpz[i].mid);
		*(uint64_t*)check_res->prog_id = mdi->qd_info_from_cpz[i].mid;
		bizman_send(g_frag_run.answer_sapp_bizman, mdi->thread_seq, ip, g_frag_cfg.qd_msg_port, (const char*)buf, buflen, 
					1,BIZMAN_RELIABLE_SEND|BIZMAN_SMOOTH_DEST|BIZMAN_PUSH_SEND);		
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_OUT_BLOCK][TOTAL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_OUT_BLOCK][TOTAL_BYTES] += buflen;		
	}

	if(NULL!=frag_survey_buf)
	{
		free(frag_survey_buf);
		frag_survey_buf = NULL;
		frag_survey_buflen = 0;
	}	

	if(g_frag_cfg.media_json_switch)
	{
		mdi->cfg_id = check_res->cfg_id;
		mdi->service = check_res->service;
		mdi->level = check_res->level;
		mdi->survey_time = time(NULL);
		memcpy(mdi->log_url, log_url, MIN(log_urllen, sizeof(mdi->log_url)-1));
#if K_PROJECT
		send_json_log(mdi,check_res);
#else
		//send_json_log(mdi,check_res);//for test

		resp_checkresult_json_report(mdi, check_res);
#endif
	}
	
	g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_BLOCK][TOTAL_PKTS]++; 
	g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN_BLOCK][TOTAL_BYTES] += buflen;

	/*save configID for soqav_dup_report*/
	mdi->configID = check_res->cfg_id;
	return 0;
}

int proc_resp_checkresult(char* buf, uint32_t size)
{
	temp_buf_t temp_buf;
	uint32_t i=0;	
	resp_checkresult_t* check_res = (resp_checkresult_t*)(buf + MSG_HEADER_LEN);
		
	/* Ignore invalid packet */
	if(size < MSG_HEADER_LEN + MSG_RESP_CHECKRESULT_LEN)
	{		
		g_frag_stat.stat_info[INVALID_RESP_CHECKRESULT][TOTAL_PKTS]++;
		g_frag_stat.stat_info[INVALID_RESP_CHECKRESULT][TOTAL_BYTES] += size;
		return(-1);
	}		
	
	resp_write_to_log(RECV_RESP, check_res, NULL,NULL,0);
	g_frag_stat.stat_info[RESP_CHECKRESULT][TOTAL_PKTS]++;
	g_frag_stat.stat_info[RESP_CHECKRESULT][TOTAL_BYTES] += size;

	/*whiltelist*/
	if(0==check_res->level)
	{
		resp_write_to_log(RECV_RESP_WHITELIST, check_res, NULL,NULL,0);
		g_frag_stat.stat_info[RESP_CHECKRESULT_WHITELIST_IN][TOTAL_PKTS]++;	
		g_frag_stat.stat_info[RESP_CHECKRESULT_WHITELIST_IN][TOTAL_BYTES] += size;
		int return_size = 0;
		for(i=0;i<g_frag_cfg.whitelist_addr_num;i++)
		{				
			return_size = send_udp_socket_send(g_frag_run.send_msg_sd, 
											   g_frag_cfg.whitelist_addr[i].sin_addr.s_addr, 
											   g_frag_cfg.whitelist_addr[i].sin_port,
											   buf, size);
			if(-1==return_size)
			{
				/*static output*/				
				g_frag_stat.whitelist_send_stat[i][TOTAL_PKTS]++;
				g_frag_stat.whitelist_send_stat[i][TOTAL_BYTES] += return_size;	
				
			}
			else
			{
				/*static output*/
				g_frag_stat.whitelist_send_stat[i][TOTAL_PKTS]++;
				g_frag_stat.whitelist_send_stat[i][TOTAL_BYTES] += return_size;		
			}		
		}		
	}
	else
	{
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN][TOTAL_PKTS]++;	
		g_frag_stat.stat_info[RESP_CHECKRESULT_BLACKLIST_IN][TOTAL_BYTES] += size;
		long  rec_cb = 0;
		temp_buf.buf = buf;
		temp_buf.size = size;
		MESA_htable_search_cb(g_frag_run.media_hash,(const uint8_t *)&check_res->prog_id,sizeof(check_res->prog_id),
							  resp_checkresult_search_media_cb, (void*)&temp_buf, &rec_cb);
	}	
	return 0;
}

int construct_check_result(char* buf, int* size)
{
	if(int(*size-sizeof(msg_head_t)-sizeof(msg_result_t))<0) return -1;
	int 				new_size = 0;
	char				new_buf[SOCKET_BUF_SIZE] = {0}; 		
	msg_header_t*		new_header = (msg_header_t*)new_buf;	
	msg_head_t*			head = (msg_head_t*)buf;
	msg_result_t* 		result = (msg_result_t*)(buf+sizeof(msg_head_t));	
	resp_checkresult_t* check_res = (resp_checkresult_t*)(new_buf + MSG_HEADER_LEN);

	/*如果采用AV_sendback.h定义的回传接口， 需要通过msg_result_t转换为resp_checkresult_t结构的buf,size*/
	new_header->magic_num = PROTO_MAGICNUM;		
	new_header->version = PROTO_VERSION;
	new_header->msg_type = head->m_type;
	new_header->cont_len = sizeof(resp_checkresult_t)+(*size-sizeof(msg_head_t)-sizeof(msg_result_t));

	*(uint64_t*)(check_res->prog_id) = *(uint64_t*)result->pid;		
	check_res->service = result->servicetype;
	check_res->level = result->level;		
	check_res->cfg_id = result->cfgid;
	check_res->file_header_size = 0;
	check_res->file_packet_size = 0;

	memcpy(new_buf+MSG_HEADER_LEN+sizeof(resp_checkresult_t), buf+sizeof(msg_head_t)+sizeof(msg_result_t), *size-sizeof(msg_head_t)-sizeof(msg_result_t));
	new_size = MSG_HEADER_LEN+new_header->cont_len;
	memset(buf, 0, *size);
	memcpy(buf, new_buf, new_size);
	*size = new_size;

	return 0;
}

int msghead_check(char* buf, uint32_t size)
{
	msg_header_t*	header = (msg_header_t*)buf;

	/* Ignore invalid packet */
	if(size < (int)sizeof(msg_header_t))
	{			
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
					    		"{%s:%d} recv_response_msg too samll %d < %d.", 
					    		__FILE__,__LINE__, size, MSG_HEADER_LEN);
		return -1;
	}
	if(PROTO_MAGICNUM != header->magic_num && AV_MAGIC_VALUE != header->magic_num)
	{
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
					    		"{%s:%d} recv_response_msg magic_num error: %d != %d.", 
					    		__FILE__,__LINE__, header->magic_num, PROTO_MAGICNUM);
		return -1;
	}		
	return 0;
}

void* recv_response_msg(void *param)
{
	int 					size = 0;
	uint32_t				src_ip = 0;
	char			 		buf[SOCKET_BUF_SIZE] = {0};	
	msg_header_t*			msg_header = (msg_header_t*)buf;
	fd_set 					rset;
	int 					maxfdpl = ((g_frag_run.recv_msg_fd > g_frag_run.recv_msg_sd) ? (g_frag_run.recv_msg_fd) : (g_frag_run.recv_msg_sd))+1;
	
	while(1)
	{
		/*recv msg*/
		FD_ZERO(&rset);
		if(g_frag_run.recv_msg_fd>=0)
		{
			FD_SET(g_frag_run.recv_msg_fd,&rset);
		}
		if(g_frag_run.recv_msg_sd>=0)
		{
			FD_SET(g_frag_run.recv_msg_sd,&rset);
		}
		FD_SET(g_frag_run.recv_msg_fd,&rset);
		FD_SET(g_frag_run.recv_msg_sd,&rset);
		if(-1==select(maxfdpl,&rset,NULL,NULL,NULL))
		{
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} recv_response_msg select error.", 
						    		__FILE__,__LINE__);
			continue;
		}
		if(FD_ISSET(g_frag_run.recv_msg_fd,&rset))
		{
			size = recv_unix_socket_recv(g_frag_run.recv_msg_fd, (char*)buf, sizeof(buf));
		}
		else if(FD_ISSET(g_frag_run.recv_msg_sd,&rset))
		{
			size = recv_udp_socket_recv(g_frag_run.recv_msg_sd, &src_ip, (unsigned char*)buf, sizeof(buf));			
		}
		else
		{
			printf("select in msg fd nothing to read.\n");
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} recv_response_msg select nothing to read.", 
						    		__FILE__,__LINE__);
			continue;
		}

		if(-1==msghead_check(buf, size))
		{			
			continue;
		}	
		/*接收检测结果可能包括av_sendback.h和av_interface.h两种接口*/
#if K_PROJECT
		construct_check_result(buf, &size);
#endif
		/*proc msg*/
		switch(msg_header->msg_type)
		{
			case MSG_RESP_CHECKRESULT:				
				proc_resp_checkresult(buf, size);
				break;
				
			/*ack from wins metadata to do*/
			case MSG_RESP_CHARACTER:
				g_frag_stat.stat_info[RESP_CHARACTER][TOTAL_PKTS]++;
				g_frag_stat.stat_info[RESP_CHARACTER][TOTAL_BYTES] += size;
				break;

			case MSG_PROG_SYNC:
				proc_prog_sync(buf, size);
				break;			
				
			default:
				g_frag_stat.stat_info[RESP_UNRECOGNIZED][TOTAL_PKTS]++;
				g_frag_stat.stat_info[RESP_UNRECOGNIZED][TOTAL_BYTES] += size;
				MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} proc_response_msg	receiving checkresult,unrecognized msg_type=%d", 
										__FILE__,__LINE__, msg_header->msg_type);			
				break;
		}
	}
	return NULL;
}

