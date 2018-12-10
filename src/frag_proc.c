

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

#include "MESA_handle_logger.h"
#include "MESA_prof_load.h"
#include "MESA_htable.h"
#include "MESA_list_queue.h"
#include "MESA_trace.h"
#include "stream.h"
#include "bizman.h"
#include "interval_index.h"
#include "stream_fuzzy_hash.h"
#include "soqav_dedup.h"
#include "app_detect.h"
#include "KafkaProducer.h"

#include "common.h"
#include "frag_reassembly_in.h"
#include "frag_proc.h"
#include "log.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;
extern frag_reassembly_t 			frag_rssb;

extern sip_opt_t	g_sip_opt_type[SIP_OPT_NUM]; 

/*use for mid*/
unsigned long long idMaskIndex[16] =
{ 
		0x0000000000000000,   /* unknown */
        0x1000000000000000,   /*url*/
        0x2000000000000000,   /*服务器ip +len*/
        0x3000000000000000,   /*文件内容*/
        0x4000000000000000,   /*四元组*/
        0x5000000000000000,   /*唯一计数*/
        0x6000000000000000,   /*resort  data*/
        0x7000000000000000,   /*youtube data*/
};

void create_media_write_to_log(media_t* mdi, int type, void* param)
{
	char 		buf[2048] = {0};	
	int 		buflen = 0;
	time_t		cur_time;
	struct timeval 	tv;
	struct timezone tz;
	struct tm	now;
	char		now_time[32] = {0};
	char		day_time[32] = {0};	
	char		filename[MAX_PATH_LEN] = {0};
	FILE*		fp = NULL;
	char		ip_buf[64] = {0};
	
	time(&cur_time);
	//localtime_r(&cur_time, &now);
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &now);	
	strftime(now_time, sizeof(now_time), "%Y-%m-%d %H:%M:%S", &now);

	/*init*/
	pthread_mutex_lock(&g_frag_run.media_create_file_lock);
	if(NULL==g_frag_run.media_create_file)
	{
		strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);		
		snprintf(filename, sizeof(filename), "%s.%s", g_frag_cfg.media_create_filename, day_time);
		g_frag_run.media_create_file = fopen(filename, "a+");			
		g_frag_run.cur_time = cur_time;
		localtime_r(&g_frag_run.cur_time, &g_frag_run.media_create_filetime);
	}	
	if(now.tm_mday!=g_frag_run.media_create_filetime.tm_mday || now.tm_mon!=g_frag_run.media_create_filetime.tm_mon || now.tm_year!=g_frag_run.media_create_filetime.tm_year)
	{
		if(g_frag_run.media_create_file)
		{		
			fclose(g_frag_run.media_create_file);			
			strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);	
			snprintf(filename, sizeof(filename), "%s.%s", g_frag_cfg.media_create_filename, day_time);
			g_frag_run.media_create_file = fopen(filename, "a+");			
		}		
		g_frag_run.cur_time = cur_time;
                localtime_r(&g_frag_run.cur_time, &g_frag_run.media_create_filetime);
	}	
	pthread_mutex_unlock(&g_frag_run.media_create_file_lock);
	
	switch(type)
	{
		case MEDIA_NEW:
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 "\n", now_time, tv.tv_usec, "create_media", mdi->mid,mdi->media_type,mdi->media_len);	
			fp = g_frag_run.media_create_file;			
			break;

		case MEDIA_RENEW:			
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 "\n", now_time, tv.tv_usec, "renew_media", mdi->mid, mdi->media_type,mdi->media_len);				
			fp = g_frag_run.media_create_file;	
			break;

		case MEDIA_MID_CHANGE:			
			inet_ntop(AF_INET, &mdi->multisrc_bizmanip, ip_buf, sizeof(ip_buf));
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",new_mid:%" PRIu64 ",to_cpz:%s\n", now_time, tv.tv_usec, "change_mid", mdi->mid, mdi->media_type,mdi->media_len, *(uint64_t*)param,ip_buf);				
			fp = g_frag_run.media_create_file;	
			break;

		case MEDIA_OFFSET_ZERO:
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",offset:0\n", now_time, tv.tv_usec, "media_offset_0", mdi->mid, mdi->media_type,mdi->media_len);				
			fp = g_frag_run.media_create_file;			
			break;		

		case MEDIA_FROM_CAPIP:
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",capIP:%s\n", now_time, tv.tv_usec, "media_from_qd", mdi->mid, mdi->media_type,mdi->media_len, (char*)param);				
			fp = g_frag_run.media_create_file;	
			break;	
			
		case MEDIA_FROM_CPZIP:
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",capIP:%s\n", now_time, tv.tv_usec, "media_from_cpz", mdi->mid, mdi->media_type,mdi->media_len, (char*)param);				
			fp = g_frag_run.media_create_file;	
			break;	

		case MEDIA_FROM_QDIP:
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",qdIP:%s\n", now_time, tv.tv_usec, "media_query_qd", mdi->mid, mdi->media_type,mdi->media_len, (char*)param);				
			fp = g_frag_run.media_create_file;	
			break;	
		
		default:
			break;
	}

	if(fp)
	{
		fwrite(buf, buflen, 1, fp);
		fflush(fp);
	}
}

void expire_media_write_to_log(media_t* mdi, int type, void* param)
{
	char 		buf[2048] = {0};	
	int 		buflen = 0;
	time_t		cur_time;
	struct tm	now;
	struct tm	time_cache;
	char		now_time[32] = {0};
	char		create_time[32] = {0};	
	char		day_time[32] = {0};	
	char		filename[MAX_PATH_LEN] = {0};	
	char		renew_desc[MAX_PATH_LEN] = {"renew_expire_media"};
	char		expire_desc[MAX_PATH_LEN] = {"expire_media"};
	char*		type_desc = NULL;
	FILE*		fp = NULL;
	
	time(&cur_time);
	localtime_r(&cur_time, &now);
	strftime(now_time, sizeof(now_time), "%Y-%m-%d %H:%M:%S", &now);

	/*init*/
	pthread_mutex_lock(&g_frag_run.media_expire_file_lock);
	if(NULL==g_frag_run.media_expire_file)
	{
		strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);		
		snprintf(filename, sizeof(filename), "%s.%s", g_frag_cfg.media_expire_filename, day_time);
		g_frag_run.media_expire_file = fopen(filename, "a+");			
		g_frag_run.cur_time = cur_time;
		localtime_r(&g_frag_run.cur_time, &g_frag_run.media_expire_filetime);
	}	
	if(now.tm_mday!=g_frag_run.media_expire_filetime.tm_mday || now.tm_mon!=g_frag_run.media_expire_filetime.tm_mon || now.tm_year!=g_frag_run.media_expire_filetime.tm_year)
	{
		if(g_frag_run.media_expire_file)
		{		
			fclose(g_frag_run.media_expire_file);				
			strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);	
			snprintf(filename, sizeof(filename), "%s.%s", g_frag_cfg.media_expire_filename, day_time);
			g_frag_run.media_expire_file = fopen(filename, "a+");			
		}
		g_frag_run.cur_time = cur_time;
        localtime_r(&g_frag_run.cur_time, &g_frag_run.media_expire_filetime);
	}
	pthread_mutex_unlock(&g_frag_run.media_expire_file_lock);
	
	switch(type)
	{	
		case MEDIA_RENEW_EXPIRE:
			type_desc = renew_desc;
			break;
		case MEDIA_EXPIRE:			
			type_desc = expire_desc;  				
			break;
		
		default:
			break;
	}

	localtime_r(&mdi->create_time,&time_cache);
	strftime(create_time, sizeof(create_time), "%Y-%m-%d %H:%M:%S", &time_cache);
	fp = g_frag_run.media_expire_file;
	buflen = snprintf(buf, sizeof(buf), "%20s,%12s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",maxoffset:%10" PRIu64 ",frag_unit_cnt:%3u,pkt:%7u/%7u,byte:%10" PRIu64 "/%10" PRIu64 ",time:%s/%s\n",
					  now_time,
					  type_desc,
					  mdi->mid, mdi->media_type,mdi->media_len,mdi->maxoffset,mdi->frag_unit_cnt,
					  mdi->pkt_proc, mdi->pkt_in, 
					  mdi->byte_proc, mdi->byte_in, 
					  now_time,
					  create_time);	
	
	if(fp)
	{
		fwrite(buf, buflen, 1, fp);
		fflush(fp);
	}
}

