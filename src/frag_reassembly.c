/*
author: 		lishu
start date:		2015-9-23
function:
1. frag_reassembly use for netdisk and HLS/OSMF av.
2. the difference between netdisk and AV
    netdisk maybe boundary when upload , need extract, but AV not proc FRAG_CONTENT
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
#include <sys/time.h>
#include <assert.h>
#include <sys/queue.h>
#include <assert.h>


#include "MESA_handle_logger.h"
#include "MESA_prof_load.h"
#include "MESA_htable.h"
#include "MESA_list_queue.h"
#include "MESA_trace.h"
#include "stream.h"

#include "cJSON.h"

#include "interval_index.h"
#include "stream_fuzzy_hash.h"
#include "soqav_dedup.h"
#include "app_detect.h"
#include "bizman.h"

#include "AV_sendback_in.h"
#include "AV_sendback.h"
#include "common.h"
#include "frag_reassembly_in.h"
#include "frag_reassembly.h"
#include "frag_proc.h"
#include "frag_dedup.h"
#include "frag_redis.h"
#include "frag_av.h"
#include "frag_app.h"
#include "frag_voip.h"
#include "frag_json.h"
#include "frag_dedup.h"
#include "av_record.h"
#include "sifter.h"
#include "service.h"
#include "log.h"
#include "field_stat2.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;

frag_reassembly_t 					frag_rssb;
extern const char* 					hash_eliminate_type[3];

int expire_media_hash_node(void *data, int eliminate_type)
{
	media_t* mdi = (media_t*)data;
	switch(eliminate_type)
	{
		case ELIMINATE_TYPE_NUM:			
			atomic_inc(&g_frag_stat.sysinfo_stat[MEDIA_HASH][HASH_NUM_EXPIRE]);	
			break;
		case ELIMINATE_TYPE_TIME:
			atomic_inc(&g_frag_stat.sysinfo_stat[MEDIA_HASH][HASH_TIME_EXPIRE]);	
			break;
		default:
			break;

	}
	MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} expire_media_hash_node %s: [MID: %llu]", 
							__FILE__,__LINE__, 
							hash_eliminate_type[eliminate_type], 
							mdi->mid);	
	
	return 1;
}

void free_text(text_t** pp, int n_p)
{
    text_t* p = *pp;
    if(NULL!=p)
    {
        for(int i=0;i<n_p;i++)
        {
            if(NULL!=p+i)
            {
                if(NULL!=(p+i)->text)
                {
                    free((p+i)->text);
                    (p+i)->text = NULL;
                }
            }
        }       
        free(p);
        p = NULL;
    }
    *pp = NULL; 
}

void free_opt(opt_in_t** data)
{
	opt_in_t* opt = (opt_in_t*)(*data);
	if(NULL!=opt)
	{
		if(NULL!=opt->opt_value)
		{
			free(opt->opt_value);
		}
		free(opt);
	}
	*data = NULL;	
}

int free_frag_in(void *data, long data_len, void *arg)
{	
	frag_in_t* 	frg_in = (frag_in_t*)data;
	if(NULL!=frg_in)
	{
		if(frg_in->data!=NULL)
		{
			free(frg_in->data);
		}
		free(frg_in);
	}
	return 0;
}

int64_t renew_time_accord_medialen(uint64_t media_len)
{
	int64_t renew_time = g_frag_cfg.renew_time_min + media_len / 1024 / 1024 * g_frag_cfg.renew_time_step; /* prog_len(MB)*step */
	if(renew_time > g_frag_cfg.renew_time_max)
	{
		renew_time = g_frag_cfg.renew_time_max;
	}
	return(renew_time);
}

void renew_media(media_t* mdi)
{
	if(NULL!=mdi)
	{		
		expire_media_write_to_log(mdi, MEDIA_RENEW_EXPIRE, NULL);	
		create_media_write_to_log(mdi, MEDIA_RENEW, NULL);		
		atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_RENEW]);
				
		FLAG_CLEAR(mdi->flag, PROG_SEND_META);				
		mdi->create_time = time(NULL);
		mdi->renew_time	= mdi->create_time + renew_time_accord_medialen(mdi->media_len);
		mdi->pkt_proc = 0;
		mdi->pkt_in = 0;
		mdi->byte_proc = 0;
		mdi->byte_in = 0;		
		for(int i=0;i<KEEP_REOFFSET_MAXNUM;i++)
		{
			mdi->repeat_reoffset[i] = 0;
		}
		IVI_destroy(mdi->ivi, NULL, NULL);			
		mdi->ivi = IVI_create();		
	}	
}

void save_media(media_t* mdi)
{
	int ivi_seg_cnt = IVI_seg_cnt(mdi->save_ivi);
	int tmp_cnt = ivi_seg_cnt;
	IVI_seg_t* ivi_first_seg = IVI_first_seg(mdi->save_ivi);
	IVI_seg_t* ivi_seg = ivi_first_seg;
	IVI_seg_t* next_seg;
	
	FILE*		pFile;
	char		filename[MAX_PATH_LEN] = {0};
	char		data_path[MAX_PATH_LEN] = {0};
	struct tm	now;
	char		day_time[32] = {0};		
	char*		suffix	= NULL;		
	struct timeval 	tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &now);		
	strftime(day_time, sizeof(day_time), "%Y%m%d", &now); 

	if((g_frag_cfg.cpz_type == CPZ_VOIP)&&((mdi->media_type == AUDIO_VIVOX) ||( mdi->media_type==FILE_UNKNOWN)))
	{
		return;
	}
	if(NULL==ivi_seg) return;
	suffix = gen_filesuffix_by_mediatype(suffix, mdi->media_type, mdi->proto);	

	mkdir_r(g_frag_cfg.save_media_path);	
	snprintf(data_path, sizeof(data_path), "%s/%s/",  g_frag_cfg.save_media_path, day_time);
	mkdir_r(data_path);	

	int reoffset = 0;	
	snprintf(filename, sizeof(filename), "%s/%s/%lu_%hu.%s",  g_frag_cfg.save_media_path, day_time, mdi->mid, reoffset, suffix);
	
	pFile = fopen(filename,"r+");	
	if(NULL==pFile)
	{
		pFile = fopen(filename,"w");
		fclose(pFile);
		pFile = fopen(filename,"r+");
	}
	if(NULL!=pFile)
	{
		fwrite(ivi_seg->data, ivi_seg->right - ivi_seg->left+1, 1, pFile);
	}
	
	while( -- tmp_cnt)
	{
		next_seg = IVI_next_seg(ivi_seg);
		if(ivi_seg->right == next_seg->left - 1)
		{
			fwrite(next_seg->data, next_seg->right - next_seg->left+1, 1, pFile);
		}
		else
		{
			fflush(pFile);
			fclose(pFile);
			reoffset ++;
			snprintf(filename, sizeof(filename), "%s/%s/%lu_%hu.%s",  g_frag_cfg.save_media_path, day_time, mdi->mid, reoffset, suffix);
			pFile = fopen(filename,"r+");	
			if(NULL==pFile)
			{
				pFile = fopen(filename,"w");
				fclose(pFile);
				pFile = fopen(filename,"r+");
			}
			if(NULL!=pFile)
			{
				fwrite(next_seg->data, next_seg->right - next_seg->left+1, 1, pFile);
			}
		}
		ivi_seg = next_seg;
	}
	fflush(pFile);
	fclose(pFile);

	//free ivi
	while(ivi_seg_cnt--)
	{
		ivi_first_seg = IVI_first_seg(mdi->save_ivi);
		free(ivi_first_seg->data);
		ivi_first_seg->data = NULL;
		IVI_remove(mdi->save_ivi,ivi_first_seg);
		IVI_seg_free(ivi_first_seg,NULL,NULL);
	}
	IVI_destroy(mdi->save_ivi,NULL,NULL);	
}

