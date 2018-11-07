/*
* 降载与多源查询业务
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
#include "MESA_htable.h"
#include "MESA_list_queue.h"
#include "stream_fuzzy_hash.h"
#include "soqav_dedup.h"

#include "common.h"
#include "frag_reassembly_in.h"
#include "frag_proc.h"
#include "frag_dedup.h"
#include "frag_av.h"
#include "log.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;

uchar addrlist_to_streamtuple4(const char *addr, struct stream_tuple4_v4 *tuple4_v4, struct stream_tuple4_v6 *tuple4_v6)
{	
	uchar addrtype = 0;
	
	touple4_type_t touple4;	
	
	if(sscanf(addr, "%[0-9.],%hu>%[0-9.],%hu", touple4.sip, &touple4.sport, touple4.dip, &touple4.dport) == 4)
	{
		addrtype = ADDR_TYPE_IPV4; 		
		inet_pton(AF_INET, touple4.sip, &tuple4_v4->saddr); 
		inet_pton(AF_INET, touple4.dip, &tuple4_v4->daddr); 		
		tuple4_v4->source = htons(touple4.sport); 
		tuple4_v4->dest = htons(touple4.dport); 
	}	
	else if(sscanf(addr, "%[0-9A-Fa-f:.],%hu>%[0-9A-Fa-f:.],%hu",  touple4.sip, &touple4.sport, touple4.dip, &touple4.dport) == 4)
	{
		addrtype = ADDR_TYPE_IPV6; 
		inet_pton(AF_INET6, touple4.sip, tuple4_v6->saddr); 
		inet_pton(AF_INET6, touple4.dip, tuple4_v6->daddr);		
		tuple4_v6->source = htons(touple4.sport); 
		tuple4_v6->dest = htons(touple4.dport); 
	}	
	return addrtype;
}

void media_dedup_report(media_t* mdi)
{
	char* digest_buff = NULL;
	unsigned long long digest_len = 0;
	report_detail_t detail;
	struct stream_tuple4_v4 tuple4_v4;
	struct stream_tuple4_v6 tuple4_v6;
	
	memset(&detail, 0, sizeof(report_detail_t));

	if(mdi->configID)
	{
		detail.scan_detail = (scan_detail_t*)malloc(sizeof(scan_detail_t));
		detail.scan_detail->config_id = mdi->configID;
	}
	
	detail.mid = mdi->mid;
	detail.current_len = mdi->byte_proc;
	detail.total_len = mdi->media_len;	
	detail.ivi_handle = mdi->ivi;

	/*digest*/	
	if(NULL!=mdi->fuzzy)
	{
		digest_len = SFH_status(mdi->fuzzy, HASH_LENGTH);
		digest_buff = (char*)malloc(sizeof(char)*digest_len);
		detail.sfh_len = SFH_digest(mdi->fuzzy, digest_buff, digest_len);	
		detail.file_sfh = digest_buff;
	}

	/*url*/	
	if(NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
	{
		detail.urllen = mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len;
		detail.url = mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value;
	}

	/*addr*/	
	if(NULL!=mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index])
	{
		detail.addrtype = addrlist_to_streamtuple4(mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, &tuple4_v4, &tuple4_v6);		
		switch(detail.addrtype)
		{
			case ADDR_TYPE_IPV4:
				detail.tuple4_v4 = &tuple4_v4;
				break;
			case ADDR_TYPE_IPV6:
				detail.tuple4_v6 = &tuple4_v6;
				break;
		}
	}
	
	if(FLAG_TEST(mdi->td_query, TD_QUERY_TYPE_DEDUP) && !FLAG_TEST(mdi->td_query, TD_QUERY_RES_NOREPORT))
	{
		soqav_dedup_report(g_frag_run.dedup_hd, mdi->td, &detail);	
	}
	else
	{
		/*重复节目不需要上报，TD置为NULL*/
		soqav_dedup_report(g_frag_run.dedup_hd, NULL, &detail);	
	}
	frag_write_to_log(AV_DEDUP_REPORT, mdi->mid, (void*)mdi->td, (void*)&mdi->byte_proc, mdi->configID);	
	atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_DEDUP_REPORT]);
	if(mdi->td_complete)
	{
		atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_DEDUP_REPORT_COMPLETE]);
	}	

	if(NULL!=digest_buff)
	{
		free(digest_buff);
	}
	if(NULL!=detail.scan_detail)
	{
		free(detail.scan_detail);
	}
}