void frag_write_to_log(int type, uint64_t mid, void* param1, void* param2, uint32_t param3)
{
	char			ip_buf[32] = {0};
	frag_in_t* 		frg = NULL;
	frag_unit_t* 	frg_unit = NULL;
	media_t* 		mdi = NULL;
	media_info_t* 	media_info = NULL;
	int				trace_flag = 0;
	char 			buf[2048] = {0};	
	int 			buflen = 0;	
	char 			strbuf[1024] = {0};	
	char 			strbuf1[1024] = {0};	
	int 			strbuflen = 1024;	
	time_t			cur_time;
	struct tm*		now;
	char			now_time[32] = {0};
	char			filename[MAX_PATH_LEN] = {0};
	FILE*			fp = NULL;

	if(NULL!=g_frag_run.mid_trace_hd)
	{
		trace_flag = MESA_trace_match_numerial(g_frag_run.mid_trace_hd, mid);
	}

	if(trace_flag)
	{
		time(&cur_time);
		now = localtime(&cur_time);
		strftime(now_time, sizeof(now_time), "%Y-%m-%d %H:%M:%S", now);
	}

	switch(type)
	{	
		case ADD_META:	
			frg_unit = (frag_unit_t*)param1;
			inet_ntop(AF_INET, &frg_unit->capIP, ip_buf, sizeof(ip_buf));
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [PID:%" PRIu64 ", media_len:%" PRIu64 ", media_type:0x%02x, cap_IP:%s]", 
					  				__FILE__,__LINE__, "add_mediainfo", frg_unit->pid, frg_unit->media_len, frg_unit->media_type, ip_buf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", media_len:%" PRIu64 ", media_type:0x%02x, cap_IP:%s]\n", 
								  now_time, "add_mediainfo", 
								  frg_unit->pid, frg_unit->media_len, frg_unit->media_type, ip_buf);	
			}
			break;	

		case FRAG_FORECAST_ERROR:	
			frg_unit = (frag_unit_t*)param1;
			inet_ntop(AF_INET, &frg_unit->capIP, ip_buf, sizeof(ip_buf));
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [PID:%" PRIu64 ", media_len:%" PRIu64 ", media_type:0x%02x, cap_IP:%s, url:%s]", 
					  				__FILE__,__LINE__, "frag_forecast_error", frg_unit->pid, frg_unit->media_len, frg_unit->media_type, ip_buf, frg_unit->opt[MEDIA_OPT_URL]->opt_value);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", media_len:%" PRIu64 ", media_type:0x%02x, cap_IP:%s, url:%s]\n", 
								  now_time, "frag_forecast_error", 
								  frg_unit->pid, frg_unit->media_len, frg_unit->media_type, ip_buf, frg_unit->opt[MEDIA_OPT_URL]->opt_value);	
			}
			break;			

		case GET_META:
			media_info = (media_info_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [MID:%" PRIu64 ", media_len:%" PRIu64 ", media_type:0x%02x, protocol:%hu]", 
									__FILE__,__LINE__, "get_mediainfo", media_info->mid, media_info->prog_len, media_info->media_type, media_info->protocol);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID:%" PRIu64 ", media_len:%" PRIu64 ", media_type:0x%02x, protocol:%hu]\n", 
								  now_time, "get_mediainfo", 
								  media_info->mid, media_info->prog_len, media_info->media_type, media_info->protocol);	
			}
			break;

		case ADD_FRAG:				
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]", 
					   				__FILE__,__LINE__, "add_frag", frg->pid, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]\n", 
								  now_time, "add_frag", 
								  frg->pid, frg->offset, frg->datalen);	
			}
			break;

		case ADD_FRAG_NOUSE:				
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]", 
					   				__FILE__,__LINE__, "add_frag_no_unit", frg->pid, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]\n", 
								  now_time, "add_frag_no_unit", 
								  frg->pid, frg->offset, frg->datalen);	
			}
			break;

		case GET_FRAG:
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
					    			"{%s:%d} %20s : [MID:%" PRIu64 ", PID:%" PRIu64 ", reoffset:%hu, aboffset:%" PRIu64 ", datalen:%u]", 
					    			__FILE__,__LINE__, "get_frag", frg->mid, frg->pid, frg->seq, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID:%" PRIu64 ", PID:%" PRIu64 ", reoffset:%hu, aboffset:%" PRIu64 ", datalen:%u]\n", 
								  now_time, "get_frag", 
								  frg->mid, frg->pid, frg->seq, frg->offset, frg->datalen);	
			}	
			break;		

		case ADD_FRAG_TO_CNVG_LQ:			
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]", 
					   				__FILE__,__LINE__, "add frg_cnvg_lq", frg->pid, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]\n", 
								  now_time, "add frg_cnvg_lq", 
								  frg->pid, frg->offset, frg->datalen);	
			}
			break;

		case GET_FRAG_FROM_CNVG_LQ:			
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]", 
						    		__FILE__,__LINE__, "get frg_cnvg_lq", frg->pid, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]\n", 
								  now_time, "get frg_cnvg_lq", 
								  frg->pid, frg->offset, frg->datalen);	
			}
			break;

		case ADD_FRAG_TO_INDEX_LQ:
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]", 
									__FILE__,__LINE__, "add frg_index_lq", frg->pid, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]\n", 
								  now_time, "add frg_index_lq", 
								  frg->pid, frg->offset, frg->datalen);	
			}
			break;

		case GET_FRAG_FROM_INDEX_LQ:
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]", 
									__FILE__,__LINE__, "get frg_index_lq", frg->pid, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]\n", 
								  now_time, "get frg_index_lq", 
								  frg->pid, frg->offset, frg->datalen);	
			}
			break;			

		case ADD_FRAG_TO_WAIT_LQ:			
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
							    	"{%s:%d} %20s : [MID: %" PRIu64 ", PID: %" PRIu64 ", offset_in:%" PRIu64 ", reoffset:%hu, aboffset:%" PRIu64 ", datalen:%u]", 
							   		 __FILE__,__LINE__, "add wait_queue", frg->mid, frg->pid, frg->offset_in, frg->seq, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID: %" PRIu64 ", PID: %" PRIu64 ", offset_in:%" PRIu64 ", reoffset:%hu, aboffset:%" PRIu64 ", datalen:%u]\n", 
								  now_time, "add wait_queue", 
								  frg->mid, frg->pid, frg->offset_in, frg->seq, frg->offset, frg->datalen);	
			}
			break;	

		case ADD_FRAG_TO_TAILQ:			
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
							    	"{%s:%d} %20s : [MID: %" PRIu64 ", PID: %" PRIu64 ", offset_in:%" PRIu64 ", reoffset:%hu, aboffset:%" PRIu64 ", datalen:%u]", 
							   		 __FILE__,__LINE__, "add tailq", frg->mid, frg->pid, frg->offset_in, frg->seq, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID: %" PRIu64 ", PID: %" PRIu64 ", offset_in:%" PRIu64 ", reoffset:%hu, aboffset:%" PRIu64 ", datalen:%u]\n", 
								  now_time, "add tailq", 
								  frg->mid, frg->pid, frg->offset_in, frg->seq, frg->offset, frg->datalen);	
			}
			break;	

		case ADD_FRAG_FROM_TAILQ:
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]", 
									__FILE__,__LINE__, "get tailq", frg->pid, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", offset: %" PRIu64 ", datalen:%u]\n", 
								  now_time, "get tailq", 
								  frg->pid, frg->offset, frg->datalen);	
			}
			break;	
		
		case ADD_FRAG_TO_APP_LQ:			
			frg = (frag_in_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [MID: %" PRIu64 ", PID: %" PRIu64 ", offset_in:%" PRIu64 ", reoffset:%hu, aboffset:%" PRIu64 ", datalen:%u]", 
									 __FILE__,__LINE__, "add app_queue", frg->mid, frg->pid, frg->offset_in, frg->seq, frg->offset, frg->datalen);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID: %" PRIu64 ", PID: %" PRIu64 ", offset_in:%" PRIu64 ", reoffset:%hu, aboffset:%" PRIu64 ", datalen:%u]\n", 
								  now_time, "add wait_queue", 
								  frg->mid, frg->pid, frg->offset_in, frg->seq, frg->offset, frg->datalen); 
			}
			break;			
		
		case MEDIA_NO_META:
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [MID: %" PRIu64 "]", 
									__FILE__,__LINE__,"no mediainfo", mid);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID: %" PRIu64 "]\n", 
								  now_time, "no mediainfo", mid);	
			}	
			break;

		
		case HLS_OSMF_REPEAT:			
			frg_unit = (frag_unit_t*)param1;
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s : [MID: %" PRIu64 ", PID: %" PRIu64 ", reoffset:%" PRIu64 "]", 
									__FILE__,__LINE__, "hls and osmf repeat", mid, frg_unit->pid, frg_unit->re_offset);	
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID: %" PRIu64 ", PID: %" PRIu64 ", reoffset: %" PRIu64 "]\n", 
								  now_time, "hls and osmf repeat", mid, frg_unit->pid, frg_unit->re_offset);	
			}	
			break;

		case SET_OFFSET:			
			frg_unit = (frag_unit_t*)param1;
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [MID:%" PRIu64 ", PID:%" PRIu64 ", frag_unit_reoffset:%" PRIu64 ", frag_unit_aboffset:%" PRIu64 ", frag_unit_offset_in:%" PRIu64 "]", 
									__FILE__,__LINE__, "set frag_unit offset", mid, frg_unit->pid, frg_unit->re_offset, frg_unit->ab_offset, frg_unit->ab_offset_for_in);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID:%" PRIu64 ", PID:%" PRIu64 ", frag_unit_reoffset:%" PRIu64 ", frag_unit_aboffset:%" PRIu64 ", frag_unit_offset_in:%" PRIu64 "]\n", 
								  now_time, 
								  "set frag_unit offset", mid, frg_unit->pid, frg_unit->re_offset, frg_unit->ab_offset, frg_unit->ab_offset_for_in);	
			}	
			break;
		
		case APP_CHANGE_PID:			
			frg_unit = (frag_unit_t*)param1;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID: %" PRIu64 " change to PID: %" PRIu64 "]", 
						    		__FILE__,__LINE__, "app_change_pid", mid, frg_unit->pid);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 " change to PID: %" PRIu64 "]\n", 
								  now_time, "app_change_pid", mid, frg_unit->pid);	
			}
			break;

