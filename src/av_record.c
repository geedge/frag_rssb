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
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stddef.h>

#include "MESA_handle_logger.h"
#include "stream_fuzzy_hash.h"

#include "av_record.h"
#include "common.h"
#include "log.h"
#include "frag_reassembly_in.h"
#include "frag_proc.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;

void free_av_record(av_record_t* av_record)
{
	if(NULL!=av_record->url)
	{
		free(av_record->url);
	}
	if(NULL!=av_record->referer)
	{
		free(av_record->referer);
	}
	if(NULL!=av_record->addr)
	{
		free(av_record->addr);
	}
	free(av_record);
}

int av_record_open_file(int tid)
{
	char			str_time[64];	
	time_t			currTime;
	struct tm*		now;	
	char			filename[MAX_PATH_LEN] = {0};

	time(&currTime);
	now = localtime(&currTime);
	strftime(str_time, sizeof(str_time), "%Y-%m-%d-%H-%M-%S", now); 

	snprintf(filename, sizeof(filename), "%s/av_record_capip%u_tid%d_%s", g_frag_cfg.avrecord_filepath, g_frag_cfg.local_ip_nr, tid ,str_time);
	g_frag_run.av_record_curfile[tid] = fopen(filename,"a+");
	if(NULL==g_frag_run.av_record_curfile[tid])
	{
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} av_record open filr error: [filename:%s]",	__FILE__,__LINE__, filename);
		return -1;
	}
	return 0;
}

void* av_record_thread(void *param)
{
	av_record_t*	av_record = NULL;
	long			av_record_len = sizeof(av_record);		
	int				rec = 0;
	long			tid = (long)param;
		
	while(1)
	{	
		av_record = NULL;
		rec = MESA_lqueue_get_head(g_frag_run.av_record_lq[tid], &av_record, &av_record_len); 		
		if(0!=rec)
		{
			usleep(10);
			continue;
		}
		atomic_inc(&g_frag_stat.sysinfo_stat[AV_RECORD_QUEUE][QUEUE_OUT]);
		/*first*/
		if(NULL==g_frag_run.av_record_curfile[tid])
		{	
			if(av_record_open_file(tid)<0)
			{	
				free_av_record(av_record);
				continue;
			}
		}		
		/*file full*/	
		if(g_frag_run.av_record_curcnt[tid]>=g_frag_cfg.avrecord_maxnum)
		{
			fclose(g_frag_run.av_record_curfile[tid]);
			if(av_record_open_file(tid)<0)
			{				
				free_av_record(av_record);
				continue;
			}
			g_frag_run.av_record_curcnt[tid] = 0;
		}	
		/*capip*/
		char capip4[18] = {0};
		inet_ntop(AF_INET,(const void *)&av_record->capIP,capip4,18); 
		/*av record write to file*/
		/*mid*//*media_type*/	/*media_len*/
		fprintf(g_frag_run.av_record_curfile[tid],"%lu;0x%02x;%lu;%s", av_record->mid, 
																	 	  av_record->media_type,
																	      av_record->media_len,
																	      capip4);
		/*url*/
		if(NULL!=av_record->url)
		{	
			fwrite(";", 1, 1, g_frag_run.av_record_curfile[tid]);		
			fwrite(av_record->url, av_record->url_len, 1, g_frag_run.av_record_curfile[tid]);				
		}
		else
		{
			fprintf(g_frag_run.av_record_curfile[tid],";NULL");

		}
		/*referer*/
		if(NULL!=av_record->referer)
		{	
			fwrite(";", 1, 1, g_frag_run.av_record_curfile[tid]);		
			fwrite(av_record->referer, av_record->referer_len, 1, g_frag_run.av_record_curfile[tid]);				
		}
		else
		{
			fprintf(g_frag_run.av_record_curfile[tid],";NULL");

		}		
		fwrite("\n", 1, 1, g_frag_run.av_record_curfile[tid]);		
		fflush(g_frag_run.av_record_curfile[tid]);
		
		g_frag_run.av_record_curcnt[tid]++;		
		atomic_inc(&g_frag_stat.media_stat[LOG_AV_RECORD]);
		free_av_record(av_record);
	}
	return NULL;
}