long media_query_ack(void *data, const uint8_t *key, uint size, void *user_arg)
{	
	media_t*	mdi = (media_t*)data;
	char 		ip_buf[64] = {0};
	query_result_t* ack_rsl = (query_result_t*)user_arg;	
	int			result = ack_rsl->status;		
	uint32_t	multisrc_bizmanip = ack_rsl->cpz_payload_ip;

	/*调试代码*/
	/*
	result = BIT_TD_MULTI_SOURCE;
	multisrc_bizmanip = 16777343;	
	ack_rsl->mid = 1111283823269670000;
	*/	
	
	if(NULL!=mdi)
	{	
		/*如果之前收到了应答，不用再处理*/
		if(FLAG_TEST(mdi->td_query, TD_QUERY_ACK_DEDUP) || FLAG_TEST(mdi->td_query, TD_QUERY_ACK_MULTISRC))
		{			
			return 0;
		}
		FLAG_SET(mdi->td_query, TD_QUERY_ACK_DEDUP);
		FLAG_SET(mdi->td_query, TD_QUERY_ACK_MULTISRC);

		/*debug 降载不上线的情况下，设置debug模式，降载失效*/
		/*多源有数据缓存*/
		if(FLAG_TEST(mdi->td_query, TD_QUERY_TYPE_MULTISRC))
		{
			if(result & BIT_TD_KNOWN)
			{					
				frag_write_to_log(RECV_AV_DEDUP_ACK_KNOWN, mdi->mid, (void*)mdi->td, &result, 0);					
				atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_DEDUP]);
				FLAG_SET(mdi->td_query, TD_QUERY_RES_DEDUP);				
				FLAG_SET(mdi->td_query, TD_QUERY_RES_NOREPORT);				
			}
			/*汇聚IP是本机16777343(127.0.0.1)，且节目ID一样，不需要聚合*/
			else if(result & BIT_TD_MULTI_SOURCE && (multisrc_bizmanip!=16777343 ||(multisrc_bizmanip==16777343 && mdi->mid != (uint64_t)ack_rsl->mid)))
			{	
				inet_ntop(AF_INET, &multisrc_bizmanip, ip_buf, 64);
				frag_write_to_log(RECV_AV_DEDUP_ACK_MULTI, mdi->mid, mdi, ip_buf, 0);	
				atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_MULTI]);					
				FLAG_SET(mdi->td_query, TD_QUERY_RES_MULTISRC); 				
				mdi->mid_after_multisrc = ack_rsl->mid;						
				mdi->multisrc_bizmanip = multisrc_bizmanip;					
				create_media_write_to_log(mdi, MEDIA_MID_CHANGE, &ack_rsl->mid);		
			}
			
			struct queue_item* first_item = TAILQ_FIRST(&mdi->query_wait_lq);			
			frag_ivi_info_t frag_ivi_info;
			while(first_item != NULL)
			{			
				memset(&frag_ivi_info, 0, sizeof(frag_ivi_info));
				struct queue_item *item = TAILQ_NEXT(first_item, entries);
				TAILQ_REMOVE(&mdi->query_wait_lq, first_item, entries);
				frag_ivi_info.frg = (frag_in_t*)first_item->node;
				frag_ivi_info.thread_seq = first_item->thread_seq;					
				atomic_inc(&g_frag_stat.sysinfo_stat[MULTISRC_QUEUE][QUEUE_OUT]);
				frag_write_to_log(ADD_FRAG_FROM_TAILQ, frag_ivi_info.frg->mid, frag_ivi_info.frg, NULL, 0);
				/*节目重复，直接丢弃*/
				if(result & BIT_TD_KNOWN && !g_frag_cfg.dedup_invalid)
				{											
					/*不应该计入去重统计，因为这部分数据其实已经发送给了分析程序*/
					free_frag_in(frag_ivi_info.frg,0,NULL);
				}
				else
				{					
					if(result & BIT_TD_MULTI_SOURCE && (multisrc_bizmanip!=16777343 ||(multisrc_bizmanip==16777343 && mdi->mid != (uint64_t)ack_rsl->mid)))
					{							
						frag_ivi_info.frg->new_mid = mdi->mid_after_multisrc;
						frag_ivi_info.frg->multisrc_bizmanip = mdi->multisrc_bizmanip;							
						FLAG_SET(frag_ivi_info.frg->frag_flag, FRAG_FLAG_MULTISRC);						
						frag_add_wait_lq(&frag_ivi_info, 1, frag_ivi_info.thread_seq); 
					}
					/*不聚合的多源，数据丢弃，因为已经发送过了*/
					else
					{
						free_frag_in(frag_ivi_info.frg,0,NULL);
					}
				}				
				free(first_item);
				first_item = NULL;
				first_item = item;					
			}
		}
		else if(result & BIT_TD_KNOWN)
		{	
			atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_DEDUP]);							
			FLAG_SET(mdi->td_query, TD_QUERY_RES_DEDUP);						
			FLAG_SET(mdi->td_query, TD_QUERY_RES_NOREPORT);	
		}
	}
	else
	{
		atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_DEDUP_TIMEOUT]);
	}
	return 0;
}