//**********************************************碎片单向流查询
		case SEND_CNVG_QUERY_1:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID: %" PRIu64 ", key: %s]", 
						    		__FILE__,__LINE__, "send_cnvg_query_1", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", key: %s]\n", 
								  now_time, "send_cnvg_query_1", mid, strbuf);	
			}
			break;	

		case SEND_CNVG_QUERY_2:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID: %" PRIu64 ", key: %s]", 
						    		__FILE__,__LINE__, "send_cnvg_query_2", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", key: %s]\n", 
								  now_time, "send_cnvg_query_2", mid, strbuf);	
			}
			break;
			
		case RECV_CNVG_ACK_1:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID: %" PRIu64 ", key: %s]", 
						    		__FILE__,__LINE__, "recv_cnvg_ack_1", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", key: %s]\n", 
								  now_time, "recv_cnvg_ack_1", mid, strbuf);	
			}
			break;	

		case RECV_CNVG_ACK_2:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID: %" PRIu64 ", key: %s]", 
						    		__FILE__,__LINE__, "recv_cnvg_ack_2", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", key: %s]\n", 
								  now_time, "recv_cnvg_ack_2", mid, strbuf);	
			}
			break;	

		case CNVG_QUERY_FAIL_1:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));	
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s [PID: %" PRIu64 ", key: %s]",
									__FILE__,__LINE__, "cnvg_query_fail_1", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf),
								  "%20s, %20s : [PID: %" PRIu64 ", key: %s]\n",
								  now_time, "cnvg_query_fail_1", mid, strbuf);
			}
			break;

		case CNVG_QUERY_FAIL_2:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));	
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s [PID: %" PRIu64 ", key: %s]",
									__FILE__,__LINE__, "cnvg_query_fail_2", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf),
								  "%20s, %20s : [PID: %" PRIu64 ", key: %s]\n",
								  now_time, "cnvg_query_fail_2", mid, strbuf);
			}
			break;
			
		case CNVG_FAIL_PROC:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID: %" PRIu64 ", key: %s]", 
						    		__FILE__,__LINE__, "cnvg_fail_proc", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID: %" PRIu64 ", key: %s]\n", 
								  now_time, "cnvg_fail_proc", mid, strbuf);	
			}
			break;

//**********************************************传统节目单向流查询
		case SEND_AV_QUERY_1:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
                        	memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID: %" PRIu64 ", key:%s]", 
									__FILE__,__LINE__, "send_av_query_1", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								   "%20s, %20s : [PID: %" PRIu64 ", key:%s]\n",
								   now_time, "send_av_query_1", mid, strbuf);	
			}
			break;
			
		case SEND_AV_QUERY_2:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
                        	memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID: %" PRIu64 ", key:%s]", 
									__FILE__,__LINE__, "send_av_query_2", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								   "%20s, %20s : [PID: %" PRIu64 ", key:%s]\n",
								   now_time, "send_av_query_2", mid, strbuf);	
			}
			break;

		case RECV_AV_ACK_1:				
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", key:%s]",
									__FILE__,__LINE__, "recv_av_ack_1", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", key:%s]\n", 
								  now_time, "recv_av_ack_1", mid, frg_unit->mid, strbuf);	
			}			
			break;

		case RECV_AV_ACK_2:				
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", key:%s]",
									__FILE__,__LINE__, "recv_av_ack_2", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", key:%s]\n", 
								  now_time, "recv_av_ack_2", mid, frg_unit->mid, strbuf);	
			}			
			break;

		case AV_QUERY_FAIL_1:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", key:%s]",
									__FILE__,__LINE__, "av_query_fail_1", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf),
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", key:%s]\n",
								  now_time, "av_query_fail_1", mid, frg_unit->mid, strbuf);
			}
			break;	

		case AV_QUERY_FAIL_2:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", key:%s]",
									__FILE__,__LINE__, "av_query_fail_2", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf),
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", key:%s]\n",
								  now_time, "av_query_fail_2", mid, frg_unit->mid, strbuf);
			}
			break;				
			
