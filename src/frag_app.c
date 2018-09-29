/*
* APP
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

#include "MESA_handle_logger.h"

#include "app_detect.h"

#include "AV_interface.h"

#include "frag_app.h"
#include "frag_proc.h"
#include "log.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;
extern frag_reassembly_t 			frag_rssb;

void app_change_pid(frag_unit_t* frg_unit)
{	
	char				pid_buf[1024] = {0};	
	char 				serverIP[64] = {0};
	uint64_t			pid_before = frg_unit->pid;
	/*APP的PID修改为serverIP+len, 不是tuple7*/
	get_serverIP(frg_unit->opt[MEDIA_OPT_ADDR]->opt_value, frg_unit->opt[MEDIA_OPT_ADDR]->opt_len, serverIP);
	snprintf(pid_buf, sizeof(pid_buf), "APP_%s->%" PRIu64 "",  serverIP, frg_unit->content_length);
	frg_unit->pid = make_mid(pid_buf, strlen(pid_buf), PID_TYPE_IPLEN);	
	frg_unit->mid = frg_unit->pid;
	frag_write_to_log(APP_CHANGE_PID, pid_before, frg_unit, NULL, 0);			
}

static int url_indicated_app(const char *url)
{
	const char *pfilename = NULL, *p1 = NULL, *p2 = NULL, *p3 = NULL, *p = NULL;
	unsigned int len=0;

	if(url == NULL)
		return 0;

	pfilename = strrchr(url, '/');
	if(pfilename == NULL || *(pfilename + 1) == '\0')
		return 0;

	pfilename = pfilename + 1;
	len = strlen(pfilename);

	p1 = strchr(pfilename, ';');
	p2 = strchr(pfilename, '?');
	p3 = strchr(pfilename, '#');

	if(p1>p2)
	{
		p = p1; p1 = p2; p2 = p;
	}
	if(p1>p3)
	{
		p = p3; p1 = p3; p3 = p;
	}
	if(p2>p3)
	{
		p = p2; p2 = p3; p3 = p;
	}
	if(p1 != NULL)
		len = p1 - pfilename;
	else if(p2 != NULL)
		len = p2 - pfilename;
	else if(p3 != NULL)
		len = p3 - pfilename;
	
	if(!strncasecmp(pfilename + len - 4, ".apk", 4))
		return FILE_ANDRIOD;
	if(!strncasecmp(pfilename + len - 4, ".ipa", 4))
		return FILE_IOS;
	return 0;
}

void free_appdtc_detail(appdtc_detail_in_t*	 detail)
{
	if(NULL!=detail)
	{
		if(NULL!=detail->app_data)
		{
			free(detail->app_data);
		}
		if(NULL!=detail->tuple4)
		{
			free(detail->tuple4);
		}
		if(NULL!=detail->url)
		{
			free(detail->url);
		}
		if(NULL!=detail->user_agent)
		{
			free(detail->user_agent);
		}
		if(NULL!=detail->app_frg_lq)
		{
			MESA_lqueue_destroy(detail->app_frg_lq, NULL, NULL);
			detail->app_frg_lq = NULL;
		}
		free(detail);
	}
}

void proc_app_data(media_t* mdi)
{	
	appdtc_detail_in_t*	detail = (appdtc_detail_in_t*)calloc(1, sizeof(appdtc_detail_in_t));
	char*				p_url = NULL;
	
	/*appid*/
	detail->appid = mdi->mid;

	/*app_type*/
	if(mdi->media_type==FILE_APP)
	{
		if(NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
		{
			p_url = (char*)malloc(mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len+1);
			memset(p_url, 0 , mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len+1);
			memcpy(p_url, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
			mdi->media_type = url_indicated_app(p_url);
			if(NULL!=p_url)
			{
				free(p_url);
			}
		}
	}

	switch(mdi->media_type)
	{	
		case FILE_IOS:			
			detail->app_type = APP_TYPE_IOS;
			break;
		case FILE_ANDRIOD:			
			detail->app_type = APP_TYPE_ANDROID;
			break;
		default:			
			detail->app_type = APP_TYPE_UNKNOWN;
			break;
	}
	
	/*data*/
	detail->app_frg_lq = mdi->app_frg_lq;
	mdi->app_frg_lq = NULL;	

	/*tuple4*/
	if(NULL!=mdi->addrlist)
	{
		detail->tuple4 = (char*)malloc(mdi->addrlist_len);
		memcpy(detail->tuple4, mdi->addrlist, mdi->addrlist_len);
		detail->tuple4_len = mdi->addrlist_len;
	}

	/*url : app_http_session urls are same, so save one*/
	if(NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
	{
		detail->url = (char*)malloc(mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
		memcpy(detail->url, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
		detail->url_len = mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len;		
	}	
	
	/*ua*/
	if(NULL!=mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index])
	{
		detail->user_agent = (char*)malloc(mdi->opt[MEDIA_OPT_UA][mdi->url_opt_index]->opt_len);
		memcpy(detail->user_agent, mdi->opt[MEDIA_OPT_UA][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_UA][mdi->url_opt_index]->opt_len);
		detail->ua_len = mdi->opt[MEDIA_OPT_UA][mdi->url_opt_index]->opt_len;	
	}	
	atomic_inc(&g_frag_stat.sysinfo_stat[APP_QUEUE][QUEUE_IN]);
	MESA_lqueue_join_tail(g_frag_run.app_lq, &detail, sizeof(detail));		
}

void* thread_send_app_data(void *param)
{
	appdtc_detail_in_t*	detail = NULL;
	long 				detail_len = sizeof(detail);	
	frag_in_t*			frg = NULL;
	long				frglen = sizeof(frg);
	int 				rec_detail = 0;
	int 				rec_frg = 0;
	
	while(1)
	{
		rec_detail = MESA_lqueue_try_get_head(g_frag_run.app_lq, &detail, &detail_len);
		if (rec_detail<0)
		{		
			usleep(10);
		}	
		else
		{		
			rec_frg = MESA_lqueue_try_get_head(detail->app_frg_lq, &frg, &frglen);
			while(0==rec_frg)
			{	
				atomic_inc(&frag_rssb.sysinfo_stat[RSSB_WAIT_QUEUE][QUEUE_OUT]); 
				MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
										"{%s:%d} get app_frg_lq: [PID: %llu, offset: %llu, datalen:%u]", 
										__FILE__,__LINE__, frg->pid, frg->offset, frg->datalen);
				if(detail->data_len > frg->offset+frg->datalen)
				{
					memcpy((char*)detail->app_data+frg->offset, (void*)frg->data, frg->datalen);
				}
				else
				{
					detail->app_data = (char*)realloc(detail->app_data, frg->offset+frg->datalen);			
					memcpy((char*)detail->app_data+frg->offset, (void*)frg->data, frg->datalen);
					detail->data_len = frg->offset+frg->datalen;			
				}						
				free_frag_in(frg,0,NULL);
				rec_frg = MESA_lqueue_try_get_head(detail->app_frg_lq, &frg, &frglen);
			}			
			atomic_inc(&g_frag_stat.sysinfo_stat[APP_QUEUE][QUEUE_OUT]);
			atomic_inc(&g_frag_stat.stat_info[TO_SEND][TOTAL_PKTS]);
			atomic_add(&g_frag_stat.stat_info[TO_SEND][TOTAL_BYTES], detail->data_len);
#if APP_FUNC
			APPDTC_PLUG_ENTRY(g_frag_run.appdtc_handle, (appdtc_detail_t*)detail, 0);	
#endif
			free_appdtc_detail(detail);
			detail = NULL;			
		}
	}	
	return NULL;
}