long soqav_query_callback(const char *td, const query_result_t *result, void *user_arg)
{	
	long		rec_cb = 0;
	uint64_t 	media_mid = (uint64_t)user_arg;	
	
	frag_write_to_log(RECV_AV_DEDUP_ACK, media_mid, (void*)td, (void*)&result->status, 0); 			
	MESA_htable_search_cb(g_frag_run.media_hash, (const uint8_t *)&media_mid, sizeof(media_mid),
						  media_query_ack, (void*)result, &rec_cb);			
	
	atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_DEDUP_ACK]);		
	return rec_cb;
}

void generate_td_meta(media_t* mdi)
{
	char*		td_buf = NULL;
	int 		td_buflen = 0;
	char		media_len[64] = {0};
	char		media_type[16] = {0};
	char        serverIP[64] = {0};
	
	snprintf(media_type, sizeof(media_type), "%hhu", mdi->media_type);
	if(is_frag(mdi->media_type))
	{
		snprintf(media_len, sizeof(media_len), "%lu", mdi->media_len);
	}
	else
	{
		snprintf(media_len, sizeof(media_len), "%d", 0);
	}
	
	if(mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index] != NULL)
	{
		get_serverIP(mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_len, serverIP);
	}
	
	td_buflen = strlen("URL:")
		+strlen("ServerIP:")+strlen(serverIP)
		+strlen("MediaType:")+strlen(media_type)
		+strlen("MediaLen:")+strlen(media_len)
		+strlen("Etag:")
		+strlen("LastModify:")
		+1;
	if(mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index] != NULL)
	{
		td_buflen+=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len;
	}
	if(mdi->opt[MEDIA_OPT_ETAG][mdi->url_opt_index] != NULL)
	{
		td_buflen+=mdi->opt[MEDIA_OPT_ETAG][mdi->url_opt_index]->opt_len;
	}
	if(mdi->opt[MEDIA_OPT_LAST_MODIFY][mdi->url_opt_index] !=NULL)
	{
		td_buflen+=mdi->opt[MEDIA_OPT_LAST_MODIFY][mdi->url_opt_index]->opt_len;
	}
	td_buf = (char*)calloc(1, td_buflen);
	char* p_td_buf = td_buf;

	memcpy(p_td_buf, "URL:" ,strlen("URL:"));
	p_td_buf += strlen("URL:");
	if(mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index] != NULL)
	{
		memcpy(p_td_buf,mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value,mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
		p_td_buf += mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len;
	}
	memcpy(p_td_buf, "ServerIP:" ,strlen("ServerIP:"));
	p_td_buf += strlen("ServerIP:");	
	if(strlen(serverIP) >0)
	{
		memcpy(p_td_buf, serverIP,strlen(serverIP));
		p_td_buf += strlen(serverIP);
	}
	memcpy(p_td_buf, "MediaType:" ,strlen("MediaType:"));
	p_td_buf += strlen("MediaType:");	
	memcpy(p_td_buf, media_type, strlen(media_type));	
	p_td_buf += strlen(media_type);
	memcpy(p_td_buf, "MediaLen:" ,strlen("MediaLen:"));
	p_td_buf += strlen("MediaLen:");	
	memcpy(p_td_buf, media_len, strlen(media_len));	
	p_td_buf += strlen(media_len);	
	
	memcpy(p_td_buf, "Etag:" ,strlen("Etag:"));
	p_td_buf += strlen("Etag:");
	if(mdi->opt[MEDIA_OPT_ETAG][mdi->url_opt_index] != NULL)
	{
		memcpy(p_td_buf, mdi->opt[MEDIA_OPT_ETAG][mdi->url_opt_index]->opt_value,mdi->opt[MEDIA_OPT_ETAG][mdi->url_opt_index]->opt_len);
		p_td_buf += mdi->opt[MEDIA_OPT_ETAG][mdi->url_opt_index]->opt_len;
	}
	memcpy(p_td_buf, "LastModify:" ,strlen("LastModify:"));
	p_td_buf += strlen("LastModify:");
	if(mdi->opt[MEDIA_OPT_LAST_MODIFY][mdi->url_opt_index] != NULL)
	{
		memcpy(p_td_buf, mdi->opt[MEDIA_OPT_LAST_MODIFY][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_LAST_MODIFY][mdi->url_opt_index]->opt_len);
		p_td_buf += mdi->opt[MEDIA_OPT_LAST_MODIFY][mdi->url_opt_index]->opt_len;
	}

	mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index] = (opt_in_t*)malloc(sizeof(opt_in_t));	
	mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_len = td_buflen;
	mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_type = 0;
	mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value = (char*)malloc(td_buflen);
	memcpy(mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value,td_buf,td_buflen);	
	if(NULL!=td_buf)
	{
		free(td_buf);
	}

}