//**********************************************碎片索引查询			
		case SEND_INDEX_QUERY_1:			
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->frg_info[FRAG_UNIT_ID]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->frg_info[FRAG_UNIT_ID]->text, MIN((int)(frg_unit->frg_info[FRAG_UNIT_ID]->text_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID:%" PRIu64 ", FRAG_UNIT_ID: %s]", 
						    		__FILE__,__LINE__, "send_index_query_1", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", FRAG_UNIT_ID: %s]\n", 
								  now_time, "send_index_query_1", mid, strbuf);	
			}
			break;

		case SEND_INDEX_QUERY_2:			
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->frg_info[FRAG_UNIT_ID]!=NULL)
			{
				memcpy(strbuf, (char*)frg_unit->frg_info[FRAG_UNIT_ID]->text, MIN((int)(frg_unit->frg_info[FRAG_UNIT_ID]->text_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID:%" PRIu64 ", FRAG_UNIT_ID: %s]", 
						    		__FILE__,__LINE__, "send_index_query_2", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", FRAG_UNIT_ID: %s]\n", 
								  now_time, "send_index_query_2", mid, strbuf);	
			}
			break;

		case RECV_INDEX_ACK_1:				
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->opt[MEDIA_OPT_INDEX_URL]!=NULL)
			{
                        	memcpy(strbuf, (char*)frg_unit->opt[MEDIA_OPT_INDEX_URL]->opt_value, MIN((int)(frg_unit->opt[MEDIA_OPT_INDEX_URL]->opt_len), strbuflen-1));
			}
			if(frg_unit->frg_info[FRAG_UNIT_ID]!=NULL)
                        {
                                memcpy(strbuf1, (char*)frg_unit->frg_info[FRAG_UNIT_ID]->text, MIN((int)(frg_unit->frg_info[FRAG_UNIT_ID]->text_len), strbuflen-1));
                        }
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", reoffset:%lld, FRAG_UNIT_ID:%s, INDEX_URL:%s]",
									__FILE__,__LINE__, "recv_index_ack_1", mid, frg_unit->mid, frg_unit->re_offset, strbuf1, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", FRAG_UNIT_ID:%s]\n", 
								  now_time, "recv_index_ack_1", mid, frg_unit->mid, strbuf);	
			}			
			break;

		case RECV_INDEX_ACK_2:				
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->frg_info[FRAG_UNIT_ID]!=NULL)
			{
                        	memcpy(strbuf, (char*)frg_unit->frg_info[FRAG_UNIT_ID]->text, MIN((int)(frg_unit->frg_info[FRAG_UNIT_ID]->text_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", FRAG_UNIT_ID:%s]",
									__FILE__,__LINE__, "recv_index_ack_2", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", FRAG_UNIT_ID:%s]\n", 
								  now_time, "recv_index_ack_2", mid, frg_unit->mid, strbuf);	
			}			
			break;

		case INDEX_QUERY_FAIL_1:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->frg_info[FRAG_UNIT_ID]!=NULL)
			{
                        	memcpy(strbuf, (char*)frg_unit->frg_info[FRAG_UNIT_ID]->text, MIN((int)(frg_unit->frg_info[FRAG_UNIT_ID]->text_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s [PID:%" PRIu64 ", FRAG_UNIT_ID: %s]",
									__FILE__,__LINE__, "index_query_fail_1", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf),
								  "%20s, %20s : [PID:%" PRIu64 ", FRAG_UNIT_ID: %s]\n",
								  now_time, "index_query_fail_1", mid, strbuf);
			}
			break;
			
		case INDEX_QUERY_FAIL_2:
			frg_unit = (frag_unit_t*)param1;
			if(frg_unit->frg_info[FRAG_UNIT_ID]!=NULL)
			{
                        	memcpy(strbuf, (char*)frg_unit->frg_info[FRAG_UNIT_ID]->text, MIN((int)(frg_unit->frg_info[FRAG_UNIT_ID]->text_len), strbuflen-1));
			}
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,
									"{%s:%d} %20s [PID:%" PRIu64 ", FRAG_UNIT_ID: %s]",
									__FILE__,__LINE__, "index_query_fail_2", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf),
								  "%20s, %20s : [PID:%" PRIu64 ", FRAG_UNIT_ID: %s]\n",
								  now_time, "index_query_fail_2", mid, strbuf);
			}
			break;
			
		case INDEX_FAIL_PROC:			
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 "]",
									__FILE__,__LINE__, "index_fail_proc", mid);	
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 "]\n", 
								  now_time, "index_fail_proc", mid);	
			}			
			break;		

//**********************************************voip查询			
		case SEND_VOIP_QUERY_1:			
			frg_unit = (frag_unit_t*)param1;
			memcpy(strbuf, (char*)frg_unit->sip_diadata_ID->opt_value, MIN((int)(frg_unit->sip_diadata_ID->opt_len), strbuflen-1));
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]", 
						    		__FILE__,__LINE__, "send_voip_query_1", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]\n", 
								  now_time, "send_voip_query_1", mid, strbuf);	
			}
			break;

		case SEND_VOIP_QUERY_2:			
			frg_unit = (frag_unit_t*)param1;
			memcpy(strbuf, (char*)frg_unit->sip_diadata_ID->opt_value, MIN((int)(frg_unit->sip_diadata_ID->opt_len), strbuflen-1));
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [PID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]", 
						    		__FILE__,__LINE__, "send_voip_query_2", mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]\n", 
								  now_time, "send_voip_query_2", mid, strbuf);	
			}
			break;

		case RECV_VOIP_ACK_1:				
			frg_unit = (frag_unit_t*)param1;			
			memcpy(strbuf, (char*)frg_unit->sip_diadata_ID->opt_value, MIN((int)(frg_unit->sip_diadata_ID->opt_len), strbuflen-1));
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]",
									__FILE__,__LINE__, "recv_voip_ack_1", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]\n", 
								  now_time, "recv_voip_ack_1", mid, frg_unit->mid, strbuf);	
			}			
			break;

		case RECV_VOIP_ACK_2:				
			frg_unit = (frag_unit_t*)param1;			
			memcpy(strbuf, (char*)frg_unit->sip_diadata_ID->opt_value, MIN((int)(frg_unit->sip_diadata_ID->opt_len), strbuflen-1));
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]",
									__FILE__,__LINE__, "recv_voip_ack_2", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]\n", 
								  now_time, "recv_voip_ack_2", mid, frg_unit->mid, strbuf);	
			}			
			break;

		case VOIP_QUERY_FAIL_1:				
			frg_unit = (frag_unit_t*)param1;			
			memcpy(strbuf, (char*)frg_unit->sip_diadata_ID->opt_value, MIN((int)(frg_unit->sip_diadata_ID->opt_len), strbuflen-1));
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]",
									__FILE__,__LINE__, "voip_query_fail_1", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]\n", 
								  now_time, "voip_query_fail_1", mid, frg_unit->mid, strbuf);	
			}			
			break;

		case VOIP_QUERY_FAIL_2:				
			frg_unit = (frag_unit_t*)param1;			
			memcpy(strbuf, (char*)frg_unit->sip_diadata_ID->opt_value, MIN((int)(frg_unit->sip_diadata_ID->opt_len), strbuflen-1));
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]",
									__FILE__,__LINE__, "voip_query_fail_2", mid, frg_unit->mid, strbuf);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [PID:%" PRIu64 ", MID:%" PRIu64 ", VOIP_SIP_DIADATA_ID: %s]\n", 
								  now_time, "voip_query_fail_2", mid, frg_unit->mid, strbuf);	
			}			
			break;	
			
		case SEND_AV_DEDUP_QUERY:			
			mdi = (media_t*)param1;
			if(mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]!=NULL)
			{
				MESA_handle_runtime_log(g_frag_run.dedup_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
							    		"{%s:%d} %20s [MID:%" PRIu64 ", td_query:%hhu, TD: %s, addr: %s url: %s]", 
							    		__FILE__,__LINE__, "soqav_dedup_query", mid, mdi->td_query, mdi->td, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value);
			}
			else
			{
				MESA_handle_runtime_log(g_frag_run.dedup_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
														"{%s:%d} %20s [MID:%" PRIu64 ", td_query:%hhu, TD: %s, addr: %s, url: ]", 
														__FILE__,__LINE__, "soqav_dedup_query", mid, mdi->td_query, mdi->td, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value);							
			}
			if(trace_flag)
			{
				if(mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]!=NULL)
				{
					buflen = snprintf(buf, sizeof(buf), 
									  "%20s, %20s : [MID:%" PRIu64 ", td_query:%hhu, TD: %s, addr: %s url: %s]\n", 
									  now_time, "soqav_dedup_query",  mid, mdi->td_query, mdi->td, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value);	
				}
				else
				{
					buflen = snprintf(buf, sizeof(buf), 
														  "%20s, %20s : [MID:%" PRIu64 ", td_query:%hhu, TD: %s, addr: %s url: ]\n", 
														  now_time, "soqav_dedup_query",  mid, mdi->td_query, mdi->td, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value);				
				}
			}
			break;			

		case RECV_AV_DEDUP_ACK:
			MESA_handle_runtime_log(g_frag_run.dedup_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [MID:%" PRIu64 ", TD: %s, result:0x%x]", 
						    		__FILE__,__LINE__, "soqav_query_callback", mid, (char*)param1, *(int*)param2);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID:%" PRIu64 ", TD: %s, result:0x%x]\n", 
								  now_time, "soqav_query_callback",  mid, (char*)param1, *(int*)param2);	
			}				
			break;

		case RECV_AV_DEDUP_ACK_KNOWN:
			MESA_handle_runtime_log(g_frag_run.dedup_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [MID:%" PRIu64 ", TD: %s, result:0x%x]", 
						    		__FILE__,__LINE__, "soqav_query_callback_known", mid, (char*)param1, *(int*)param2);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID:%" PRIu64 ", TD: %s, result:0x%x]\n", 
								  now_time, "soqav_query_callback_known",  mid, (char*)param1, *(int*)param2);	
			}				
			break;
			
		case RECV_AV_DEDUP_ACK_MULTI:			
			mdi = (media_t*)param1;
			MESA_handle_runtime_log(g_frag_run.dedup_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [MID:%" PRIu64 ", TD: %s, cpz_ip:%s, addr: %s, url: %s]", 
						    		__FILE__,__LINE__, "soqav_query_callback_multisrc", mid, mdi->td, (char*)param2, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID:%" PRIu64 ", TD: %s, cpz_ip:%s, addr: %s, url: %s]", 
								  now_time, "soqav_query_callback_multisrc",  mid,  mdi->td, (char*)param2, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value);
			}				
			break;

		case AV_DEDUP_QUERY_TIMEOUT:
			MESA_handle_runtime_log(g_frag_run.dedup_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [MID:%" PRIu64 "]", 
						    		__FILE__,__LINE__, "soqav_query_timeout", mid);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID:%" PRIu64 ",]\n", 
								  now_time, "soqav_query_timeout",  mid);	
			}				
			break;

		case AV_DEDUP_REPORT:
			MESA_handle_runtime_log(g_frag_run.dedup_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} %20s [MID:%" PRIu64 ", TD: %s, cur_len:%" PRIu64 ", config:%u]", 
						    		__FILE__,__LINE__, "soqav_dedup_report", mid, (char*)param1, *(uint64_t*)param2, param3);
			if(trace_flag)
			{
				buflen = snprintf(buf, sizeof(buf), 
								  "%20s, %20s : [MID:%" PRIu64 ", TD: %s, cur_len:%" PRIu64 " config:%u]\n", 
								  now_time, "soqav_dedup_report",  mid, (char*)param1, *(uint64_t*)param2, param3);	
			}				
			break;
			
		default:
			break;		
	}

	if(trace_flag)
	{
		snprintf(filename, MAX_PATH_LEN, "%s/%" PRIu64 ".log", g_frag_cfg.trace_filepath, mid);
		fp = fopen(filename, "a+");
		if(NULL!=fp)
		{
			fwrite(buf, buflen,1, fp);
			fclose(fp);
		}
	}
}