void free_media(void* data)
{
	media_t* mdi = (media_t*)data;
	double complte_rate = 0;
	char filename[MAX_PATH_LEN]  = {0};
	
	if(NULL!=mdi)
	{		
		expire_media_write_to_log(mdi, MEDIA_EXPIRE, NULL);			
		atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_EXPIRE]);
		
		if(g_frag_cfg.app_switch)
		{
			if(mdi->app_frg_lq)
			{
				proc_app_data(mdi);
				if(NULL!=mdi->app_frg_lq)
				{
					MESA_lqueue_destroy(mdi->app_frg_lq, NULL, NULL);
					mdi->app_frg_lq = NULL;
				}
			}	
			if(NULL!=mdi->addrlist)
			{
				free(mdi->addrlist);
			}
		}	

		if(g_frag_cfg.save_media)
		{
			save_media(mdi);
		}
		
		/*survey_json_report*/
		if(g_frag_cfg.media_json_switch)
		{	
			media_json_report(mdi, TOPIC_EVENT_EXPIRE);
		}

		/*av_dup report upload*/
		if(g_frag_cfg.av_dedup_switch && MEDIA_SERVICE_TYPE_AV==mdi->media_service_type)			
		{
			media_dedup_report(mdi);
		}			
		if(mdi->media_service_type==MEDIA_SERVICE_TYPE_SIP)
		{
			send_sip_log_when_expire(mdi);			
			free_media_sip(mdi);
		}
		 /*delete monitor file whose size is lower than ...*/
		if(g_frag_cfg.all_hit_monitor_switch==ALL_HIT_MONITOR_FULLFILE && MEDIA_SERVICE_TYPE_AV==mdi->media_service_type && g_frag_cfg.all_hit_monitor_complete_rate>0)
		{
			complte_rate = (double)mdi->byte_proc/(double)mdi->media_len*100;
			if(complte_rate < g_frag_cfg.all_hit_monitor_complete_rate)
			{
				gen_monitor_file_path(mdi->create_time, mdi->mid, (char*)"flv", filename, MAX_PATH_LEN);
				remove(filename);
			}
		}

		if(NULL!=mdi->fuzzy)
		{
			av_digest_record(mdi);
			SFH_release(mdi->fuzzy);
			mdi->fuzzy = NULL;
		}

		for(int i=0;i<MEDIA_OPT_MAXNUN;i++)
		{
			for(int j=0;j<mdi->opt_index;j++)
			{
				if(NULL!=mdi->opt[i][j])
				{
					free_opt(&mdi->opt[i][j]);
				}
			}
		}
		if(NULL!=mdi->td_data)
		{
			free(mdi->td_data);
			mdi->td_data = NULL;
		}		
		if(NULL!=mdi->pid)
		{
			free(mdi->pid);
		}
		if(NULL!=mdi->monitor_path)
		{
			free(mdi->monitor_path);
		}

		/*清除缓存队列*/
		struct queue_item* first_item = TAILQ_FIRST(&mdi->query_wait_lq);		
		frag_in_t* frg = NULL;
		while(first_item != NULL)
		{
			struct queue_item *item = TAILQ_NEXT(first_item, entries);
			TAILQ_REMOVE(&mdi->query_wait_lq, first_item, entries);				
			frg = (frag_in_t*)first_item->node;
			atomic_inc(&g_frag_stat.sysinfo_stat[MULTISRC_QUEUE][QUEUE_OUT]);
			frag_write_to_log(ADD_FRAG_FROM_TAILQ, frg->mid, frg, NULL, 0);
			free_frag_in(frg,0,NULL);
			free(first_item);
			first_item = NULL;
			first_item = item;	
		}
		
		for(int i=0;i<INFO_MEDIA_NUM;i++)
		{	
			if(NULL!=mdi->media_info[i])
			{
				free_text((&mdi->media_info[i]), 1);
			}		
		}		
		IVI_destroy(mdi->ivi, NULL, NULL);	
		free(mdi);
	}	
}

void set_sip_query_task(frag_unit_t* frg_unit, media_t* mdi)
{
	/*voip SIP关联*/
	frag_write_to_log(SEND_VOIP_QUERY_1, frg_unit->pid, frg_unit, NULL, 0); 
	atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_ONCE_QUERY_SEND]);			
	if(sip_index_query(frg_unit, frg_unit->thread_seq))
	{				
		frag_write_to_log(RECV_VOIP_ACK_1, frg_unit->pid, frg_unit, NULL, 0);					
		atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_ONCE_QUERY_RECV]);
	}
	else
	{
		frag_write_to_log(VOIP_QUERY_FAIL_1, frg_unit->pid, frg_unit, NULL, 0); 
	}
	proc_sip_opt(frg_unit, mdi);

	if(mdi->proto==AV_PROTOCOL_SIP)
	{	
		if(mdi->re_offset==0)
		{
			/*设置定时任务:不存在URL的时候，需要进行第二次单向流反查*/
			struct timer_context_t *context = (struct timer_context_t *)calloc(1, sizeof(struct timer_context_t));
			context->mid = mdi->mid;
			MESA_timer_add(g_frag_run.index_query_timer[mdi->thread_seq],
						   time(NULL),
						   g_frag_cfg.index_query_timeout,
						   index_query_timeout,
						   context,
						   index_query_timeout_free,
						   &mdi->index_query_timer_idx);
			g_frag_run.index_query_timer_on[mdi->thread_seq] = 1;
		}			
	}
}

void proc_media_opt(frag_unit_t* frg_unit, media_t* mdi)
{
	/*addr for app*/
	if(g_frag_cfg.app_switch && NULL!=frg_unit->opt[MEDIA_OPT_ADDR])
	{
		mdi->addrlist = merge_addr(mdi->addrlist, &mdi->addrlist_len, frg_unit->opt[MEDIA_OPT_ADDR]->opt_value, frg_unit->opt[MEDIA_OPT_ADDR]->opt_len, frg_unit->thread_seq);
	}	
}
void save_media_opt(frag_unit_t* frg_unit, media_t* mdi)
{
	/*存放每一组选项*/
	for(int i =0;i<MEDIA_OPT_MAXNUN;i++)
	{
		if(mdi->opt_index<OPT_MAXNUN && NULL!=frg_unit->opt[i])
		{
			mdi->opt[i][mdi->opt_index] = frg_unit->opt[i];
			frg_unit->opt[i] = NULL;
		}		
		if(NULL==mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index] && NULL!=mdi->opt[MEDIA_OPT_URL][i])
		{
			mdi->url_opt_index = i;
		}		
	}	
	if(mdi->opt_index<OPT_MAXNUN)
	{
		mdi->opt_index++;
	}

	/*设置二次查询的定时任务*/	
	if(g_frag_cfg.av_dedup_switch && MEDIA_SERVICE_TYPE_AV==mdi->media_service_type)
	{
		if(NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
		{
			/*存在URL进行多源查询*/
			if(!FLAG_TEST(mdi->td_query,TD_QUERY_TYPE_YES))
			{			
				FLAG_SET(mdi->td_query,TD_QUERY_TYPE_MULTISRC);		
			}
		}

		/*存在URL也进行了查询，单纯是为了统计*/
		if(NULL==mdi->index_query_timer_idx)
		{
			/*设置定时任务:不存在URL的时候，需要进行第二次单向流反查*/
			struct timer_context_t *context = (struct timer_context_t *)calloc(1, sizeof(struct timer_context_t));
			context->mid = mdi->mid;
			MESA_timer_add(g_frag_run.index_query_timer[mdi->thread_seq],
						   time(NULL),
						   g_frag_cfg.index_query_timeout,
						   index_query_timeout,
						   context,
						   index_query_timeout_free,
						   &mdi->index_query_timer_idx);
			g_frag_run.index_query_timer_on[mdi->thread_seq] = 1;
		}
	}		
}

/*识别节目的类型 普通音视频 碎片节目 VOIP*/
void set_media_service_type(media_t* mdi)
{
	if(FILE_OSMF==mdi->media_type||FILE_HLS==mdi->media_type)
	{
		mdi->media_service_type = MEDIA_SERVICE_TYPE_FRAG;
		mdi->media_len = 0;
	}	
	else if(FILE_FRAG==mdi->media_type)
	{
		mdi->media_service_type = MEDIA_SERVICE_TYPE_FRAG;
	}
	else if(AV_PROTOCOL_SIP==mdi->proto)
	{			
		mdi->media_service_type = MEDIA_SERVICE_TYPE_SIP;
	}	
}