long set_td_data(media_t* mdi, frag_in_t* frg)
{
	uint64_t 	cur_td_datalen = 0;
	
	if(mdi->td_complete==0 && frg->offset<g_frag_cfg.td_data_maxsize)
	{
		if(NULL==mdi->td_data)
		{
			mdi->td_data = (char*)malloc(g_frag_cfg.td_data_maxsize);
		}
		if(NULL!=mdi->td_data)
		{
			cur_td_datalen = ((frg->offset+frg->datalen)>g_frag_cfg.td_data_maxsize) ? (g_frag_cfg.td_data_maxsize-frg->offset): frg->datalen;
			memcpy(mdi->td_data+frg->offset, frg->data, cur_td_datalen);
			mdi->td_datalen += cur_td_datalen;
			/*td_data完整才能查询*/
			if(mdi->td_datalen==g_frag_cfg.td_data_maxsize)
			{
				//url is not empty,等待多源查询URL，结合url生成TD
				if(NULL != mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
				{					
					FLAG_SET(mdi->td_query,TD_QUERY_TYPE_DEDUP);
					generate_td_meta(mdi);	
					caculate_md5(mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_len, mdi->td_data, mdi->td_datalen, mdi->td, TD_LEN);
					mdi->td_complete = 1;
					return 1;					
				}				
			}					
		}				
	}
	return 0;
}

void soqav_query_timeout(void * context)
{
	struct timer_context_t *ctx = (struct timer_context_t *)context;
	uint64_t mid = ctx->mid;
	long rec_cb = 0;

	query_result_t result;

	result.status = 0;	
	result.cpz_mg_ip = 0;
	result.cpz_payload_ip = 0;
	result.reserve = 0;
	result.mid = mid;	
	atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_DEDUP_ACK_TIMEOUT]); 		
	
	MESA_htable_search_cb(g_frag_run.media_hash, (const uint8_t *)&mid, sizeof(mid),
						  media_query_ack, &result, &rec_cb);		
}

void soqav_query_free_timeout(void * context)
{
   if(NULL!=context)
   {
		free(context);
   }
}

void free_query_detail(query_detail_t* query_detail)
{
	if(NULL!=query_detail)
	{			
		switch(query_detail->addrtype)
		{
			case ADDR_TYPE_IPV4:
				if(NULL!=query_detail->tuple4_v4)
				{
					free(query_detail->tuple4_v4);
				}
				break;
			case ADDR_TYPE_IPV6:
				if(NULL!=query_detail->tuple4_v6)
				{
					free(query_detail->tuple4_v6);
				}
				break;
		}		
		if(NULL!=query_detail->url)
		{
			free((void*)query_detail->url);
		}
		free(query_detail);
	}
}