/*opt_addr is printaddr result*/
char* merge_addr(char* addr_buf, uint32_t* addr_buflen, char* opt_addr, uint32_t opt_addrlen, int thread_seq)
{	
	char* 		ip = opt_addr;
	uint32_t 	ip_len = strlen(ip); //printaddr 包括\0
	if(NULL==addr_buf)
	{			
		addr_buf = (char*)malloc(ip_len);
		memcpy(addr_buf, ip, ip_len);
		*addr_buflen = ip_len;
	}
	else
	{			
		addr_buf = (char*)realloc(addr_buf, (*addr_buflen)+strlen(";")+ip_len);
		memcpy(addr_buf+*addr_buflen, ";", strlen(";"));
		memcpy(addr_buf+*addr_buflen+strlen(";"), ip, ip_len);
		*addr_buflen += +strlen(";")+ip_len;
	}	
	return addr_buf;
}

int frag_unit_removal(media_t* mdi, uint64_t reoffset)
{
	return 0;
	int array_x = 0, bit_x = 0;
#if 1
	int i=0;
	for(i=0;(mdi->repeat_reoffset[i]!=-1&&i<KEEP_REOFFSET_MAXNUM);i++)
	{
		if(mdi->repeat_reoffset[i]==(int64_t)reoffset) return 1;
	}	
	mdi->repeat_reoffset[mdi->repeat_reoffset_idx_last] = (int64_t)reoffset;		
	mdi->repeat_reoffset_idx_last = (mdi->repeat_reoffset_idx_last+1%KEEP_REOFFSET_MAXNUM);	
	return 0;
#else
	/*超过限制，认为不重复*/
	if(reoffset>KEEP_REOFFSET_MAXNUM) return 0;
	array_x = reoffset>>3;
	bit_x = reoffset & 7;
	if(GET_BIT(array_x,bit_x)) return 1;
	SET_BIT(array_x, bit_x);
	return 0;
#endif
}

int add_to_seg(IVI_seg_t* seg, OFFSET_TYPE left, OFFSET_TYPE right)
{
	seg->left = MIN(seg->left, left);
	seg->right = MAX(seg->right, right);
	return 0;
}