/*proc_unit : frag_unit*/
long media_create_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{	
	media_t* 			mdi = (media_t*)data;
	frag_unit_t*		frg_unit = (frag_unit_t*)user_arg;
	int 				in_set = 0;  // record cap_ip/PID , update, keep the newest capIP
	char				pbuf[32] = {0};			
	int 				buf_len = 32;
	int					media_create_flag = 0;
		
	if(NULL==mdi)
	{		
		mdi = (media_t*)calloc(1,sizeof(media_t));
		if(0>(MESA_htable_add(g_frag_run.media_hash,key, size,(const void*)mdi)))
		{
			MESA_handle_runtime_log(g_frag_run.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									"{%s:%d} create_media_cb MESA_htable_add error: [mid: %llu]", 
									__FILE__,__LINE__, *(uint64_t*)key);
			free_media(mdi);
			return -1;
		}
		media_create_flag = 1;
		
		/*set mediainfo*/
		mdi->mid = frg_unit->mid;		
		mdi->mid_after_multisrc = mdi->mid;						
		mdi->media_len = frg_unit->media_len;
		/*media type*/
		mdi->media_type = frg_unit->media_type; 			
		mdi->proto = frg_unit->proto;	
		mdi->hit_service = frg_unit->hitservice;
		mdi->data_flag = frg_unit->data_flag;
		mdi->meta_flag = frg_unit->flag;
		mdi->thread_seq = frg_unit->thread_seq;

		/*确定节目是否是碎片或者VOIP*/
		set_media_service_type(mdi);

		/*处理选项*/
		proc_media_opt(frg_unit, mdi);		

		/*设置sip查询任务*/		
		if(mdi->media_service_type==MEDIA_SERVICE_TYPE_SIP)
		{
			set_sip_query_task(frg_unit, mdi);
		}

		/*特殊媒体类型检查*/
		if(0!=mdi->media_type)
		{
			set_special_media_service(mdi->media_type, mdi);
		}
		
		/*app*/
		if(g_frag_cfg.app_switch)
		{
			mdi->app_frg_lq =  MESA_lqueue_create(1, 0);
		}
		
		mdi->create_time = time(NULL);
		mdi->renew_time	= mdi->create_time + renew_time_accord_medialen(mdi->media_len);	
		
		for(int i=0;i<KEEP_REOFFSET_MAXNUM;i++)
		{
			mdi->repeat_reoffset[i] = -1;
		}	
		/*ivi*/
		mdi->ivi = IVI_create();	
		mdi->save_ivi = IVI_create();

		/*多源查询缓存队列*/
		TAILQ_INIT(&mdi->query_wait_lq);
		
		/*fuzzy*/
		if(g_frag_cfg.fuzzy_digest_switch)
		{
			mdi->fuzzy = SFH_instance(mdi->media_len);	
		}			
		create_media_write_to_log(mdi, MEDIA_NEW, NULL);
		atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_CREATE]);	
		media_stat(mdi, frg_unit);				
	}
	else
	{
		/*常规音视频节目一定时间或者命中语种监测，renew节目。*/
		if(!g_frag_cfg.app_switch && frg_unit->ab_offset==0 
			&& ((MEDIA_SERVICE_TYPE_AV==mdi->media_service_type && mdi->renew_time < time(NULL))||mdi->hit_service==SERVICE_AUDIO_LANG))
		{						
			renew_media(mdi);			
			media_create_flag = 1;
		}		
	}
	save_media_opt(frg_unit,mdi);
	/*av_dedup*/
	if(g_frag_cfg.av_dedup_switch 
		&& mdi->td_complete==0 
		&& g_frag_cfg.td_data_maxsize==0 
		&& MEDIA_SERVICE_TYPE_AV==mdi->media_service_type 
		&& NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
	{		
		generate_td_meta(mdi);	
		FLAG_SET(mdi->td_query,TD_QUERY_TYPE_DEDUP);
		caculate_md5(mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_len, NULL, 0, mdi->td, TD_LEN);
		mdi->td_complete = 1;
	}
	
	/*碎片去重*/
	if(-1==media_frag_removal(mdi, frg_unit))
	{
		return -1;
	}
	frag_write_to_log(SET_OFFSET, *(uint64_t*)key, frg_unit, NULL, 0); 

	/*PID存储*/
	mdi->frag_unit_cnt++;	
	if(NULL==mdi->pid)
	{
		mdi->pid = (uint64_t*)calloc(1, PID_MAXNUM*sizeof(uint64_t));
	}
	for(int i=0; i<PID_MAXNUM; i++)
	{
		if(frg_unit->pid== mdi->pid[i])
		{
			in_set = 1;
			break;
		}
	}
	if(!in_set)
	{			
		mdi->pid[mdi->pid_idx_last] = frg_unit->pid;				
		mdi->pid_idx_last = (mdi->pid_idx_last+1)%PID_MAXNUM;
	}

	/*CAP_IP 回传数据的前端机器IP*/
	if(frg_unit->qd_info_from_cpz_idx_last==0)
	{
		if(save_qd_info(mdi->qd_info, QD_MAXNUM, &mdi->qdinfo_idx_last,frg_unit->capIP, frg_unit->mid))
		{
			inet_ntop(AF_INET, &frg_unit->capIP, pbuf, buf_len);
			create_media_write_to_log(mdi, MEDIA_FROM_CAPIP, pbuf); 
		}
	}
	/*多源条件下，其他粗瓶装的前端信息，用于回建议*/
	for(int i=0;i<frg_unit->qd_info_from_cpz_idx_last;i++)
	{
		inet_ntop(AF_INET, &frg_unit->capIP, pbuf, buf_len);
		create_media_write_to_log(mdi, MEDIA_FROM_CPZIP, pbuf); 
		save_qd_info(mdi->qd_info_from_cpz, QD_MAXNUM, &mdi->qdinfo_from_cpz_idx_last, frg_unit->qd_info_from_cpz[i].cap_ip, frg_unit->qd_info_from_cpz[i].mid);
	}	
	
	/*monitor because of 音频语种全节目监测*/
	/*有音频语种监测派生出来，粗拼装根据上述日志进行的音频原始文件记录*/
	audio_lang_monitor_service(frg_unit->src_ip, mdi);
	/*send 监测建议*/
#if K_PROJECT
	if(frg_unit->hitservice >= 0x180 && frg_unit->hitservice < 0x200)
	{
		mdi->hit_service = frg_unit->hitservice;
		config_monitor_service(frg_unit->src_ip, mdi);			
	}
#else
	if(frg_unit->hitservice > 0x80)
	{
		mdi->hit_service = frg_unit->hitservice;
		config_monitor_service(frg_unit->src_ip, mdi);			
	}
#endif

	/*数据全监测，但是不向前端发建议*/
	if(g_frag_cfg.all_hit_monitor_switch==ALL_HIT_MONITOR_FULLFILE && MEDIA_SERVICE_TYPE_AV==mdi->media_service_type)
	{
		FLAG_SET(mdi->flag, PROG_FLAG_DUMP);
	}
	
	/*media_create json*/
	if(g_frag_cfg.media_json_switch && media_create_flag)
	{	
		/*survey_json_report*/
		media_json_report(mdi, TOPIC_EVENT_CREATE);
	}
	return 0;
}

int media_create(frag_unit_t* frg_unit)
{
	long  			rec_cb = 0;

	/*检查定时器*/	
	if(g_frag_cfg.av_dedup_switch && g_frag_run.multisrc_timer_on[frg_unit->thread_seq] && g_frag_run.multisrc_timer[frg_unit->thread_seq])
	{
		MESA_timer_check(g_frag_run.multisrc_timer[frg_unit->thread_seq], time(NULL), g_frag_cfg.multisrc_timer_cb_maxtime);
	}
	/*检查定时器*/	
	if(g_frag_run.index_query_timer_on[frg_unit->thread_seq] && g_frag_run.index_query_timer[frg_unit->thread_seq])
	{
		MESA_timer_check(g_frag_run.index_query_timer[frg_unit->thread_seq], time(NULL), g_frag_cfg.index_query_timer_cb_maxtime);
	}
	
	if(0!=frg_unit->mid)
	{		
		MESA_htable_search_cb(g_frag_run.media_hash,(const uint8_t *)&frg_unit->mid,sizeof(frg_unit->mid),
							  media_create_cb,(void*)frg_unit,&rec_cb);
		/*av record*/
		av_record(frg_unit);		
	}
	return (int)rec_cb;
}