int av_record(frag_unit_t* frg_unit)
{
	if(g_frag_cfg.avrecord_switch!=AV_RECORD_TYPR_BASIC) return 0;	

	av_record_t* av_record = (av_record_t*)calloc(1, sizeof(av_record_t));
	av_record->mid = frg_unit->mid;
	av_record->media_len = frg_unit->media_len;
	av_record->media_type = frg_unit->media_type;
	av_record->capIP = frg_unit->capIP;
	
	/*addr*/
	if(NULL!=frg_unit->opt[MEDIA_OPT_ADDR])
	{
		av_record->addr = (char*)malloc(frg_unit->opt[MEDIA_OPT_ADDR]->opt_len);
		av_record->addr_len = frg_unit->opt[MEDIA_OPT_ADDR]->opt_len;
		memcpy(av_record->addr, frg_unit->opt[MEDIA_OPT_ADDR]->opt_value, frg_unit->opt[MEDIA_OPT_ADDR]->opt_len);
	}
	/*url*/
	if(NULL!=frg_unit->opt[MEDIA_OPT_URL])
	{
		av_record->url = (char*)malloc(frg_unit->opt[MEDIA_OPT_URL]->opt_len);
		av_record->url_len = frg_unit->opt[MEDIA_OPT_URL]->opt_len;
		memcpy(av_record->url, frg_unit->opt[MEDIA_OPT_URL]->opt_value, frg_unit->opt[MEDIA_OPT_URL]->opt_len);
	}
	/*referer*/
	if(NULL!=frg_unit->opt[MEDIA_OPT_REFERER])
	{
		av_record->referer = (char*)malloc(frg_unit->opt[MEDIA_OPT_REFERER]->opt_len);
		av_record->referer_len = frg_unit->opt[MEDIA_OPT_REFERER]->opt_len;
		memcpy(av_record->referer, frg_unit->opt[MEDIA_OPT_REFERER]->opt_value, frg_unit->opt[MEDIA_OPT_REFERER]->opt_len);
	}
	MESA_lqueue_join_tail(g_frag_run.av_record_lq[frg_unit->thread_seq], &av_record, sizeof(av_record));	
	atomic_inc(&g_frag_stat.sysinfo_stat[AV_RECORD_QUEUE][QUEUE_IN]);
	return 1;
}

void free_av_digest_record(av_digest_record_t* av_digest_record)
{
	if(NULL!=av_digest_record->digest)
	{
		free(av_digest_record->digest);
	}
	free(av_digest_record);
}

int av_digest_record_open_file(int tid)
{
	char			str_time[64];	
	time_t			currTime;
	struct tm*		now;	
	char			filename[MAX_PATH_LEN] = {0};

	time(&currTime);
	now = localtime(&currTime);
	strftime(str_time, sizeof(str_time), "%Y-%m-%d-%H-%M-%S", now); 

	snprintf(filename, sizeof(filename), "%s/av_digest_record_capip%u_tid%d_%s", g_frag_cfg.avrecord_filepath, g_frag_cfg.local_ip_nr, tid, str_time);
	g_frag_run.av_digest_record_curfile[tid] = fopen(filename,"a+");
	if(NULL==g_frag_run.av_digest_record_curfile[tid])
	{
		MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} av_digest_record open filr error: [filename:%s]",	__FILE__,__LINE__, filename);
		return -1;
	}
	return 0;
}