/*
*return  -1:error 
*return 0: delete frag
return 1 : keep frag to add wait_lq
return 2 : new frag to add wait_lq
*
*/
/*IVI removal IVI节点归并*/
int frag_removal_and_merge(IVI_t* ivi, frag_ivi_info_t* frag_ivi_info)
{	
	frag_in_t*			frg = frag_ivi_info->frg;	
	IVI_seg_t			contain_segs[FRAG_CONTAIN_MAXNUM];
	int					contain_segs_cnt = 0;
	IVI_seg_t** 		overlap_segs; //声明指针数组，不用分配内存	
	int 				overlap_cnt = 0;
	int 				break_flag = 0;		
	OFFSET_TYPE 		left_before = frg->offset;
	OFFSET_TYPE 		right_before = frg->offset+frg->datalen-1;
	OFFSET_TYPE 		left = left_before;
	OFFSET_TYPE 		right = right_before;
	
	MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} frag_removal before : [ MID: %llu, PID: %llu, frag_left:%u, frag_right:%u]", 
							__FILE__,__LINE__, frg->mid, frg->pid, left_before, right_before);
	
	if((overlap_cnt = IVI_query(ivi, left_before, right_before, &overlap_segs)) < 0)
	{
		MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} IVI_query error: [mid: %llu]", 
								__FILE__,__LINE__, frg->mid);
		free(overlap_segs); 
		return -1;
	}
	/*create curseg*/
	IVI_seg_t* seg_before = IVI_seg_malloc(left_before, right_before, NULL);	
	IVI_seg_t* seg_to_add = IVI_seg_malloc(left_before, right_before, NULL);
	int seq_add_flag = 1;
	Relation_t rea;
	OFFSET_TYPE overlap_left = 0;
	OFFSET_TYPE overlap_right = 0;		
	for(int i=0;i<overlap_cnt && !break_flag;i++)
	{
		rea = IVI_relative_position(seg_before, overlap_segs[i]);
		overlap_left = overlap_segs[i]->left;
		overlap_right = overlap_segs[i]->right;
		switch(rea)
		{					
			case LEFT_OVERLAP:
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval LEFT_OVERLAP: [ MID: %llu, PID: %llu, seg_left:%u, seg_right:%u ,frag_left:%u, frag_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_left, overlap_right, left, right);					
				right = overlap_left-1;	
				add_to_seg(seg_to_add, overlap_left, overlap_right);	
				seq_add_flag =1; 
				/*删除之前的节点*/
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_remove succ: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_segs[i]->left, overlap_segs[i]->right);
				IVI_remove(ivi, overlap_segs[i]);
				IVI_seg_free(overlap_segs[i], NULL, NULL);
				break;
				
			/*被包含，不需要此frag*/
			case CONTAINED:				
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval CONTAINED: [ MID: %llu, PID: %llu, seg_left:%u, seg_right:%u ,frag_left:%u, frag_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_left, overlap_right, left, right);
				left = 0;
				right = 0;
				break_flag = 1;				
				seq_add_flag = 0; 
				break;

			/*包含已有区间，删除并释放已有区间*/
			case CONTAIN: 
				if(contain_segs_cnt<FRAG_CONTAIN_MAXNUM)
				{
					MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval CONTAIN: [ MID: %llu, PID: %llu, seg_left:%u, seg_right:%u ,frag_left:%u, frag_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_left, overlap_right, left, right);			
					contain_segs[contain_segs_cnt].left = overlap_left;
					contain_segs[contain_segs_cnt].right = overlap_right;
					contain_segs_cnt++;
					add_to_seg(seg_to_add, overlap_left, overlap_right);	
					seq_add_flag = 1; 
					/*可以被删除，因为当前frag会包含该区间*/
					MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_remove succ: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_segs[i]->left, overlap_segs[i]->right);				
					IVI_remove(ivi, overlap_segs[i]);
					IVI_seg_free(overlap_segs[i], NULL, NULL);						
				}
				else
				{
					MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
											"{%s:%d} frag_moval CONTAIN fatal bigger than FRAG_CONTAIN_MAXNUM: [ MID: %llu, PID: %llu, frag_left:%u, frag_right:%u]", 
											__FILE__,__LINE__, frg->mid, frg->pid, left, right);
					/*delete this frag*/					
					left = 0;
					right = 0;
					break_flag = 1;					
					seq_add_flag = 0;					
				}								
				break;
				
			case RIGHT_OVERLAP:				
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval RIGHT_OVERLAP: [ MID: %llu, PID: %llu, seg_left:%u, seg_right:%u ,frag_left:%u, frag_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_left, overlap_right, left, right);
				left = overlap_right+1;
				add_to_seg(seg_to_add, overlap_left, overlap_right);	
				seq_add_flag =1; 
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_remove succ: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_segs[i]->left, overlap_segs[i]->right);
				IVI_remove(ivi, overlap_segs[i]);
				IVI_seg_free(overlap_segs[i], NULL, NULL);
				break;
				
			/*以下情况不应该发生*/
			case LEFT_NO_OVERLAP:
				left = 0;
				right = 0;
				break_flag = 1;				
				seq_add_flag = 0; 		
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_relative_position error LEFT_NO_OVERLAP: [ MID: %llu, PID: %llu, aboffset:%llu, datalen:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, frg->offset, frg->datalen);
				break;	
			case RIGHT_NO_OVERLAP:					
				left = 0;
				right = 0;
				break_flag = 1;				
				seq_add_flag = 0; 		
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_relative_position error RIGHT_NO_OVERLAP: [ MID: %llu, PID: %llu, aboffset:%llu, datalen:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, frg->offset, frg->datalen);
				break;	
			default:				
				left = 0;
				right = 0;
				break_flag = 1;				
				seq_add_flag = 0; 	
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_relative_position error: [ MID: %llu, PID: %llu, aboffset:%llu, datalen:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, frg->offset, frg->datalen);
				break;				
		}
	}

	free(overlap_segs);   //释放指针数组
	IVI_seg_free(seg_before, NULL, NULL);

	/*更新IVI节点*/
	if(seq_add_flag)
	{
		/*frag加入到IVI区间里面*//*插入失败，该frag依然入队列!!!!!!!*/
		if(-1==IVI_insert(ivi, seg_to_add))
		{
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} IVI_insert error: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
									__FILE__,__LINE__, frg->mid, frg->pid, seg_to_add->left, seg_to_add->right);
		}
		else
		{
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} IVI_insert succ: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
									__FILE__,__LINE__, frg->mid, frg->pid, seg_to_add->left, seg_to_add->right);
			/*merge IVI 节点*/	
			IVI_seg_t* prev_seg = IVI_prev_continuous_seg(seg_to_add);	
			IVI_seg_t* next_seg = IVI_next_continuous_seg(seg_to_add);			
			IVI_seg_t* new_seg = IVI_seg_malloc(seg_to_add->left, seg_to_add->right, NULL);
			int 	   del_seg_flag = 0;
			if(NULL!=prev_seg)
			{
				if(prev_seg->right+1==seg_to_add->left)
				{
					MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} IVI_remove succ: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
									__FILE__,__LINE__, frg->mid, frg->pid, prev_seg->left, prev_seg->right);
					new_seg->left = prev_seg->left;					
					IVI_remove(ivi, prev_seg);
					IVI_seg_free(prev_seg, NULL, NULL);
					del_seg_flag = 1;			
				}
			}
			if(NULL!=next_seg)
			{
				if(seg_to_add->right+1==next_seg->left)
				{
					MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} IVI_remove succ: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
									__FILE__,__LINE__, frg->mid, frg->pid, next_seg->left, next_seg->right);
					new_seg->right = next_seg->right;						
					IVI_remove(ivi, next_seg);
					IVI_seg_free(next_seg, NULL, NULL);
					del_seg_flag = 1;				
				}
			}
			if(del_seg_flag)
			{
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} IVI_remove succ: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
									__FILE__,__LINE__, frg->mid, frg->pid, seg_to_add->left, seg_to_add->right);
				IVI_remove(ivi, seg_to_add);
				IVI_seg_free(seg_to_add, NULL, NULL);
				if(-1==IVI_insert(ivi, new_seg))
				{
					MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
											"{%s:%d} IVI_insert error: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
											__FILE__,__LINE__, frg->mid, frg->pid, new_seg->left, new_seg->right);
				}
				else
				{
					MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} IVI_insert succ: [ MID: %llu, PID: %llu, left:%u, right:%u ]", 
									__FILE__,__LINE__, frg->mid, frg->pid, new_seg->left, new_seg->right);
				}
			}
			else
			{
				IVI_seg_free(new_seg, NULL, NULL);
			}
		}
	}
	else
	{
		IVI_seg_free(seg_to_add, NULL, NULL);
	}	
	
	/*修正frag*/
	if(right>left && contain_segs_cnt==0)
	{			
		/*有去重*/
		if(left!=left_before || right!=right_before)
		{			
			/*修正frag*/
			frg->datalen = right-left+1;			
			frg->offset = left;
			char* data = (char*)malloc(frg->datalen);
			memcpy(data, frg->data+left-left_before, frg->datalen);
			free(frg->data);
			frg->data = data;	
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} frag_moval after overlop: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,after_left:%u, after_right:%u]", 
									__FILE__,__LINE__, frg->mid, frg->pid, left_before, right_before, left, right);			
		}
		else
		{			
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval after new: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,after_left:%u, after_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, left_before, right_before, left, right);
		}
		return 1;
	}
	else if(right>left && contain_segs_cnt!=0)
	{
		int i=0,frg_array_cnt=0;		
		OFFSET_TYPE left_forlq = left;
		/*处理CONTAIN的区间，因为此时左边界和右边界已经确定*/
		for(i=0;i<contain_segs_cnt;i++)
		{
			if(contain_segs[i].left-left_forlq>0)
			{				
				frag_ivi_info->frg_array[frg_array_cnt] = (frag_in_t*)malloc(sizeof(frag_in_t));
				frag_ivi_info->frg_array[frg_array_cnt]->data = (char*)malloc(contain_segs[i].left-left_forlq);
				frag_ivi_info->frg_array[frg_array_cnt]->offset = left_forlq;				
				memcpy(frag_ivi_info->frg_array[frg_array_cnt]->data, frg->data+left_forlq-left_before, contain_segs[i].left-left_forlq);
				frag_ivi_info->frg_array[frg_array_cnt]->datalen = contain_segs[i].left-left_forlq;	
				frg_array_cnt++;
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} frag_moval after contain: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,after_left:%u, after_right:%u]", 
								__FILE__,__LINE__,frg->mid, frg->pid, left_before, right_before, left_forlq, contain_segs[i].left-1);
			}			
			left_forlq = contain_segs[i].right+1;
		}
		/*处理最后一个被包含的区间的右边部分*/
		if(right>left_forlq)
		{
			frag_ivi_info->frg_array[frg_array_cnt] = (frag_in_t*)malloc(sizeof(frag_in_t));
			frag_ivi_info->frg_array[frg_array_cnt]->data = (char*)malloc(right-left_forlq+1);
			frag_ivi_info->frg_array[frg_array_cnt]->offset = left_forlq;		
			memcpy(frag_ivi_info->frg_array[frg_array_cnt]->data, frg->data+left_forlq-left_before, right-left_forlq+1);
			frag_ivi_info->frg_array[frg_array_cnt]->datalen = right-left_forlq+1; 
			frg_array_cnt++;
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} frag_moval after contain: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,after_left:%u, after_right:%u]", 
								__FILE__,__LINE__,frg->mid, frg->pid, left_before, right_before, left_forlq, right);		
		}
		frag_ivi_info->frg_array_cnt = frg_array_cnt;
		return 2;
	}
	else
	{				
		MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} frag_moval after contained: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,after_left:%u, after_right:%u]", 
								__FILE__,__LINE__,frg->mid, frg->pid, left_before, right_before, left, right);
		return 0;		
	}
	return 0;
}