void free_frag_unit(void* data)
{
	frag_unit_t* 	frg_unit = (frag_unit_t*)data; 
	if(NULL!=frg_unit)
	{		
		atomic_inc(&frag_rssb.stat_info[RSSB_FREE_FRAG_UNIT]);
		switch(frg_unit->media_type)
		{
			case FILE_HLS:
			case FILE_OSMF: 			
			case FILE_FRAG: 				
			case FILE_MAYBE_FRAG: 				
				frag_redis_index_twice(frg_unit);
				break;

			default:
				break;
		}
		
		if(NULL!=frg_unit->text)
		{
			free_text(&frg_unit->text, frg_unit->text_num);
		}
		if(NULL!=frg_unit->frg_info)
		{			
			for(int i=0;i<FRAG_UNIT_INFO_NUM;i++)
			{
				free_text(&frg_unit->frg_info[i], 1);
			}
			frg_unit->frg_info = NULL;
		}
		for(int i=0;i<MEDIA_OPT_MAXNUN;i++)
		{
			if(NULL!=frg_unit->opt[i])
			{
				free_opt(&frg_unit->opt[i]);
			}
		}		
		
		if(frg_unit->proto==AV_PROTOCOL_SIP)
		{
			free_frag_unit_sip_opt(frg_unit);	
		}	
		free(frg_unit);
		frg_unit = NULL;
	}	
}

void init_frag_unit(frag_unit_t* frg_unit, uchar protocol)
{
	frg_unit->frg_info = (text_t**)calloc(1, FRAG_UNIT_INFO_NUM*sizeof(text_t*));	
}

int add_media_info(msg_metainfo_t* minfo, char* opt, uint32_t src_ip, int thread_seq)
{
	long  				rec_cb = 0;
	rssb_media_info_t 	media_info;
	frag_unit_t* 		frg_unit = NULL;

	media_info.pid = *(uint64_t*)minfo->prog_id;	
	media_info.media_type = minfo->media_type;
	media_info.media_len = minfo->prog_len;
	media_info.protocol = minfo->protocol;
	media_info.hitservice = minfo->hitservice;
	media_info.data_flag = minfo->data_flag;	
	media_info.flag = minfo->flag;		
	media_info.opt_num = minfo->opt_num;
	media_info.opt = opt;	
	media_info.cap_IP = minfo->cap_IP;
	media_info.src_ip = src_ip;	
	media_info.thread_seq = thread_seq;
	
	/*音视频碎片化*/
	if(is_frag(minfo->media_type))
	{
		if(minfo->media_type==FILE_MAYBE_FRAG)
		{
			return 0;
		}
		MESA_htable_search_cb(frag_rssb.converge_hash, (const uint8_t *)&media_info.pid, sizeof(media_info.pid),
							  converge_mediainfo_search_cb, (void*)&media_info, &rec_cb);		
	}
	/*VOIP协议*/
	else if(minfo->protocol==AV_PROTOCOL_SIP)
	{
		if(!(g_frag_cfg.voip_filter_switch && minfo->media_type==VOIP_UNKNOWN_MEDIA_TYPE))
		{
			frg_unit = (frag_unit_t*)calloc(1,sizeof(frag_unit_t));
			set_sip_frag_unit(&media_info, frg_unit);			
			frg_unit->mid = frg_unit->pid;
			media_create(frg_unit);
			free_frag_unit(frg_unit);
			frg_unit = NULL;
		}
	}
	/*多线程下载*/
	else
	{
	 	frg_unit = (frag_unit_t*)calloc(1,sizeof(frag_unit_t));
		set_frag_unit(&media_info, frg_unit);
		/*前端邮件没有addr，降载会段错误*/
		if(g_frag_cfg.av_dedup_switch && frg_unit->opt[MEDIA_OPT_ADDR]==NULL)
		{		
			free_frag_unit(frg_unit);
			return 0;
		}
		/*单向流多源查询*/	
		if(g_frag_cfg.av_dedup_switch && NULL==frg_unit->opt[MEDIA_OPT_URL] && NULL!=frg_unit->opt[MEDIA_OPT_SINGLE_KEY])
		{
			frag_write_to_log(SEND_AV_QUERY_1, frg_unit->pid, frg_unit, NULL, 0);		
			atomic_inc(&frag_rssb.stat_info[RSSB_AV_ONCE_QUERY_SEND]); 	
			if(av_query(frg_unit))
			{
				frag_write_to_log(RECV_AV_ACK_1, frg_unit->pid, frg_unit, NULL, 0);				
				atomic_inc(&frag_rssb.stat_info[RSSB_AV_ONCE_QUERY_RECV]);
			}
			else
			{
				frag_write_to_log(AV_QUERY_FAIL_1, frg_unit->pid, frg_unit, NULL, 0); 
			}					
		}
		frg_unit->mid = frg_unit->pid;
		media_create(frg_unit);
		free_frag_unit(frg_unit);
		frg_unit = NULL;
	}		
	return rec_cb;
}

void trace_store_file(uint64_t mid, uint8_t media_type, uint8_t proto, uint16_t reoffset, uint64_t aboffset, char* data, uint32_t data_len)
{
	FILE*		pFile;
	char		filename[MAX_PATH_LEN] = {0};
	char		data_path[MAX_PATH_LEN] = {0};
	struct tm	now;
	char		day_time[32] = {0};		
	char*		suffix	= NULL;		
	struct timeval 	tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &now);		
	strftime(day_time, sizeof(day_time), "%Y%m%d", &now); 

	if((g_frag_cfg.cpz_type == CPZ_VOIP)&&(media_type == AUDIO_VIVOX))
	{
		return;
	}
	
	/*根据media_type命名文件后缀*/	
	suffix = gen_filesuffix_by_mediatype(suffix, media_type, proto);	

	mkdir_r(g_frag_cfg.store_filepath);	
	snprintf(data_path, sizeof(data_path), "%s/%s/",  g_frag_cfg.store_filepath, day_time);
	mkdir_r(data_path);	
	snprintf(filename, sizeof(filename), "%s/%s/%lu_%hu.%s",  g_frag_cfg.store_filepath, day_time, mid, reoffset, suffix);
	pFile = fopen(filename,"r+");	
	if(NULL==pFile)
	{
		pFile = fopen(filename,"w");
		fclose(pFile);
		pFile = fopen(filename,"r+");
	}	
	if(NULL!=pFile)
	{		
		fseek(pFile,aboffset,SEEK_SET);
		fwrite(data, data_len, 1, pFile);
		fflush(pFile);
		fclose(pFile);	
	}
}

void tag_frag_in_media(media_t* mdi, frag_in_t* frg)
{
	mdi->pkt_proc++;
	mdi->byte_proc += frg->datalen;	
	mdi->lastpkt_time = time(NULL);	

	frg->new_mid = mdi->mid_after_multisrc;	
	//frg->create_time = mdi->create_time;	
	//frg->media_type = mdi->media_type;
	//frg->proto = mdi->proto;
	
	if(NULL!=mdi->fuzzy)
	{
		mdi->fuzzy_acc_len += SFH_feed(mdi->fuzzy, frg->data, frg->datalen, frg->offset_in);
	}
	if(g_frag_cfg.av_dedup_switch && !FLAG_TEST(mdi->td_query,TD_QUERY_TYPE_YES) && MEDIA_SERVICE_TYPE_AV==mdi->media_service_type)
	{
		if(set_td_data(mdi, frg))
		{
			FLAG_SET(mdi->td_query,TD_QUERY_TYPE_DEDUP);
		}
	}
	/*在第一个数据包之前需要发送节目元信息*/
	if(!FLAG_TEST(mdi->flag, PROG_SEND_META))
	{
		FLAG_SET(frg->frag_flag, FRAG_FLAG_SEND_META);
		FLAG_SET(mdi->flag, PROG_SEND_META);
	}
	/*数据是否需要发送给福系统*/
	if(FLAG_TEST(mdi->flag, PROG_FLAG_EXCP))
	{
		FLAG_SET(frg->frag_flag, FRAG_FLAG_WINS);
	}	
	/*数据是否多源，发送给其他粗瓶装*/
	if(FLAG_TEST(mdi->td_query, TD_QUERY_RES_MULTISRC))
	{
		FLAG_SET(frg->frag_flag, FRAG_FLAG_MULTISRC);
		frg->multisrc_bizmanip = mdi->multisrc_bizmanip;
		/*多源的mid已经被修改*/		
		frg->new_mid = mdi->mid_after_multisrc;
	}
	if(g_frag_cfg.app_switch)
	{
		MESA_lqueue_join_tail(mdi->app_frg_lq, &frg, sizeof(frg));					
		atomic_inc(&frag_rssb.sysinfo_stat[RSSB_WAIT_QUEUE][QUEUE_IN]);					
		frag_write_to_log(ADD_FRAG_TO_APP_LQ, frg->mid, frg, NULL, 0);				
	}
	/*trace debug*/
	if(g_frag_cfg.store_filepath_switch)
	{
		trace_store_file(frg->mid, mdi->media_type, mdi->proto, frg->seq, frg->offset, frg->data, frg->datalen);
	}	
}

