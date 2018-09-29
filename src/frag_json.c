/*
* 跨媒体:传输运行状态
*/

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <net/if.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <openssl/md5.h>

#include "cJSON.h"

#include "MESA_handle_logger.h"
#include "stream_fuzzy_hash.h"
#include "AV_sendback_in.h"
#include "frag_json.h"
#include "frag_proc.h"
#include "log.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;

extern sip_opt_t	g_sip_opt_type[SIP_OPT_NUM];

const char* g_td_des_json[] = 
{
	"data_md5_1k", 
	"td_1k",	
	"data_md5_2k", 	
	"td_2k",
	"data_md5_4k", 	
	"td_4k"	,
	"data_md5_8k", 	
	"td_8k"	,
	"data_md5_16k",	
	"td_16k",
	"data_md5_32k",	
	"td_32k",	
	"data_md5_64k",	
	"td_64k",
};

/*入kafka的选项*/
#define 			JSON_MAXOPT	8
int 				json_opt[JSON_MAXOPT] = {MEDIA_OPT_CAP_IP,MEDIA_OPT_URL, MEDIA_OPT_REFERER, MEDIA_OPT_UA, MEDIA_OPT_SINGLE_KEY, MEDIA_OPT_ETAG, MEDIA_OPT_LAST_MODIFY, MEDIA_OPT_SERVER};
const char* 		json_optname[JSON_MAXOPT] = {"cap_ip", "url", "referer", "ua", "single_key", "etag", "last_modify", "server"};
void media_json_report(media_t* mdi, int topic_event)
{	
	cJSON* 		root;	
	char* 		digest_buff = NULL;
	unsigned long long digest_len = 0;
	uint64_t	pid = 0;
	cJSON* 		ip_array = NULL;
	cJSON* 		pid_array = NULL;
	cJSON* 		muti_url_array = NULL;
	uint32_t 	ip = 0;
	char* 		out = NULL;	
	uint32_t	out_len = 0;
	char		pbuf[32] = {0}; 
	int 		buf_len = 32;
	char		filename[64] = {0};
	FILE*       fp = NULL;	
	string		topic_name;	
	const char*	topic = NULL;	
	char		pid_buf[32] = {0}; 	
	char 		td[TD_LEN] = {0};
	int 		cb_ret = 0;
	touple4_type_t touple4;
	
	if(topic_event==TOPIC_EVENT_CREATE)
	{
		if(mdi->proto==AV_PROTOCOL_SIP)
		{
			topic = TOPIC_VOIP_CREATE_JSON;
		}
		else
		{
			topic = TOPIC_MEDIA_CREATE_JSON;
		}
	}
	else if(topic_event==TOPIC_EVENT_EXPIRE)
	{	
		if(mdi->proto==AV_PROTOCOL_SIP)
		{
			topic = TOPIC_VOIP_EXPIRE_JSON;
		}
		else
		{
			topic = TOPIC_MEDIA_EXPIRE_JSON;
		}
	}
	else
	{
		return ;
	}
	
	topic_name = topic;

	root = cJSON_CreateObject();

	/*mid*/	
	snprintf(pid_buf, sizeof(pid_buf), "%lu", mdi->mid);
	cJSON_AddStringToObject(root, "mid", pid_buf);

	/*trans_len*/
	cJSON_AddNumberToObject(root, "trans_len", mdi->byte_proc);

	/*media_len*/
	cJSON_AddNumberToObject(root, "media_len", mdi->media_len);

	/*media_type*/
	cJSON_AddNumberToObject(root, "media_type", mdi->media_type);

	/*create_time*/
	cJSON_AddNumberToObject(root, "create_time", mdi->create_time);

	/*expire_time*/
	cJSON_AddNumberToObject(root, "expire_time", time(NULL));

	/*cpz_ip*/
	memset(pbuf, 0, sizeof(pbuf));
	inet_ntop(AF_INET, &g_frag_cfg.local_ip_nr, pbuf, buf_len); 		
	cJSON_AddStringToObject(root, "cpz_ip", pbuf);

	/*pid*/	
	if(NULL!=mdi->pid)
	{
		pid_array = cJSON_CreateArray();
		for(int i=0;i<PID_MAXNUM;i++)
		{
			pid = mdi->pid[i];
			if(!pid)
			{
				continue;
			}
			memset(pid_buf, 0, sizeof(pid_buf));
			snprintf(pid_buf, sizeof(pid_buf), "%lu", pid);
			cJSON_AddStringToObject(pid_array, "pid", pid_buf);
		}	
		cJSON_AddItemToObject(root, "pid", pid_array);
	}	

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
		cJSON_AddStringToObject(ip_array, "cap_ip", pbuf);			
	}	
	cJSON_AddItemToObject(root, "cap_ip", ip_array);

	/*SIP记录字节数*/
	if(mdi->media_service_type==MEDIA_SERVICE_TYPE_SIP)
	{
		cJSON_AddNumberToObject(root, "pkt_in", mdi->pkt_in);
		cJSON_AddNumberToObject(root, "pkt_out", mdi->pkt_proc);
		cJSON_AddNumberToObject(root, "byte_in", mdi->byte_in);
		cJSON_AddNumberToObject(root, "byte_out", mdi->byte_proc);
		cJSON_AddNumberToObject(root, "reoffset", mdi->re_offset);
		char buf[128] = {0};
		for(int i=0;i<SIP_OPT_NUM;i++)
		{
			if(NULL!=mdi->sip_opt[i])
			{
				memset(buf, 0, sizeof(buf));
				memcpy(buf, mdi->sip_opt[i]->opt_value, MIN(mdi->sip_opt[i]->opt_len, 127));
				cJSON_AddStringToObject(root, g_sip_opt_type[i].opt_name, buf);
			}
		}
	}
	else
	{
		/*digest*/
		if(NULL!=mdi->fuzzy)
		{
			digest_len = SFH_status(mdi->fuzzy, HASH_LENGTH);
			digest_buff = (char*)calloc(1, sizeof(char)*digest_len+1);
			SFH_digest(mdi->fuzzy, digest_buff, digest_len);	
			cJSON_AddStringToObject(root, "digest", digest_buff);
		}	
		/*digest_len*/	
		cJSON_AddNumberToObject(root, "digest_len", digest_len);	
		
		/*multi_url*/
		char		opt_temp[JSON_MAXOPT][4096] = {{0}};
		cJSON*		url_obj[OPT_MAXNUN] = {NULL};
		muti_url_array = cJSON_CreateArray();
		for(int i=0;i<mdi->opt_index;i++)
		{
			url_obj[i] = cJSON_CreateObject();
	
			//pid
			memset(pid_buf, 0, sizeof(pid_buf));
			snprintf(pid_buf, sizeof(pid_buf), "%lu", *(uint64_t*)mdi->opt[MEDIA_OPT_PID][i]->opt_value);
			cJSON_AddStringToObject(url_obj[i], "pid", pid_buf);
	
			//addr	
			if(NULL!=mdi->opt[MEDIA_OPT_ADDR][i])
			{
				if(sscanf(mdi->opt[MEDIA_OPT_ADDR][i]->opt_value, "%[0-9.],%hu>%[0-9.],%hu", touple4.sip, &touple4.sport, touple4.dip, &touple4.dport) == 4)
				{
					touple4.addr_type = 4; 
				}	
				else if(sscanf(mdi->opt[MEDIA_OPT_ADDR][i]->opt_value, "%[0-9A-Fa-f:.],%hu>%[0-9A-Fa-f:.],%hu",  touple4.sip, &touple4.sport, touple4.dip, &touple4.dport) == 4)
				{
					touple4.addr_type = 6; 
				}
				cJSON_AddStringToObject(url_obj[i], "s_ip", touple4.sip);
				cJSON_AddNumberToObject(url_obj[i], "s_port", touple4.sport);
				cJSON_AddStringToObject(url_obj[i], "d_ip", touple4.dip);
				cJSON_AddNumberToObject(url_obj[i], "d_port", touple4.dport);
			}
	
			//protocol
			cJSON_AddNumberToObject(url_obj[i], "protocol", *(uint8_t*)mdi->opt[MEDIA_OPT_PROTOCOL][i]->opt_value);
	
			//others
			for(int j=0;j<JSON_MAXOPT;j++)
			{
				if(NULL!=mdi->opt[json_opt[j]][i])
				{
					memcpy(opt_temp[j], mdi->opt[json_opt[j]][i]->opt_value, MIN(sizeof(opt_temp[j])-1, mdi->opt[json_opt[j]][i]->opt_len));
					cJSON_AddStringToObject(url_obj[i], json_optname[j], opt_temp[j]);
				}
			}
			cJSON_AddItemToObject(muti_url_array, "multi_session", url_obj[i]);
		}
		cJSON_AddItemToObject(root, "multi_session", muti_url_array);
			
		/*index_url*/		
		if(NULL!=mdi->opt[MEDIA_OPT_INDEX_URL][mdi->url_opt_index])
		{
			cJSON_AddStringToObject(root, "index_url", mdi->opt[MEDIA_OPT_INDEX_URL][mdi->url_opt_index]->opt_value);
		}
		/*index_referer*/		
		if(NULL!=mdi->opt[MEDIA_OPT_INDEX_REFERER][mdi->url_opt_index])
		{
			cJSON_AddStringToObject(root, "index_referer", mdi->opt[MEDIA_OPT_INDEX_REFERER][mdi->url_opt_index]->opt_value);
		}	
		/*index_ua*/		
		if(NULL!=mdi->opt[MEDIA_OPT_INDEX_UA][mdi->url_opt_index])
		{
			cJSON_AddStringToObject(root, "index_ua", mdi->opt[MEDIA_OPT_INDEX_UA][mdi->url_opt_index]->opt_value);
		}		
	
		/*将载相关内容*/
		if(NULL!=mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index])
		{		
			/*td_meta*/ 
			cJSON_AddStringToObject(root, "td_meta", mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value);
			
			/*td_0k*/
			caculate_md5(mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_len, NULL, 0, td, TD_LEN);		
			cJSON_AddStringToObject(root, "td_0k", td);
	
			/*data_md5_1k*/ 	/*td_1k*/	
			/*data_md5_2k*/ 	/*td_2k*/	
			/*data_md5_4k*/ 	/*td_4k*/	
			/*data_md5_8k*/ 	/*td_8k*/	
			/*data_md5_16k*/	/*td_16k*/
			/*data_md5_32k*/	/*td_32k*/	
			/*data_md5_64k*/	/*td_64k*/
			uint32 i=0;
			for(int j=0;j<=6;j++)
			{
				i=1<<j;
				if(mdi->td_datalen>=i*1024)
				{
					caculate_md5(NULL, 0, mdi->td_data, i*1024, td, TD_LEN); 
					cJSON_AddStringToObject(root, g_td_des_json[2*j], td);
	
					caculate_md5(mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_len, mdi->td_data, i*1024, td, TD_LEN); 
					cJSON_AddStringToObject(root, g_td_des_json[2*j+1], td);
				}
			}			
		}
	}	
	
	/*节目建议*/
	/*cfg_id*/
	cJSON_AddNumberToObject(root, "cfg_id", mdi->cfg_id);	

	/*service*/
	cJSON_AddNumberToObject(root, "service", mdi->service);	

	/*level*/
	cJSON_AddNumberToObject(root, "level", mdi->level);	

	/*survey time*/
	cJSON_AddNumberToObject(root, "survey_time", mdi->survey_time);
	
	/*log_url*/
	cJSON_AddStringToObject(root, "log_url", mdi->log_url);

	//out = cJSON_PrintUnformatted(root);	
	out = cJSON_Print(root);	
	out_len = strlen(out);

	cb_ret = g_frag_run.kafka_producer->SendData(topic_name, (void *)out, (size_t)out_len);
	if(0!=cb_ret)
	{
		/*fail output*/					
		atomic_inc(&g_frag_stat.stat_info[MEDIA_JSON][FAIL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[MEDIA_JSON][FAIL_BYTES], out_len);	
	}
	else
	{
		/*succ output*/
		atomic_inc(&g_frag_stat.stat_info[MEDIA_JSON][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[MEDIA_JSON][TOTAL_BYTES], out_len);						
	}

	/*for debug*/
	if(g_frag_cfg.json_local_switch)
	{
		struct tm	now;
		char		day_time[32] = {0};	
		struct timeval 	tv;
		struct timezone tz;
		gettimeofday(&tv, &tz);
		localtime_r(&tv.tv_sec, &now);		
		strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now); 
		snprintf(filename, sizeof(filename), "%s%s%s_%s", "./log/", topic, ".json", day_time);
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
	if(NULL!=digest_buff)
	{
		free(digest_buff);
	}
}