void proc_media_multisrc(media_t* mdi, int timeout)
{
	query_detail_t*			query_detail = NULL;
	struct stream_tuple4_v4 tuple4_v4 ;
	struct stream_tuple4_v6 tuple4_v6 ;
	
	/*判断是否需要发起查询*/
	/*对准超时之后一定会发起查询*/
	if(!FLAG_TEST(mdi->td_query, TD_QUERY_TYPE_YES)&& 
		(((FLAG_TEST(mdi->td_query, TD_QUERY_TYPE_DEDUP)||mdi->byte_proc>g_frag_cfg.td_data_maxsize)
		&& FLAG_TEST(mdi->td_query, TD_QUERY_TYPE_MULTISRC))||timeout))
	{
		/*补充生成td_meta,继而生成td*/
		if((g_frag_cfg.av_dedup_switch) && (mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index] == NULL))
		{
			generate_td_meta(mdi);
			if(mdi->td_complete != 1 && mdi->td_datalen==g_frag_cfg.td_data_maxsize)
			{
				caculate_md5(mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_TD_META][mdi->url_opt_index]->opt_len, mdi->td_data, mdi->td_datalen, mdi->td, TD_LEN); 		
				mdi->td_complete = 1;
			}
		}
		
		FLAG_SET(mdi->td_query, TD_QUERY_TYPE_YES);
		/*如果需要进行多源或者降载查询，填充query_detail*/
		query_detail = (query_detail_t*)calloc(1, sizeof(query_detail_t));
		query_detail->total_len = mdi->media_len;		
		query_detail->mid = mdi->mid;	
		
		/*addr*/	
		if(NULL!=mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index])
		{
			query_detail->addrtype = addrlist_to_streamtuple4(mdi->opt[MEDIA_OPT_ADDR][mdi->url_opt_index]->opt_value, &tuple4_v4, &tuple4_v6);		
			switch(query_detail->addrtype)
			{
				case ADDR_TYPE_IPV4:
					query_detail->tuple4_v4 = (struct stream_tuple4_v4*)malloc(sizeof(struct stream_tuple4_v4));;
					memcpy(query_detail->tuple4_v4, &tuple4_v4, sizeof(tuple4_v4));
					break;
				case ADDR_TYPE_IPV6:
					query_detail->tuple4_v6 = (struct stream_tuple4_v6*)malloc(sizeof(struct stream_tuple4_v6));;
					memcpy(query_detail->tuple4_v6, &tuple4_v6, sizeof(tuple4_v6));
					break;
			}
		}
		/*多源查询*/
		if(FLAG_TEST(mdi->td_query, TD_QUERY_TYPE_MULTISRC)) 
		{
			/*设置定时器*/
			struct timer_context_t *context = (struct timer_context_t *)calloc(1, sizeof(struct timer_context_t));
			context->mid = mdi->mid;
			MESA_timer_add(g_frag_run.multisrc_timer[mdi->thread_seq],
						   time(NULL),
						   g_frag_cfg.multisrc_wait_timeout,
						   soqav_query_timeout,
						   context,
						   soqav_query_free_timeout,
						   &mdi->timer_idx);
			g_frag_run.multisrc_timer_on[mdi->thread_seq] = 1;
			if(NULL!=mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
			{
				query_detail->url = (char*)malloc(mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
				query_detail->urllen = mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len;
				memcpy((void*)query_detail->url, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_value, mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index]->opt_len);
			}
		}		
		if(FLAG_TEST(mdi->td_query, TD_QUERY_TYPE_DEDUP)) 
		{
			atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_TD_QUERY]);
			soqav_dedup_query(g_frag_run.dedup_hd, mdi->td, query_detail, soqav_query_callback, (void*)mdi->mid);	
		}	
		else
		{	
			soqav_dedup_query(g_frag_run.dedup_hd, NULL, query_detail, soqav_query_callback, (void*)mdi->mid);	
		}
		atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_DEDUP_QUERY]); 
		frag_write_to_log(SEND_AV_DEDUP_QUERY, mdi->mid, mdi, NULL, 0); 
		if(NULL!=query_detail)
		{		
			free_query_detail(query_detail);		
			query_detail =NULL;
		}	
	}	
}