void copy_frg(frag_in_t* src, frag_in_t* dst)
{
	memcpy(src, dst, sizeof(frag_in_t));
	src->data = (char*)malloc(dst->datalen);
	memcpy(src->data, dst->data, dst->datalen);
}

int frag_add_tailq(media_t* mdi, frag_in_t* frg, int thread_seq)
{
	frag_in_t*	frg_new = (frag_in_t*)malloc(sizeof(frag_in_t));
	struct queue_item* item = NULL;

	copy_frg(frg_new, frg);	
	item = (struct queue_item*)malloc(sizeof(struct queue_item));
	item->node = (void*)frg_new;		
	item->thread_seq = thread_seq;
	TAILQ_INSERT_TAIL(&mdi->query_wait_lq, item, entries);		
	atomic_inc(&g_frag_stat.sysinfo_stat[MULTISRC_QUEUE][QUEUE_IN]);
	frag_write_to_log(ADD_FRAG_TO_TAILQ, frg_new->mid, frg_new, NULL, 0);	
	return 0;
}

/*降载多源等待队列,数据同时往后回传，所以不用free*/
int frag_add_query_wait_lq(media_t* mdi, frag_ivi_info_t* frag_ivi_info, uint32_t frag_stat)
{	
	frag_in_t*	frg = frag_ivi_info->frg;
	
	if(1==frag_stat)
	{	
		frag_add_tailq(mdi, frg, frag_ivi_info->thread_seq);		
	}
	else if(frag_stat==2)
	{
		frag_in_t* frg_new = NULL;
		for(int i=0;i<frag_ivi_info->frg_array_cnt;i++)
		{
			frg_new = frag_ivi_info->frg_array[i];				
			frag_add_tailq(mdi, frg_new, frag_ivi_info->thread_seq);		
		}
	}		
	return 0;
}

long media_preproc_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{	
	int 				frag_stat = 0;
	media_t* 			mdi = (media_t*)data;
	frag_ivi_info_t* 	frag_ivi_info = (frag_ivi_info_t*)user_arg;
	frag_unit_t*		frg_unit = frag_ivi_info->frg_unit;
	frag_in_t*			frg = frag_ivi_info->frg;	
			
	if(NULL==mdi)
	{		
		return -1;			
	}	

	/*节目收到的数据长度统计*/
	mdi->pkt_in++;	
	mdi->byte_in += frg->datalen;	

	/*命中监测配置，在数据去重之前写文件*/
	if(g_frag_cfg.monitor_file_switch && FLAG_TEST(mdi->flag, PROG_FLAG_DUMP))
	{				
		monitor_service_dump_file(frg, mdi);
	}

	/*如果节目重复，不需要处理*/	
	if(FLAG_TEST(mdi->td_query, TD_QUERY_RES_DEDUP) && !g_frag_cfg.dedup_invalid)
	{	
		frag_ivi_info->td_query = mdi->td_query;	
		return 1;
	}

	/*offset==0, log*/
	if(0==frg->offset && MEDIA_SERVICE_TYPE_AV==mdi->media_service_type)
	{		
		update_special_media_service(mdi, frg);
		/*
		if(!FLAG_TEST(mdi->flag, PROG_OFFSET_ZERO))
		{
			atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_OFFSET_ZERO]);
			FLAG_SET(mdi->flag, PROG_OFFSET_ZERO);
		}			
		create_media_write_to_log(mdi, MEDIA_OFFSET_ZERO, NULL); 	
		*/
	}	
	/*record maxoffset*/
	//mdi->maxoffset = MAX(frg->offset, mdi->maxoffset);
	
	/*IVI 去重*/
	if(NULL!=frg)
	{
		frag_stat = media_removal(mdi, frg_unit, frg, frag_ivi_info);		
		/*keep frag*/
		if(frag_stat==1)
		{	
			tag_frag_in_media(mdi, frg);
			media_byte_stat(mdi, frg_unit, frg);	
		}
		/*new frag*/
		else if(frag_stat==2)
		{
			/*every frag set info same with frag*/
			for(int i=0;i<frag_ivi_info->frg_array_cnt;i++)
			{
				frag_ivi_info->frg_array[i]->mid = frg->mid;
				frag_ivi_info->frg_array[i]->pid = frg->pid;				
				frag_ivi_info->frg_array[i]->offset_in = frag_ivi_info->frg_array[i]->offset;	
				frag_ivi_info->frg_array[i]->seq = frg->seq;	
				frag_ivi_info->frg_array[i]->thread_seq = frg->thread_seq;	
				tag_frag_in_media(mdi, frag_ivi_info->frg_array[i]);		
				media_byte_stat(mdi, frg_unit, frag_ivi_info->frg_array[i]);	
			}				
		}
		else
		{
			return 0;
		}		
	}

	 /*offset==0, log*/
        if(0==frg->offset)
        {    
                if(!FLAG_TEST(mdi->flag, PROG_OFFSET_ZERO))
                {    
                        atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_OFFSET_ZERO]);
                        FLAG_SET(mdi->flag, PROG_OFFSET_ZERO);
                }    
                create_media_write_to_log(mdi, MEDIA_OFFSET_ZERO, NULL);
        }    
        /*record maxoffset*/
        mdi->maxoffset = MAX(frg->offset, mdi->maxoffset);

	/*调试功能:存储有效文件dmj*/
	if(g_frag_cfg.save_media)
	{
		if(g_frag_cfg.cpz_type == CPZ_VOIP)
		{
			if(frg->datalen > sizeof(voip_header_t))
			{
				uint32_t datalen = frg->datalen - sizeof(voip_header_t);
				char* data = (char*)calloc(1,datalen);
				memcpy(data,frg->data + sizeof(voip_header_t),datalen);
				IVI_seg_t* a_ivi_seg = IVI_seg_malloc(frg->offset, frg->offset + datalen - 1, data);
				IVI_insert(mdi->save_ivi,a_ivi_seg);
			}
		}
		else
		{
			char* data = (char*)calloc(1,frg->datalen);
			memcpy(data,frg->data,frg->datalen);
			IVI_seg_t* a_ivi_seg = IVI_seg_malloc(frg->offset, frg->offset + frg->datalen-1, data);
			IVI_insert(mdi->save_ivi,a_ivi_seg);			
		}
	}

	/*节目降载多源查询之前的数据准备*/
	if(g_frag_cfg.av_dedup_switch && MEDIA_SERVICE_TYPE_AV==mdi->media_service_type)
	{
		proc_media_multisrc(mdi, 0);
		if(!FLAG_TEST(mdi->td_query, TD_QUERY_ACK_MULTISRC))
		{
			/*因为多源需要数据缓存，同时继续往后端发送*/
			frag_add_query_wait_lq(mdi, frag_ivi_info, frag_stat); 
		}
		frag_ivi_info->td_query = mdi->td_query;
	}		
	return (long)frag_stat;
}

int media_preproc(frag_ivi_info_t* frag_ivi_info)
{
	long  			rec_cb = -1;
	frag_in_t*		frg = frag_ivi_info->frg;

	/*调试，使得检查定时器超时淘汰*/
	/*
	if(g_frag_cfg.av_dedup_switch && g_frag_run.multisrc_timer[frag_ivi_info->thread_seq])
	{
		MESA_timer_check(g_frag_run.multisrc_timer[frag_ivi_info->thread_seq], time(NULL), g_frag_cfg.multisrc_timer_cb_maxtime);
	}
	*/
	
	if(0!=frg->mid)
	{		
		MESA_htable_search_cb(g_frag_run.media_hash,(const uint8_t *)&frg->mid,sizeof(frg->mid),
							  media_preproc_cb,(void*)frag_ivi_info,&rec_cb);				
	}
	return (int)rec_cb;
}