/*
*return  -1:error 
*return 0: delete frag
return 1 : keep frag to add wait_lq
return 2 : new frag to add wait_lq
*
*/
/*IVI removal IVI节点没有归并*/
int frag_removal(IVI_t* ivi, frag_ivi_info_t* frag_ivi_info)
{	
	frag_in_t*			frg = frag_ivi_info->frg;	
	IVI_seg_t			contain_segs[FRAG_CONTAIN_MAXNUM];
	int					contain_segs_cnt = 0;
	IVI_seg_t** 		overlap_segs; //声明指针数组，不用分配内存	
	int 				overlap_cnt = 0;
	int 				break_flag = 0;		
	OFFSET_TYPE 		left_before = frg->offset;
	OFFSET_TYPE 		right_before = frg->offset+frg->datalen-1;
	OFFSET_TYPE 		left = left_before;
	OFFSET_TYPE 		right = right_before;
	
	MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} frag_removal : [ MID: %llu, PID: %llu, frag_left:%u, frag_right:%u]", 
							__FILE__,__LINE__, frg->mid, frg->pid, left_before, right_before);
	
	if((overlap_cnt = IVI_query(ivi, left_before, right_before, &overlap_segs)) < 0)
	{
		MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} IVI_query error: [mid: %llu]", 
								__FILE__,__LINE__, frg->mid);
		free(overlap_segs); 
		return -1;
	}
	/*create curseg*/
	IVI_seg_t* seg_before = IVI_seg_malloc(left_before, right_before, NULL);	
	Relation_t rea;
	OFFSET_TYPE overlap_left = 0;
	OFFSET_TYPE overlap_right = 0;		
	for(int i=0;i<overlap_cnt && !break_flag;i++)
	{
		rea = IVI_relative_position(seg_before, overlap_segs[i]);
		overlap_left = overlap_segs[i]->left;
		overlap_right = overlap_segs[i]->right;
		switch(rea)
		{					
			case LEFT_OVERLAP:
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval LEFT_OVERLAP: [ MID: %llu, PID: %llu, seg_left:%u, seg_right:%u ,frag_left:%u, frag_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_left, overlap_right, left, right);					
				right = overlap_left-1;					
				break;
				
			/*被包含，不需要此frag*/
			case CONTAINED:				
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval CONTAINED: [ MID: %llu, PID: %llu, seg_left:%u, seg_right:%u ,frag_left:%u, frag_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_left, overlap_right, left, right);
				left = 0;
				right = 0;
				break_flag = 1;
				break;

			/*包含已有区间，删除并释放已有区间*/
			case CONTAIN: 
				if(contain_segs_cnt<FRAG_CONTAIN_MAXNUM)
				{
					contain_segs[contain_segs_cnt].left = overlap_left;
					contain_segs[contain_segs_cnt].right = overlap_right;
					contain_segs_cnt++;
					/*可以被删除，因为当前frag会包含该区间*/
					IVI_remove(ivi, overlap_segs[i]);
					IVI_seg_free(overlap_segs[i], NULL, NULL);
					MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval CONTAIN: [ MID: %llu, PID: %llu, seg_left:%u, seg_right:%u ,frag_left:%u, frag_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_left, overlap_right, left, right);
				}
				else
				{
					MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
											"{%s:%d} frag_moval CONTAIN fatal bigger than FRAG_CONTAIN_MAXNUM: [ MID: %llu, PID: %llu, frag_left:%u, frag_right:%u]", 
											__FILE__,__LINE__, frg->mid, frg->pid, left, right);
					/*delete this frag*/					
					left = 0;
					right = 0;
					break_flag = 1;
				}				
				break;
				
			case RIGHT_OVERLAP:				
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} frag_moval RIGHT_OVERLAP: [ MID: %llu, PID: %llu, seg_left:%u, seg_right:%u ,frag_left:%u, frag_right:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, overlap_left, overlap_right, left, right);
				left = overlap_right+1;
				break;
				
			/*以下情况不应该发生*/
			case LEFT_NO_OVERLAP:
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_relative_position error LEFT_NO_OVERLAP: [ MID: %llu, PID: %llu, aboffset:%llu, datalen:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, frg->offset, frg->datalen);
				break;	
			case RIGHT_NO_OVERLAP:	
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_relative_position error RIGHT_NO_OVERLAP: [ MID: %llu, PID: %llu, aboffset:%llu, datalen:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, frg->offset, frg->datalen);
				break;	
			default:
				MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} IVI_relative_position error: [ MID: %llu, PID: %llu, aboffset:%llu, datalen:%u]", 
										__FILE__,__LINE__, frg->mid, frg->pid, frg->offset, frg->datalen);
				break;				
		}
	}

	free(overlap_segs);   //释放指针数组
	/*把本次的frag加入到IVI中*/
	if(right>left && contain_segs_cnt==0)
	{			
		IVI_seg_t* seg = NULL;
		/*有去重*/
		if(left!=left_before || right!=right_before)
		{			
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} frag_moval frag_overlop: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,after_left:%u, after_right:%u]", 
									__FILE__,__LINE__, frg->mid, frg->pid, left_before, right_before, left, right);
			seg = IVI_seg_malloc(left, right, NULL);
			IVI_seg_free(seg_before, NULL, NULL);
			/*修正frag*/
			frg->datalen = right-left+1;			
			frg->offset = left;
			char* data = (char*)malloc(frg->datalen);
			memcpy(data, frg->data+left-left_before, frg->datalen);
			free(frg->data);
			frg->data = data;			
		}
		else
		{
			seg = seg_before;
		}		
		/*frag加入到IVI区间里面*//*插入失败，该frag依然入队列!!!!!!!*/
		if(-1==IVI_insert(ivi, seg))
		{
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} frag_moval IVI_insert_overlap_error: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,insert_left:%u, insert_right:%u]", 
									__FILE__,__LINE__, frg->mid, frg->pid, left_before, right_before, left, right);
		}
		else
		{
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} frag_moval IVI_insert_overlap_succ: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,insert_left:%u, insert_right:%u]", 
									__FILE__,__LINE__, frg->mid, frg->pid, left_before, right_before, left, right);

		}
		return 1;
	}
	else if(right>left && contain_segs_cnt!=0)
	{
		int i=0,frg_array_cnt=0;		
		OFFSET_TYPE left_forlq = left;
		/*处理CONTAIN的区间，因为此时左边界和右边界已经确定*/
		for(i=0;i<contain_segs_cnt;i++)
		{
			if(contain_segs[i].left-left_forlq>0)
			{				
				frag_ivi_info->frg_array[frg_array_cnt] = (frag_in_t*)malloc(sizeof(frag_in_t));
				frag_ivi_info->frg_array[frg_array_cnt]->data = (char*)malloc(contain_segs[i].left-left_forlq);
				frag_ivi_info->frg_array[frg_array_cnt]->offset = left_forlq;				
				memcpy(frag_ivi_info->frg_array[frg_array_cnt]->data, frg->data+left_forlq-left_before, contain_segs[i].left-left_forlq);
				frag_ivi_info->frg_array[frg_array_cnt]->datalen = contain_segs[i].left-left_forlq;	
				frg_array_cnt++;				
			}			
			left_forlq = contain_segs[i].right+1;
		}
		/*处理最后一个被包含的区间的右边部分*/
		if(right>left_forlq)
		{
			frag_ivi_info->frg_array[frg_array_cnt] = (frag_in_t*)malloc(sizeof(frag_in_t));
			frag_ivi_info->frg_array[frg_array_cnt]->data = (char*)malloc(right-left_forlq+1);
			frag_ivi_info->frg_array[frg_array_cnt]->offset = left_forlq;		
			memcpy(frag_ivi_info->frg_array[frg_array_cnt]->data, frg->data+left_forlq-left_before, right-left_forlq+1);
			frag_ivi_info->frg_array[frg_array_cnt]->datalen = right-left_forlq+1; 
			frg_array_cnt++;
		}
		frag_ivi_info->frg_array_cnt = frg_array_cnt;
		IVI_seg_t* seg = IVI_seg_malloc(left, right, NULL);
		IVI_seg_free(seg_before, NULL, NULL);
		/*frag加入到IVI区间里面*//*插入失败，该frag依然入队列!!!!!!!*/
		if(-1==IVI_insert(ivi, seg))
		{
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} frag_moval IVI_insert_contain_error: [ MID: %llu, PID: %llu, insert_left:%u, insert_right:%u]", 
									__FILE__,__LINE__, frg->mid, frg->pid, left, right);
		}
		else
		{
			MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} frag_moval IVI_insert_contain_succ: [ MID: %llu, PID: %llu, insert_left:%u, insert_right:%u]", 
									__FILE__,__LINE__, frg->mid, frg->pid, left, right);

		}
		return 2;
	}
	else
	{				
		IVI_seg_free(seg_before, NULL, NULL);
		MESA_handle_runtime_log(frag_rssb.media_logger, RLOG_LV_DEBUG, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} frag_moval frag_contained: [ MID: %llu, PID: %llu, before_left:%u, before_right:%u ,after_left:%u, after_right:%u]", 
								__FILE__,__LINE__,frg->mid, frg->pid, left_before, right_before, left, right);
		return 0;		
	}
	return 0;
}