void* av_digest_record_thread(void *param)
{
	av_digest_record_t*	av_digest_record = NULL;
	long				av_digest_record_len = sizeof(av_digest_record_t);		
	int					rec = 0;
	long				tid = (long)param;
	time_t				currTime;
	struct tm*			now;
	char				strTime[32] = {0};
	char 				td[TD_LEN] = {0};
		
	while(1)
	{	
		av_digest_record = NULL;
		rec = MESA_lqueue_get_head(g_frag_run.av_digest_record_lq[tid], &av_digest_record, &av_digest_record_len); 		
		if(0!=rec)
		{
			usleep(10);
			continue;
		}
		atomic_inc(&g_frag_stat.sysinfo_stat[AV_DIGEST_RECORD_QUEUE][QUEUE_OUT]);
		/*first*/
		if(NULL==g_frag_run.av_digest_record_curfile[tid])
		{	
			if(av_digest_record_open_file(tid)<0)
			{	
				free_av_digest_record(av_digest_record);
				continue;
			}
		}		
		/*file full*/	/*maxnum and path are same with avrecord*/
		if(g_frag_run.av_digest_record_curcnt[tid]>=g_frag_cfg.avrecord_maxnum)
		{
			fclose(g_frag_run.av_digest_record_curfile[tid]);
			if(av_digest_record_open_file(tid)<0)
			{				
				free_av_digest_record(av_digest_record);
				continue;
			}
			g_frag_run.av_digest_record_curcnt[tid] = 0;
		}			
		/*av fuzzy  record write to file*/		
		time(&currTime);
		now = localtime(&currTime);
		memset(strTime, 0, sizeof(strTime) );
		strftime(strTime, sizeof(strTime), "%Y-%m-%d %H:%M:%S", now);
		if(g_frag_cfg.avrecord_switch==AV_RECORD_TYPR_TD)
		{		
			/*mid*/  /*media_type*/	/*media_len*/ 
			fprintf(g_frag_run.av_digest_record_curfile[tid],"%s;%lu;0x%02x;%lu", strTime, 
																					 av_digest_record->mid, 
																					 av_digest_record->media_type,
																					 av_digest_record->media_len);
			
			/*td_ori*/			
			fwrite(";", 1, 1, g_frag_run.av_digest_record_curfile[tid]);	
			fwrite(av_digest_record->td_ori, av_digest_record->td_ori_len-1, 1, g_frag_run.av_digest_record_curfile[tid]);		
			
			/*td_0k*/ 
			caculate_md5(av_digest_record->td_ori, av_digest_record->td_ori_len, NULL, 0, td, TD_LEN); 
			fwrite(";", 1, 1, g_frag_run.av_digest_record_curfile[tid]);	
			fwrite(td, strlen(td), 1, g_frag_run.av_digest_record_curfile[tid]);
			
			/*td_data_md5_1k*/	/*td_1k*/ 
			/*td_data_md5_2k*/	/*td_2k*/
			/*td_data_md5_4k*/	/*td_4k*/ 
			/*td_data_md5_8k*/	/*td_8k*/ 
			/*td_data_md5_16k*/ /*td_16k*/ 
			/*td_data_md5_32k*/ /*td_32k*/ 
			/*td_data_md5_64k*/ /*td_64k*/ 
			int i=0;
			for(int j=0;j<=6;j++)
			{
				i=1<<j;
				if(av_digest_record->td_data_len>=i*1024)
				{
					caculate_md5(NULL, 0, av_digest_record->td_data, i*1024, td, TD_LEN); 
					fwrite(";", 1, 1, g_frag_run.av_digest_record_curfile[tid]);	
					fwrite(td, strlen(td), 1, g_frag_run.av_digest_record_curfile[tid]);
					caculate_md5(av_digest_record->td_ori, av_digest_record->td_ori_len, av_digest_record->td_data, i*1024, td, TD_LEN); 
					fwrite(";", 1, 1, g_frag_run.av_digest_record_curfile[tid]);	
					fwrite(td, strlen(td), 1, g_frag_run.av_digest_record_curfile[tid]);	
				}
			}				
		}
		else
		{
			/*mid*/ /*media_type*/	/*media_len*/	/*digist len*/
			fprintf(g_frag_run.av_digest_record_curfile[tid],"%s;%lu;0x%02x;%lu", strTime, 
																					 av_digest_record->mid, 
																					 av_digest_record->media_type,
																					 av_digest_record->media_len);
		}
		
		/*digist*/ /*digist len*/		
		fprintf(g_frag_run.av_digest_record_curfile[tid],";%lu",av_digest_record->digest_len);			
		if(NULL!=av_digest_record->digest)
		{	
			fwrite(";", 1, 1, g_frag_run.av_digest_record_curfile[tid]);		
			fwrite(av_digest_record->digest, av_digest_record->digest_len, 1, g_frag_run.av_digest_record_curfile[tid]);				
		}
		else
		{
			fprintf(g_frag_run.av_digest_record_curfile[tid],";NULL");
		}	
		
		/*url*/
		if(NULL!=av_digest_record->url)
		{	
			fwrite(";", 1, 1, g_frag_run.av_digest_record_curfile[tid]);		
			fwrite(av_digest_record->url, av_digest_record->url_len, 1, g_frag_run.av_digest_record_curfile[tid]);				
		}
		else
		{
			fprintf(g_frag_run.av_digest_record_curfile[tid],";NULL");
		}
		
		fwrite("\n", 1, 1, g_frag_run.av_digest_record_curfile[tid]);		
		fflush(g_frag_run.av_digest_record_curfile[tid]);
		
		g_frag_run.av_digest_record_curcnt[tid]++;		
		atomic_inc(&g_frag_stat.media_stat[LOG_AV_DIGEST_RECORD]);
		free_av_digest_record(av_digest_record);
	}
	return NULL;
}