/*非阻塞队列，尝试多次发送*/
 void frag_try_add_wait_lq(uint8_t td_query, frag_in_t* frg, int thread_seq)
 {
 	int 		lq_rec = 0;
	//int 		try_num = 0;

	atomic_inc(&g_frag_stat.stat_info[TO_SEND][TOTAL_PKTS]);
	atomic_add(&g_frag_stat.stat_info[TO_SEND][TOTAL_BYTES], frg->datalen); 			
	if(FLAG_TEST(td_query, TD_QUERY_RES_DEDUP))
	{	
		atomic_inc(&g_frag_stat.stat_info[DEDUP_DROP][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[DEDUP_DROP][TOTAL_BYTES], frg->datalen); 
	}
	if(!FLAG_TEST(td_query, TD_QUERY_RES_DEDUP) || g_frag_cfg.dedup_invalid)
	{	
		lq_rec = MESA_lqueue_join_tail(frag_rssb.wait_lq[thread_seq], &frg, sizeof(frg));
		if(lq_rec==MESA_QUEUE_RET_OK)
		{
			frag_write_to_log(ADD_FRAG_TO_WAIT_LQ, frg->mid, frg, NULL, 0);
			atomic_inc(&frag_rssb.sysinfo_stat[RSSB_WAIT_QUEUE][QUEUE_IN]); 
		}				
		else
		{
			atomic_inc(&g_frag_stat.stat_info[WAIT_QUEUE_FULL_DROP][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.stat_info[WAIT_QUEUE_FULL_DROP][TOTAL_BYTES], frg->datalen);						
			free_frag_in(frg,0,NULL);
		}
	}
	else
	{
		free_frag_in(frg,0,NULL);
	}					
 }

/*发送队列*/
int frag_add_wait_lq(frag_ivi_info_t* frag_ivi_info, uint32_t frag_stat, int thread_seq)
{	
	frag_in_t*	frg = frag_ivi_info->frg;

	/*rec=1: IVI 去重只有一个frag   rec=2:IVI去重只有两个frag*/
	if(1==frag_stat)
	{				
		frag_try_add_wait_lq(frag_ivi_info->td_query, frg, thread_seq); 
	}
	else if(frag_stat==2)
	{
		frag_in_t* frg_new = NULL;
		for(int i=0;i<frag_ivi_info->frg_array_cnt;i++)
		{			
			frg_new = frag_ivi_info->frg_array[i];
			frag_try_add_wait_lq(frag_ivi_info->td_query, frg_new, thread_seq); 		
			frag_ivi_info->frg_array[i] = NULL;
		}
		free_frag_in(frg,0,NULL);
	}
	else
	{
		free_frag_in(frg,0,NULL);
	}
	return 0;
}

/*返回值是-1，*/
int frag_service(frag_ivi_info_t* frag_ivi_info, uint32_t src_ip, int thread_seq)
{
	int 		frag_stat = 0;	
	
	frag_ivi_info->thread_seq = thread_seq;
	frag_ivi_info->mid = frag_ivi_info->frg->mid;	
	
	/*碎片化场景，重复数据不再处理*/
	if(frag_ivi_info->frg_unit!=NULL && frag_ivi_info->frg_unit->repeat_not_proc) 
	{
		atomic_inc(&g_frag_stat.stat_info[FRAG_DEDUP][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[FRAG_DEDUP][TOTAL_BYTES],frag_ivi_info->frg->datalen);
		return -1;
	}

	/*create media, because no cnvg and no index*/
	frag_stat = media_preproc(frag_ivi_info);
	if(-1==frag_stat) 
	{
		/*没有找到节目，可能是没有元信息*/
		frag_write_to_log(MEDIA_NO_META, frag_ivi_info->frg->pid, NULL, NULL, 0);		
		atomic_inc(&g_frag_stat.stat_info[MEDIA_NOMETA][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[MEDIA_NOMETA][TOTAL_BYTES],frag_ivi_info->frg->datalen);
		return -1;
	}		
	/*碎片化处理流程:入队列，发送*/
	if(!g_frag_cfg.app_switch)
	{
		frag_add_wait_lq(frag_ivi_info, frag_stat, thread_seq);
	}	
	return 0;
}

long converge_data_search_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{	
	frag_ivi_info_t* 	frag_ivi_info = (frag_ivi_info_t*)user_arg;	
	frag_unit_t* 		frg_unit =  (frag_unit_t*)data;	
	frag_in_t*			frg = frag_ivi_info->frg;	

	if(NULL==frg_unit)
	{		
		return -1;		
	}	

	frag_ivi_info->frg_unit = frg_unit;	
	
	/*碎片化场景，处理过的不再处理，在这里丢弃，减少查节目的次数*/
	if(frg_unit->repeat_not_proc) 
	{
		atomic_inc(&g_frag_stat.stat_info[FRAG_DEDUP][TOTAL_PKTS]);
		atomic_add(&g_frag_stat.stat_info[FRAG_DEDUP][TOTAL_BYTES],frg->datalen);	
		free_frag_in(frg,0,NULL);
		return 0;
	}	
	
	/*write log*/		
	frag_write_to_log(ADD_FRAG, frg->pid, frg, NULL, 0);	

	if(frg_unit->frag_state==STAT_OK)
	{
		frg->mid = frg_unit->mid;
		return 1;
	}

	if(frg_unit->frag_state==STAT_CNVG_QUERY)
	{
		/*join frag to cnvg_lq*/
		MESA_lqueue_join_tail(frg_unit->frg_cnvg_lq, &frg, sizeof(frg));				
		/*write log*/			
		frag_write_to_log(ADD_FRAG_TO_CNVG_LQ, frg->mid, frg, NULL, 0);
		atomic_inc(&frag_rssb.sysinfo_stat[RSSB_CNVG_QUEUE][QUEUE_IN]);
	}
	else if(frg_unit->frag_state==STAT_INDEX_QUERY)
	{
		MESA_lqueue_join_tail(frg_unit->frg_index_lq, &frg, sizeof(frg));				
		frag_write_to_log(ADD_FRAG_TO_INDEX_LQ, frg->mid, frg, NULL, 0);
		atomic_inc(&frag_rssb.sysinfo_stat[RSSB_INDEX_QUEUE][QUEUE_IN]);
	}
	return 0;	
}

int add_frag(uint64_t pid, uint64_t offset, char* data, uint32_t datalen, uint8_t protocol, uint32_t src_ip, int thread_seq)
{
	frag_ivi_info_t* 	frag_ivi_info = (frag_ivi_info_t*)calloc(1, sizeof(frag_ivi_info_t));	
	frag_in_t* 			frg = NULL;
	long				rec = 0;	

	frg = (frag_in_t*)calloc(1, sizeof(frag_in_t)); 
	frg->mid = pid;	
	frg->pid = pid;	
	frg->offset = offset;
	frg->datalen = datalen;
	frg->data = (char*)malloc(frg->datalen);
	memcpy(frg->data, data, datalen);
	frg->thread_seq = thread_seq;
	frg->src_ip = src_ip;
	
	/*发送响应包*/
	send_ack_to_qd(frg, src_ip, thread_seq);
	
	frag_ivi_info->frg = frg;	
	MESA_htable_search_cb(frag_rssb.converge_hash, (const uint8_t *)&pid, sizeof(pid),
						  converge_data_search_cb, (void*)frag_ivi_info, &rec); 
	/*碎片关联结束，准备发送*/
	if(1==rec)
	{
		rec=frag_service(frag_ivi_info, src_ip, thread_seq);
		if(-1==rec)
		{			
			free_frag_in(frg,0,NULL);
		}
	}
	/*传统节目*/
	else if(-1==rec)
	{
		rec = frag_service(frag_ivi_info, src_ip, thread_seq);		
		/*没有元信息,释放*/
		if(-1==rec)
		{			
			free_frag_in(frg,0,NULL);
		}
	}		
	free(frag_ivi_info);
	return rec;	
}

/*return 
* -1:节目不存在
* 0:不发送元信息
* 1:发送元信息
*/
long get_media(void *data, const uint8_t *key, uint size, void *user_arg)
{
	media_t* 		mdi = (media_t*)data;
	media_info_t* 	media_info = (media_info_t*)user_arg;
	int				opt_num = 0;
	if(NULL!=mdi)
	{		
		media_info->td_query = mdi->td_query;
		media_info->mdi_flag = mdi->flag;		
		/*media_len*//*mid,protocol,mediatype,file_name*/
		media_info->prog_len = mdi->media_len;
		media_info->mid = mdi->mid_after_multisrc;
		media_info->protocol = mdi->proto;
		media_info->data_flag = mdi->data_flag;
		media_info->flag = mdi->meta_flag;
		/*多源在其他汇聚粗拼装上不再扩散监测标识*/
		if(FLAG_TEST(mdi->td_query, TD_QUERY_RES_MULTISRC))
		{
			media_info->hitservice = 0;	
		}
		else
		{
			media_info->hitservice = mdi->hit_service;	
		}
		/*便于统计for data*/	
		/*除了HLS OSMF，其他碎片协议media_type设置为传统节目的mediatype*/
		if(mdi->media_type==FILE_FRAG)
		{
			media_info->media_type = FILE_AV;			
			media_info->prog_len = 0;
		}
		else
		{
			media_info->media_type = mdi->media_type;	
		}
		if(g_frag_cfg.modify_capIP_switch)
		{
			media_info->cap_IP = g_frag_cfg.local_ip_nr;
		}
		else
		{
			media_info->cap_IP = mdi->qd_info[0].cap_ip;
		}		
		
		/*多源，发给其他粗拼接装，需要添加选项*/
		if(FLAG_TEST(mdi->td_query, TD_QUERY_RES_MULTISRC))
		{
			/*发给粗瓶装，选项*/			
			if(NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
			{
				media_info->opt_unit = (struct opt_unit_t*)calloc(2+mdi->qdinfo_idx_last, sizeof(struct opt_unit_t));
				/*META_OPT_LAYER_ADDR*/
				media_info->opt_unit[0].opt_len = sizeof(uint32_t)+sizeof(uint8_t)+mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_len;
				media_info->opt_unit[0].opt_type = META_OPT_LAYER_ADDR;
				media_info->opt_unit[0].opt_value = (char*)calloc(1, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_len);
				memcpy(media_info->opt_unit[0].opt_value, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_len);
				opt_num++;	
				
				/*META_OPT_LAYER_URL*/			
				media_info->opt_unit[1].opt_len = sizeof(uint32_t)+sizeof(uint8_t)+mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len;
				media_info->opt_unit[1].opt_type = META_OPT_LAYER_URL;
				media_info->opt_unit[1].opt_value = (char*)calloc(1, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
				memcpy(media_info->opt_unit[1].opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
				opt_num++;	
			}
			else
			{
				media_info->opt_unit = (struct opt_unit_t*)calloc(1+mdi->qdinfo_idx_last, sizeof(struct opt_unit_t));				
				/*META_OPT_LAYER_ADDR*/
				media_info->opt_unit[0].opt_len = sizeof(uint32_t)+sizeof(uint8_t)+mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_len;
				media_info->opt_unit[0].opt_type = META_OPT_LAYER_ADDR;
				media_info->opt_unit[0].opt_value = (char*)calloc(1, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_len);
				memcpy(media_info->opt_unit[0].opt_value, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_len);
				opt_num++;
			}
			
			/*META_OPT_MID_BEFORE_MULTISRC*/
			for(int i=0;i<mdi->qdinfo_idx_last;i++)
			{
				media_info->opt_unit[2+i].opt_len = sizeof(uint32_t)+sizeof(uint8_t)+sizeof(uint64_t)+sizeof(uint32_t);
				media_info->opt_unit[2+i].opt_type = META_OPT_INFO_BEFORE_MULTISRC;
				media_info->opt_unit[2+i].opt_value = (char*)calloc(1, sizeof(uint64_t)+sizeof(uint32_t));
				memcpy(media_info->opt_unit[2+i].opt_value, &mdi->qd_info[i].mid, sizeof(uint64_t));				
				memcpy(media_info->opt_unit[2+i].opt_value+sizeof(uint64_t), &mdi->qd_info[i].cap_ip, sizeof(uint32_t));
				opt_num++;	
			}			
		}
		else
		{
			/*SIP 选项*/
			if(mdi->proto==AV_PROTOCOL_SIP && mdi->sip_rate_info!=NULL)
			{
				media_info->opt_unit = (struct opt_unit_t*)calloc(1, sizeof(struct opt_unit_t));
				media_info->opt_unit->opt_len = sizeof(uint32_t)+sizeof(uint8_t)+mdi->sip_rate_info->opt_len;
				media_info->opt_unit->opt_type = META_OPT_SIP_SEND_RATE_INFO;
				media_info->opt_unit->opt_value = (char*)calloc(1, mdi->sip_rate_info->opt_len);
				memcpy(media_info->opt_unit->opt_value, mdi->sip_rate_info->opt_value, mdi->sip_rate_info->opt_len);
				opt_num++;
			}		
#if K_PROJECT
#else
			if(mdi->proto!=AV_PROTOCOL_SIP)
			{				
				/*发给内容分析程序，选项*/
				media_info->opt_unit = (struct opt_unit_t*)calloc(1, sizeof(struct opt_unit_t));
				media_info->opt_unit->opt_len = sizeof(uint32_t)+sizeof(uint8_t)+sizeof(char*);
				media_info->opt_unit->opt_type = OPT_SOURCE_IP;
				media_info->opt_unit->opt_value = (char*)calloc(1, sizeof(char*));
				*(unsigned int*)(media_info->opt_unit->opt_value) = g_frag_cfg.local_ip_nr;	
				opt_num++;	
			}
#endif
		}	
		media_info->opt_num = opt_num;
		return 1;		
	}	
	return 0;
}

void frag_reassembly_stats_init(const char* logdir, const char* filename)
{	
	char 	log[MAX_PATH_LEN]={0};
	int 	value = 0;		
	char	conf_buf[MAX_PATH_LEN]={0};		
	
	memset(conf_buf, 0, sizeof(conf_buf));
	frag_rssb.stat_handle = FS_create_handle();	
	MESA_load_profile_string_def(filename, "LOG", "StatFile", conf_buf, sizeof(conf_buf),"./log/frag_rssb/frag_reassembly_stat.log");	
	snprintf(log,sizeof(log),"%s/%s", logdir,conf_buf);
	FS_set_para(frag_rssb.stat_handle, OUTPUT_DEVICE, log, strlen(log)+1);
	value = 1;//flush by date
	FS_set_para(frag_rssb.stat_handle, FLUSH_BY_DATE, &value, sizeof(value));	
	value = 2;//append
	FS_set_para(frag_rssb.stat_handle, PRINT_MODE, &value, sizeof(value));	
	MESA_load_profile_short_def(filename, "LOG", "StatInterval", (short*)&frag_rssb.stat_interval,0); 
	FS_set_para(frag_rssb.stat_handle, STAT_CYCLE, &frag_rssb.stat_interval, sizeof(frag_rssb.stat_interval));	
	value = (frag_rssb.stat_interval!=0) ? 1 : 0;
	FS_set_para(frag_rssb.stat_handle, PRINT_TRIGGER, &value, sizeof(value));
	value = 0;
	FS_set_para(frag_rssb.stat_handle, CREATE_THREAD, &value, sizeof(value));	
	FS_set_para(frag_rssb.stat_handle, APP_NAME, g_frag_stat.fs_app, strlen(g_frag_stat.fs_app)+1);		
	if(g_frag_stat.fs_remote_switch)
	{
		FS_set_para(frag_rssb.stat_handle, STATS_SERVER_IP, g_frag_stat.fs_ip, strlen(g_frag_stat.fs_ip)+1);	
		FS_set_para(frag_rssb.stat_handle, STATS_SERVER_PORT, &g_frag_stat.fs_port, sizeof(g_frag_stat.fs_port));
	}
	
	memset(conf_buf, 0, sizeof(conf_buf));	
	frag_rssb.sysinfo_handle = FS_create_handle();	
	MESA_load_profile_string_def(filename, "LOG", "SysinfoFile", conf_buf, sizeof(conf_buf),"./log/frag_rssb/frag_reassembly_sysinfo.log");	
	snprintf(log,sizeof(log),"%s/%s", logdir,conf_buf);
	FS_set_para(frag_rssb.sysinfo_handle, OUTPUT_DEVICE, log, strlen(log)+1);
	value = 1;//flush by date
	FS_set_para(frag_rssb.sysinfo_handle, FLUSH_BY_DATE, &value, sizeof(value));	
	value = 2;//append
	FS_set_para(frag_rssb.sysinfo_handle, PRINT_MODE, &value, sizeof(value));	
	MESA_load_profile_short_def(filename, "LOG", "SysinfoInterval", (short*)&frag_rssb.sysinfo_interval,0); 
	FS_set_para(frag_rssb.sysinfo_handle, STAT_CYCLE, &frag_rssb.sysinfo_interval, sizeof(frag_rssb.sysinfo_interval));	
	value = (frag_rssb.sysinfo_interval!=0) ? 1 : 0;
	FS_set_para(frag_rssb.sysinfo_handle, PRINT_TRIGGER, &value, sizeof(value));
	value = 0;
	FS_set_para(frag_rssb.sysinfo_handle, CREATE_THREAD, &value, sizeof(value));

}

int read_frag_reassembly_conf(int thread_num, const char* logdir, const char* filename, int* maat_stat_swicth, int* maat_perf_swicth)
{
	int log_level = 0;	
	char log_name[MAX_PATH_LEN]={0};
	char log[MAX_PATH_LEN]={0};

	/*frag reassembly log*/
	MESA_load_profile_short_def(filename, "LOG", "FragReassemblyLogLevel", (short*)&frag_rssb.logger_level,10);
	MESA_load_profile_string_def(filename, "LOG", "FragReassemblyLogName", log_name , sizeof(log_name),"./log/frag_rssb/frag_reassembly.log");
	snprintf(log,sizeof(log),"%s/%s", logdir,log_name);
	frag_rssb.logger = MESA_create_runtime_log_handle(log,frag_rssb.logger_level);
	if(NULL==frag_rssb.logger)
	{				
		printf("read_frag_reassembly_conf: get FragReassemblyLog logger error.\n");
		return -1;
	}

	/*maat stat log*/
	MESA_load_profile_int_def(filename, "LOG", "MaatStatSwitch", maat_stat_swicth, 0);
	MESA_load_profile_int_def(filename, "LOG", "MaatPerfSwitch", maat_perf_swicth, 0);

	/*media log*/
	memset(log, 0, sizeof(log));
	memset(log_name, 0, sizeof(log_name));
	MESA_load_profile_short_def(filename, "LOG", "MediaLogLevel", (short*)&log_level,30);
	MESA_load_profile_string_def(filename, "LOG", "MediaLogName", log_name , sizeof(log_name),"./log/frag_rssb/frag_reassembly_media.log");
	snprintf(log,sizeof(log),"%s/%s", logdir,log_name);
	frag_rssb.media_logger = MESA_create_runtime_log_handle(log,log_level);
	if(NULL==frag_rssb.media_logger)
	{			
		printf("read_frag_reassembly_conf: get MediaLog media_logger error.\n");
		return -1;
	}	

	/*sysinfo log*/
	frag_reassembly_stats_init(logdir, filename);	

	/*redis cluster init*/
	int timeout = 0;	
	MESA_load_profile_int_def(filename, "NETWORK", "RedisClusterSwitch", &frag_rssb.redis_cluster_switch, 1);
	MESA_load_profile_int_def(filename, "NETWORK", "RedisTimeout", &timeout, 5);	

	MESA_load_profile_string_nodef(filename, "NETWORK", "RedisBrokers", frag_rssb.redis_addr, sizeof(frag_rssb.redis_addr));		
	//MESA_load_profile_string_nodef(filename, "NETWORK", "RedisNet", frag_rssb.redis_cluster_net, sizeof(frag_rssb.redis_cluster_net));		
	//MESA_load_profile_int_def(filename, "NETWORK", "RedisUniqIPFlag", &frag_rssb.redis_cluster_netflag, 512);

	MESA_load_profile_string_nodef(filename, "NETWORK", "RedisIP", frag_rssb.redis_ip, sizeof(frag_rssb.redis_ip));		
	MESA_load_profile_int_def(filename, "NETWORK", "RedisPort", &frag_rssb.redis_port, 6379);
	frag_rssb.redis_tv.tv_sec = timeout;	
	frag_rssb.redis_tv.tv_usec = 0;	
	
	/*wait queue num*/	
	MESA_load_profile_uint_def(filename, "SYSTEM", "WaitQueueNum", &frag_rssb.wait_lq_num, 5000000);
	
	/*init hash size*/	
	MESA_load_profile_uint_def(filename, "SYSTEM", "ConvergeHashThreadSafe", &frag_rssb.cnvg_hash_thread_safe, 64);
	MESA_load_profile_uint_def(filename, "SYSTEM", "ConvergeHashSize", &frag_rssb.cnvg_hash_max_elem_num, 1024*1024);
   	MESA_load_profile_uint_def(filename, "SYSTEM", "ConvergeHashElemNum", &frag_rssb.cnvg_hash_size, 1024*1024*16);
	MESA_load_profile_uint_def(filename, "SYSTEM", "ConvergeHashExpireTime", &frag_rssb.cnvg_hash_expire_time, 120);
		
	return 0;
}

void frag_reassembly_release()
{
	int i=0;
	if(NULL!=frag_rssb.sifter)
	{
		destroy_sifter(frag_rssb.sifter);	
	}
		
	if(NULL!=frag_rssb.converge_hash)
	{
		MESA_htable_destroy(frag_rssb.converge_hash, free_frag_unit);
	}		
	
	for(i=0;i<frag_rssb.lq_num;i++)
	{			
		if(NULL!=frag_rssb.wait_lq[i])
		{
			MESA_lqueue_destroy(frag_rssb.wait_lq[i], free_frag_in, NULL);
		}
	}
}

int frag_reassembly_init(const char* frag_rssb_cfg_dir, const char* frag_rssb_log_dir, int thread_num)
{
	memset(&frag_rssb, 0, sizeof(frag_reassembly_t));
	char config_buff[MAX_PATH_LEN] = {0};

	/*frag_reassembly conf*/
	int maat_stat_swicth = 0;
	int maat_perf_swicth = 0;
	snprintf(config_buff, sizeof(config_buff), "%s/%s", frag_rssb_cfg_dir, "frag_reassembly.conf");	
	if(-1==read_frag_reassembly_conf(thread_num, frag_rssb_log_dir, config_buff, &maat_stat_swicth, &maat_perf_swicth))
	{	
		printf("read_frag_reassembly_conf error.\n");
		frag_reassembly_release();
		return -1;
	}	

	/*tpl conf*/
	snprintf(config_buff, sizeof(config_buff), "%s/%s", frag_rssb_cfg_dir, "sifter/");
	frag_rssb.sifter = create_sifter(config_buff, frag_rssb_log_dir, maat_stat_swicth, maat_perf_swicth, thread_num, frag_rssb.logger);
	if(NULL==frag_rssb.sifter)
	{	
		printf("create_sifter error.\n");
		MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} create_sifter error", __FILE__,__LINE__);
		frag_reassembly_release();		
		return -1;
	}
	
	/*converge_hash*/
	MESA_htable_create_args_t hash_args;
	memset(&hash_args,0,sizeof(MESA_htable_create_args_t));
	hash_args.thread_safe = frag_rssb.cnvg_hash_thread_safe;  //group lock
	hash_args.recursive = 1;
	hash_args.hash_slot_size = frag_rssb.cnvg_hash_size;
	hash_args.max_elem_num = frag_rssb.cnvg_hash_max_elem_num;
	hash_args.eliminate_type = HASH_ELIMINATE_ALGO_LRU;
	hash_args.expire_time = frag_rssb.cnvg_hash_expire_time;
	hash_args.key_comp = NULL;
	hash_args.key2index = NULL;
	hash_args.data_free = free_frag_unit; 
	hash_args.data_expire_with_condition = expire_cnvg_hash_node;   
	frag_rssb.converge_hash = MESA_htable_create(&hash_args, sizeof(hash_args));	
	if(NULL==frag_rssb.converge_hash)
	{		
		frag_reassembly_release();
		printf("create converge_hash error.\n");
		MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} create converge_hash error", __FILE__,__LINE__);
		return -1;
	}	

	/*wait_lq for get_frag*/
	frag_rssb.lq_num = thread_num;
	frag_rssb.wait_lq = (MESA_lqueue_head*)calloc(1, frag_rssb.lq_num*sizeof(MESA_lqueue_head));
    for(int i=0;i<frag_rssb.lq_num;i++)
    {		
		frag_rssb.wait_lq[i] =  MESA_lqueue_create(1, frag_rssb.wait_lq_num);
    }		
	
	/*stat log*/
	if(0<frag_rssb.stat_interval)
	{
		if(-1 == create_pthread(thread_frag_rssb_stat_output,NULL,frag_rssb.logger))
    	{
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									(char*)"[%s:%d] thread thread_frag_rssb_stat_output create failed" ,__FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
			return -1;
    	}
	}

	/*sysinfo log*/
	if(0<frag_rssb.sysinfo_interval)
	{
		if(-1 == create_pthread(thread_frag_rssb_sysinfo_output,NULL,frag_rssb.logger))
    	{
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
									(char*)"[%s:%d] thread thread_frag_rssb_sysinfo_output create failed" ,__FILE__,__LINE__,FRAG_REASSEMBLY_MODULE_NAME);
			return -1;
    	}
	}
	return 0;
}