/*use AV_make_porg_id*/
uint64_t make_mid(char * key, unsigned int size, unsigned char type)
{
	unsigned long long      hash = 0;
	unsigned char           *p      = NULL;
	unsigned int            hashH   = 1610612741L, hashL = 2166136261L;
	unsigned int            i=0, ch=0;	
	unsigned char* 			key_tmp=NULL;	
	if(size <= 0)
	{
		return(idMaskIndex[0]);
	}
	//force convert key from uint8_t* to uint* will over read 3 bytes,make a copy to be safe;
	key_tmp=(unsigned char*)calloc(sizeof(unsigned char),size+4);
	memcpy(key_tmp,key,size);
	p=key_tmp;
	i = size;
	while(i--)
	{
		ch = (unsigned int)(*p++);
		if(ch != 0)
		{
			hashH   = (hashH ^ ch) * 12255871;
			hashL   = (hashL ^ ch) * 16777619;
		}
	}
	hash = ( (unsigned long long)hashH << 32) | (unsigned long long)hashL;
	hash = (hash & 0x0FFFFFFFFFFFFFFF) | idMaskIndex[type];
	free(key_tmp);
	return(hash);
}

int save_qd_info(qd_info_t qd_info[], uint32_t qd_info_maxnum, uint8_t* qd_info_idx_last, uint32_t capip, uint64_t mid)
{
	int ip_set = 0;

	for(uint32_t i=0; i<qd_info_maxnum; i++)
	{
		if(capip == qd_info[i].cap_ip)
		{
			ip_set = 1;
			break;
		}
	}
	if(!ip_set)
	{			
		qd_info[*qd_info_idx_last].cap_ip = capip;	
		qd_info[*qd_info_idx_last].mid = mid;		
		*qd_info_idx_last = (*qd_info_idx_last+1)%qd_info_maxnum;
		return 1;
	}		
	return 0;
}

int get_serverIP(const char *addr, uint32_t addr_len, char* d_ip)
{	
	touple4_type_t touple4;	
	
	if(sscanf(addr, "%[0-9.],%hu>%[0-9.],%hu", touple4.sip, &touple4.sport, touple4.dip, &touple4.dport) == 4)
	{
		touple4.addr_type = 4; 
	}	
	else if(sscanf(addr, "%[0-9A-Fa-f:.],%hu>%[0-9A-Fa-f:.],%hu",  touple4.sip, &touple4.sport, touple4.dip, &touple4.dport) == 4)
	{
		touple4.addr_type = 6; 
	}	
	memcpy(d_ip, touple4.dip, sizeof(touple4.dip));
	return 0;
}

void send_ack_to_qd(frag_in_t* frg, uint32_t src_ip, int thread_seq)
{
	/*ack for sapp*/
	if(g_frag_cfg.ack_switch)
	{		
		memcpy(g_frag_run.fb_ack_hdr[thread_seq]->prog_id, (char*)(&frg->mid), 8);
		g_frag_run.fb_ack_hdr[thread_seq]->ack_offset = frg->offset+frg->datalen;
		bizman_send(g_frag_run.answer_sapp_bizman, 
					thread_seq, 
					src_ip,
					g_frag_cfg.qd_msg_port,  
					(const char*)g_frag_run.fb_ack_buf[thread_seq],
					MSG_HEADER_LEN + sizeof(msg_data_ack_t),
				    1, BIZMAN_SMOOTH_DEST|BIZMAN_PUSH_SEND);
		atomic_inc(&g_frag_stat.stat_info[RESP_CHARACTER_ACK][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[RESP_CHARACTER_ACK][TOTAL_BYTES], MSG_HEADER_LEN+sizeof(msg_data_ack_t)); 
	}
}

int caculate_md5(const char *meta, unsigned long meta_len, const char *data, unsigned long len, char *result, unsigned int result_len)
{
	MD5_CTX c;
	unsigned char md5[17]={0};
	int i;

	if(result_len < 33)
		return -1;

	if(MD5_Init(&c) != 1)
		return -1;
	if(meta!=NULL && MD5_Update(&c, meta, meta_len) != 1)
		return -1;
	if(data!=NULL && MD5_Update(&c, data, len) != 1)
		return -1;
	if(MD5_Final(md5, &c) != 1)
		return -1;
	
	for(i=0; i<16; i++)
	{
		sprintf(result + i*2, "%02x", md5[i]);
	}
	return 0;
}