/*record the last url*/
int av_digest_record(media_t* mdi)
{
	if(!g_frag_cfg.avrecord_switch) return 0;	
	char* digest_buff = NULL;
	unsigned long long digest_len = 0;

	if(g_frag_cfg.avrecord_switch==AV_RECORD_TYPR_TD && !mdi->td_complete) return 0;
	
	av_digest_record_t* av_digest_record = (av_digest_record_t*)calloc(1, sizeof(av_digest_record_t));
	av_digest_record->mid = mdi->mid;
	av_digest_record->media_len = mdi->media_len;
	av_digest_record->media_type = mdi->media_type;

	/*digest*/
	digest_len = SFH_status(mdi->fuzzy, HASH_LENGTH);
	digest_buff = (char*)malloc(sizeof(char)*digest_len);
	av_digest_record->digest_len = SFH_digest(mdi->fuzzy, digest_buff, digest_len);
	av_digest_record->digest = digest_buff;
	/*free by av_fuzzy_record_t*/
	digest_buff = NULL;	

	/*td information*/
	if(g_frag_cfg.avrecord_switch==AV_RECORD_TYPR_TD && mdi->td_complete)
	{
		av_digest_record->td_ori_len = mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_len;
		av_digest_record->td_ori = (char*)calloc(1, av_digest_record->td_ori_len);
		memcpy(av_digest_record->td_ori, mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_len);

		av_digest_record->td_data_len = mdi->td_datalen;
		av_digest_record->td_data = (char*)malloc(mdi->td_datalen);
		memcpy(av_digest_record->td_data, mdi->td_data, mdi->td_datalen);
	}

	/*url*/
	if(NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
	{
		av_digest_record->url = (char*)malloc(mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
		av_digest_record->url_len = mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len;
		memcpy(av_digest_record->url, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
	}

	MESA_lqueue_join_tail(g_frag_run.av_digest_record_lq[mdi->thread_seq], &av_digest_record, sizeof(av_digest_record));	
	atomic_inc(&g_frag_stat.sysinfo_stat[AV_DIGEST_RECORD_QUEUE][QUEUE_IN]);
	
	return 1;
}

