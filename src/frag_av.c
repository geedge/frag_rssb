/*
* 音视频碎片拼接
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
#include <assert.h>

#include "cJSON.h"
#include "MESA_handle_logger.h"
#include "AV_interface.h"
#include "AV_sendback_in.h"
#include "soqav_dedup.h"
#include "frag_reassembly_in.h"
#include "frag_dedup.h"
#include "frag_av.h"
#include "frag_redis.h"
#include "frag_proc.h"
#include "frag_app.h"
#include "frag_voip.h"
#include "log.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;
extern frag_reassembly_t 			frag_rssb;

extern const char*                  hash_eliminate_type[3];

/*for sifter*/
const char* 		frag_info_str[FRAG_UNIT_INFO_NUM]= 
{
	"FRAG_UNIT_ID", 
	"FRAG_UNIT_ABOFFSET", 
	"FRAG_UNIT_REOFFSET",
	"MEDIA_ID",
	"MEDIA_SIZE",
	"MEDIA_NAME"
};
const char* 		frag_str = "FRAG_CONTENT";

/*查询index的参数*/
#define INDEX_REDIS_CMMD_NUM 9
const char* g_index_cmmd_argv[INDEX_REDIS_CMMD_NUM] =
{
        "HMGET",
        "",
        "MEDIA_ID",
        "MEDIA_SIZE",
        "REOFFSET",
        "ABOFFSET",
        "CAPIP",
        "REFERER",
        "USER_AGENT",       
};

int expire_cnvg_hash_node(void *data, int eliminate_type)
{
	frag_unit_t*	frg_unit = (frag_unit_t*)data; 
	switch(eliminate_type)
	{
		case ELIMINATE_TYPE_NUM:			
			atomic_inc(&frag_rssb.sysinfo_stat[RSSB_CNVG_HASH][HASH_NUM_EXPIRE]);	
			break;
		case ELIMINATE_TYPE_TIME:
			atomic_inc(&frag_rssb.sysinfo_stat[RSSB_CNVG_HASH][HASH_TIME_EXPIRE]);	
			break;
		default:
			break;

	}
	MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} expire_cnvg_hash_node %s: [PID: %llu]", 
							__FILE__,__LINE__, 
							hash_eliminate_type[eliminate_type],
							frg_unit->pid); 
	return 1;
}

int select_maat_result(struct Maat_rule_t maat_result[],int result_num)
{
	int best_idx = 0;
	int max_config = maat_result[0].config_id;
	for(int i=1;i<result_num;i++)
	{
		if(max_config<maat_result[i].config_id)
		{
			best_idx = i;
			max_config = maat_result[i].config_id;
		}
	}
	return best_idx;
}

/*暂时全部使用url扫描*/
int frag_forecast_identify(frag_unit_t* frg_unit)
{		
	int					ret = 0;
	scan_status_t 		mid = NULL;		
	int 				hit_result = 0;	
	char				url_name[32] = "HTTP_MESSAGE_URL";	
	struct Maat_rule_t	maat_result[MAAT_RESULT_NUM];		
	int 				found_pos[MAAT_RESULT_NUM] = {0};	
	int 				idx = 0;
	
	ret = Maat_set_scan_status(g_frag_run.feather, &mid, MAAT_SET_SCAN_DISTRICT, url_name, strlen(url_name));
	if(ret<0)
	{
		return -1;
	}
	hit_result = Maat_full_scan_string(g_frag_run.feather, g_frag_run.expr_tableid, CHARSET_GBK, 
										frg_unit->opt[MEDIA_OPT_URL]->opt_value, frg_unit->opt[MEDIA_OPT_URL]->opt_len, 
										maat_result, found_pos, MAAT_RESULT_NUM, &(mid), frg_unit->thread_seq);	
	if(hit_result<=0)
	{
		return -1;
	}
	
	idx = select_maat_result(maat_result, hit_result);
	if(NULL!=maat_result)
	{
		frg_unit->service_id = maat_result[idx].service_id;	
		frg_unit->media_type = maat_result[idx].do_blacklist;			
	}	
	return 0;
}

/**
*
*return query succ or fail : -1:fail 0:succ ,but no result  1:succ have result
*/
char* debug_url = "edge.hls.ll.hitbox.tv/hls/spacemonkeylive_480p/1464245925824.ts";
//char* debug_url = "60.210.16.46/videos/v0/20180302/f7/f0/f6310d0abe236a2d100f7678e9b2f65a.f4v?key=0560530077e035bcbdeef5b629b5f4cee&dis_k=2ee2c572aa7c51a7b53aa2394b12658eb&dis_t=1520219255&dis_dz=CNC-BeiJing&dis_st=42&src=iqiyi.com&uuid=ca2b94bd-5a9cb477-14f&rn=1520219254781&qd_ip=ca2b94bd&qyid=6444c99ac2ccc0d0f102e6fb781267a6&qd_tm=1520219251910&qd_vipdyn=0&cross-domain=1&pri_idc=zibo5_cnc&pv=0.1&qd_aid=948563500&qd_stert=0&qypid=948563500_02020031010000000000&qd_p=ca2b94bd&qd_uid=0&qd_src=01010031010000000000&qd_index=1&qd_vip=0&qd_tvid=948563500&qd_vipres=0&qd_k=a54e93591c62da7dd946316c13028f51";
//char* debug_url = "60.210.16.46/videos/v0/20180302/f7/f0/f6310d0abe236a2d100f7678e9b2f65a.f4v?key=0560530077e035bcbdeef5b629b5f4cee&dis_k=2ee2c572aa7c51a7b53aa2394b12658eb&dis_t=1520219255&dis_dz=CNC-BeiJing&dis_st=42&src=iqiyi.com&uuid=ca2b94bd-5a9cb477-14f&rn=1520219254781&qd_ip=ca2b94bd&qyid=6444c99ac2ccc0d0f102e6fb781267a6&qd_tm=1520219251910&qd_vipdyn=0&cross-domain=1&pri_idc=zibo5_cnc&pv=0.1&qd_aid=948563500&qd_stert=0&qypid=948563500_02020031010000000000&qd_p=ca2b94bd&qd_uid=0&qd_src=01010031010000000000&qd_index=1&qd_vip=0&qd_tvid=948563500&qd_vipres=0&qd_k=a54e93591c62da7dd946316c13028f51&range=800768-11890687";
int redis_converge_query(frag_unit_t*  frg_unit, int thread_seq)
{
	if(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]==NULL) return 0;	
	/*debug*/
	/*	
	frg_unit->opt[MEDIA_OPT_URL] = (opt_in_t*)malloc(sizeof(opt_in_t));	
	frg_unit->opt[MEDIA_OPT_URL]->opt_len = strlen(debug_url);  
	frg_unit->opt[MEDIA_OPT_URL]->opt_type = META_OPT_LAYER_URL;
	frg_unit->opt[MEDIA_OPT_URL]->opt_value = (char*)malloc(strlen(debug_url));
	memcpy(frg_unit->opt[MEDIA_OPT_URL]->opt_value, debug_url, strlen(debug_url));		
	frg_unit->text = (text_t*)realloc(frg_unit->text, sizeof(text_t)*(frg_unit->text_num+1));
	memset(frg_unit->text[frg_unit->text_num].text_name,0,sizeof(frg_unit->text[frg_unit->text_num].text_name));
	frg_unit->text[frg_unit->text_num].text_len = frg_unit->opt[MEDIA_OPT_URL]->opt_len;
	frg_unit->text[frg_unit->text_num].text_type = META_OPT_LAYER_URL;
	frg_unit->text[frg_unit->text_num].text = (char*)malloc(frg_unit->opt[MEDIA_OPT_URL]->opt_len);
	memcpy(frg_unit->text[frg_unit->text_num].text, frg_unit->opt[MEDIA_OPT_URL]->opt_value, frg_unit->opt[MEDIA_OPT_URL]->opt_len);
	frg_unit->text_num++;	
	return 1;	
	*/
	int rec = redis_av_query(frg_unit);
	if(rec==1)
	{
		/*save for sifter*/
		frg_unit->text = (text_t*)realloc(frg_unit->text, sizeof(text_t)*(frg_unit->text_num+1));
		memset(frg_unit->text[frg_unit->text_num].text_name,0,sizeof(frg_unit->text[frg_unit->text_num].text_name));
		frg_unit->text[frg_unit->text_num].text_len = frg_unit->opt[MEDIA_OPT_URL]->opt_len;
		frg_unit->text[frg_unit->text_num].text_type = META_OPT_LAYER_URL;
		frg_unit->text[frg_unit->text_num].text = (char*)malloc(frg_unit->opt[MEDIA_OPT_URL]->opt_len);
		memcpy(frg_unit->text[frg_unit->text_num].text, frg_unit->opt[MEDIA_OPT_URL]->opt_value, frg_unit->opt[MEDIA_OPT_URL]->opt_len);
		frg_unit->text_num++;
	}
	return rec;
}
/**
*
*return query succ or fail : -1:fail 0:succ ,but no result  1:succ have result
*/
int redis_converge_query_1(frag_unit_t*  frg_unit, int thread_seq)
{	
	int 			rec = 0;	
	char 			command[REDIS_CMMD_MAXLEN] = {0};	
	int 			cmmd_argc = 0;
	const char* 	cmmd_argv[7] = {NULL}; 
	size_t			cmmd_argvlen[7] = {0};
	redisReply*		reply = NULL;
	char			pid[32] = {0};
	
	snprintf(command, sizeof(command), "HMGET %" PRIu64 " C2S_URL USER_AGENT REFERER S2C_CONTENT CAPIP", frg_unit->pid);
	//rec = redis_excute_command(thread_seq, frag_rssb.logger, &reply, command);
	cmmd_argc = 7;
	cmmd_argv[0] = "HMGET";
	cmmd_argvlen[0] = strlen("HMGET");
	snprintf(pid, sizeof(pid), "%" PRIu64 "", frg_unit->pid);	
	cmmd_argv[1] = pid;
	cmmd_argvlen[1] = strlen(pid);
	cmmd_argv[2] = "C2S_URL";
	cmmd_argvlen[2] = strlen("C2S_URL");
	cmmd_argv[3] = "USER_AGENT";
	cmmd_argvlen[3] = strlen("USER_AGENT");
	cmmd_argv[4] = "REFERER";
	cmmd_argvlen[4] = strlen("REFERER");
	cmmd_argv[5] = "S2C_CONTENT";
	cmmd_argvlen[5] = strlen("S2C_CONTENT");
	cmmd_argv[6] = "CAPIP";
	cmmd_argvlen[6] = strlen("CAPIP");
	rec = redis_excute_command(thread_seq, frag_rssb.logger, &reply, command ,cmmd_argc, cmmd_argv, cmmd_argvlen);
	if(0==rec)
	{
		/*单向流信息hash 存储顺序key(PID) URL UA REFERER S2C_CONTENT*/
		for(size_t i=0;i<reply->elements;i++)
		{
			/*所有都是REDIS_REPLY_STRING，统一判断*/
			if(reply->element[i]->type==REDIS_REPLY_STRING)
			{
				/*查询有结果*/
				rec = 1;
				switch(i)
				{
					/*url*/
					case 0:
						frg_unit->opt[MEDIA_OPT_URL] = (opt_in_t*)malloc(sizeof(opt_in_t));
						frg_unit->opt[MEDIA_OPT_URL]->opt_len = reply->element[i]->len;  
						frg_unit->opt[MEDIA_OPT_URL]->opt_value = (char*)malloc(reply->element[i]->len);
						frg_unit->opt[MEDIA_OPT_URL]->opt_type = META_OPT_LAYER_URL;
						memcpy(frg_unit->opt[MEDIA_OPT_URL]->opt_value, reply->element[i]->str, reply->element[i]->len);
						/*save for sifter*/
						frg_unit->text = (text_t*)realloc(frg_unit->text, sizeof(text_t)*(frg_unit->text_num+1));
						memset(frg_unit->text[frg_unit->text_num].text_name,0,sizeof(frg_unit->text[frg_unit->text_num].text_name));
						frg_unit->text[frg_unit->text_num].text_len = reply->element[i]->len;
						frg_unit->text[frg_unit->text_num].text_type = META_OPT_LAYER_URL;
						frg_unit->text[frg_unit->text_num].text = (char*)malloc(reply->element[i]->len);
						memcpy(frg_unit->text[frg_unit->text_num].text, reply->element[i]->str, reply->element[i]->len);
						frg_unit->text_num++;
						break;
						
					/*ua*/
					case 1:
						frg_unit->opt[MEDIA_OPT_UA]= (opt_in_t*)malloc(sizeof(opt_in_t));
						frg_unit->opt[MEDIA_OPT_UA]->opt_len = reply->element[i]->len;  
						frg_unit->opt[MEDIA_OPT_UA]->opt_value = (char*)malloc(reply->element[i]->len);
						frg_unit->opt[MEDIA_OPT_UA]->opt_type = META_OPT_USER_AGENT;
						memcpy(frg_unit->opt[MEDIA_OPT_UA]->opt_value, reply->element[i]->str, reply->element[i]->len);
						break;
					
					/*referer*/
					case 2:
						frg_unit->opt[MEDIA_OPT_REFERER] = (opt_in_t*)malloc(sizeof(opt_in_t));
						frg_unit->opt[MEDIA_OPT_REFERER]->opt_len = reply->element[i]->len;  
						frg_unit->opt[MEDIA_OPT_REFERER]->opt_value = (char*)malloc(reply->element[i]->len);
						frg_unit->opt[MEDIA_OPT_REFERER]->opt_type = META_OPT_USER_AGENT;
						memcpy(frg_unit->opt[MEDIA_OPT_REFERER]->opt_value, reply->element[i]->str, reply->element[i]->len);
						break;
						
					/*s2c_content*/
					case 3:
						/*save for sifter*/
						frg_unit->text = (text_t*)realloc(frg_unit->text, sizeof(text_t)*(frg_unit->text_num+1));
						memset(frg_unit->text[frg_unit->text_num].text_name,0,sizeof(frg_unit->text[frg_unit->text_num].text_name));
						frg_unit->text[frg_unit->text_num].text_len = reply->element[i]->len;
						frg_unit->text[frg_unit->text_num].text_type = META_OPT_S2C_CONT;
						frg_unit->text[frg_unit->text_num].text = (char*)malloc(reply->element[i]->len);
						memcpy(frg_unit->text[frg_unit->text_num].text, reply->element[i]->str, reply->element[i]->len);
						frg_unit->text_num++;				
						break;

					/*capIP*/
					case 4:						
						//inet_pton(AF_INET, reply->element[i]->str, (void*)&qd_ip);
						//save_IP(frg_unit->qd_ip, CAPTURE_IP_NUM, &frg_unit->qd_ip_idx_last, qd_ip);
						break;
						
					default:
						break;
				}				
			}			
		}		
		freeReplyObject(reply);
	}	

	/*删除单向流信息由索引节点实现*/	
	return rec;		
}

/**
*
*return query succ or fail : -1:fail 0:succ ,but no result  1:succ have result
*/
int redis_index_query(frag_unit_t*  frg_unit, int thread_seq)
{	
	int 			rec = 0;	
	char 			command[REDIS_CMMD_MAXLEN] = {0};	
	char*			p_cmmd = command;	
	int 			cmmd_len = 0;
	size_t			cmmd_argvlen[INDEX_REDIS_CMMD_NUM] = {0};
	redisReply*		reply = NULL;
	char*			index_key = (char*)malloc(frg_unit->frg_info[FRAG_UNIT_ID]->text_len+1);

	/*索引信息的key是frag_unit_id*/
	memcpy(index_key, frg_unit->frg_info[FRAG_UNIT_ID]->text, frg_unit->frg_info[FRAG_UNIT_ID]->text_len);
	index_key[frg_unit->frg_info[FRAG_UNIT_ID]->text_len] = '\0';
	
	g_index_cmmd_argv[1] = index_key;
	for(int i=0;i<INDEX_REDIS_CMMD_NUM;i++)
	{
		cmmd_argvlen[i] = strlen(g_index_cmmd_argv[i]);
	}	

	/*just for record log*/
	for(int i=0;i<INDEX_REDIS_CMMD_NUM;i++)
	{		
		snprintf(p_cmmd, REDIS_CMMD_MAXLEN-cmmd_len, "%s ", g_index_cmmd_argv[i]);		
		cmmd_len = strlen(g_index_cmmd_argv[i])+1;
		p_cmmd += cmmd_len;
	}
	rec = redis_excute_command(thread_seq, frag_rssb.logger, &reply, command ,INDEX_REDIS_CMMD_NUM, g_index_cmmd_argv, cmmd_argvlen);
	if(0==rec)
	{
		/*索引信息hash 存储顺序key(frag_unit_id) MEDIA_ID MEDIA_LEN REOFFSET ABOFFSET*/
		for(size_t i=0;i<reply->elements;i++)
		{			
			switch(i)
			{
				/*media_id*/
				case 0:						
					if(reply->element[i]->type==REDIS_REPLY_STRING)
					{	
						rec = 1;
					 	frg_unit->mid = make_mid(reply->element[i]->str, reply->element[i]->len, PID_TYPE_URL);
						/*索引URL*/
						frg_unit->opt[MEDIA_OPT_INDEX_URL] = (opt_in_t*)malloc(sizeof(opt_in_t));
						frg_unit->opt[MEDIA_OPT_INDEX_URL]->opt_len = reply->element[i]->len;  
						frg_unit->opt[MEDIA_OPT_INDEX_URL]->opt_value = (char*)malloc(reply->element[i]->len);
						frg_unit->opt[MEDIA_OPT_INDEX_URL]->opt_type = OPT_FRAG_INDEX_URL;
						memcpy(frg_unit->opt[MEDIA_OPT_INDEX_URL]->opt_value, reply->element[i]->str, reply->element[i]->len);						
					}
					break;
					
				/*media_len*/
				case 1:
					if(reply->element[i]->type==REDIS_REPLY_STRING)
					{									
						//rec = 1;						
						frg_unit->media_len = strtoul(reply->element[i]->str, NULL, 10);
					}					
					break;
			
				/*reoffser*/
				case 2:
					if(reply->element[i]->type==REDIS_REPLY_STRING)
					{						
						//rec = 1;
						frg_unit->re_offset = strtoul(reply->element[i]->str, NULL, 10);
						//MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,                                                                                                                                                
                                                  //                      "{%s:%d} %20s [PID:%" PRIu64 ", MID:%" PRIu64 ", reoffset:%lld, FRAG_UNIT_ID:%s, INDEX_URL:%s]",                                                                                    
                                                    //                    __FILE__,__LINE__, "recv_index_ack_1", frg_unit->pid, frg_unit->mid, frg_unit->re_offset, index_key, reply->element[0]);
					}						
					break;
					
				/*aboffser*/
				case 3:
					if(reply->element[i]->type==REDIS_REPLY_STRING)
					{						
						//rec = 1;
						frg_unit->ab_offset = strtoul(reply->element[i]->str, NULL, 10);
					}
					break;

				/*capIP*/
				case 4:
					if(reply->element[i]->type==REDIS_REPLY_STRING)
					{					
						//rec = 1;
						//inet_pton(AF_INET, reply->element[i]->str, (void*)&qd_ip);
						//save_IP(frg_unit->qd_ip, CAPTURE_IP_NUM, &frg_unit->qd_ip_idx_last, qd_ip);
					}
					break;

				case 5:						
					if(reply->element[i]->type==REDIS_REPLY_STRING)
					{	
						/*索引REFERER*/
						frg_unit->opt[MEDIA_OPT_INDEX_REFERER] = (opt_in_t*)malloc(sizeof(opt_in_t));
						frg_unit->opt[MEDIA_OPT_INDEX_REFERER]->opt_len = reply->element[i]->len;  
						frg_unit->opt[MEDIA_OPT_INDEX_REFERER]->opt_value = (char*)malloc(reply->element[i]->len);
						frg_unit->opt[MEDIA_OPT_INDEX_REFERER]->opt_type = 0;
						memcpy(frg_unit->opt[MEDIA_OPT_INDEX_REFERER]->opt_value, reply->element[i]->str, reply->element[i]->len);						
					}
					break;

				case 6:						
					if(reply->element[i]->type==REDIS_REPLY_STRING)
					{							
						/*索引UA*/
						frg_unit->opt[MEDIA_OPT_INDEX_UA] = (opt_in_t*)malloc(sizeof(opt_in_t));
						frg_unit->opt[MEDIA_OPT_INDEX_UA]->opt_len = reply->element[i]->len;  
						frg_unit->opt[MEDIA_OPT_INDEX_UA]->opt_value = (char*)malloc(reply->element[i]->len);
						frg_unit->opt[MEDIA_OPT_INDEX_UA]->opt_type = 0;
						memcpy(frg_unit->opt[MEDIA_OPT_INDEX_UA]->opt_value, reply->element[i]->str, reply->element[i]->len);						
					}
					break;
				
				default:
					break;
			}						
		}		
		freeReplyObject(reply);		
	}	
	free(index_key);
	/*索引信息不需要删除*/	
	return rec;		
}

void set_frag_unit_after_sifter(frag_unit_t*  frg_unit)
{
	char temp[32] = {0};
	
	/*整理frg_unit*/
	if(NULL!=frg_unit->frg_info[MEDIA_ID])
	{
		frg_unit->mid = make_mid(frg_unit->frg_info[MEDIA_ID]->text, frg_unit->frg_info[MEDIA_ID]->text_len, PID_TYPE_URL);
		/*to save memory*/
		free_text(&(frg_unit->frg_info[MEDIA_ID]), 1);
	}	
	if(NULL!=frg_unit->frg_info[FRAG_UNIT_ABOFFSET])
	{
		memcpy(temp, frg_unit->frg_info[FRAG_UNIT_ABOFFSET]->text, MIN(frg_unit->frg_info[FRAG_UNIT_ABOFFSET]->text_len,31));
		frg_unit->ab_offset = strtoul(temp, NULL, 10);
		/*to save memory*/
		free_text(&(frg_unit->frg_info[FRAG_UNIT_ABOFFSET]), 1);
	}
	if(NULL!=frg_unit->frg_info[FRAG_UNIT_REOFFSET])
	{
		memcpy(temp, frg_unit->frg_info[FRAG_UNIT_REOFFSET]->text, MIN(frg_unit->frg_info[FRAG_UNIT_REOFFSET]->text_len,31));
		frg_unit->re_offset = strtoul(temp, NULL, 10);
		/*to save memory*/
		free_text(&(frg_unit->frg_info[FRAG_UNIT_REOFFSET]), 1);
	}
	if(NULL!=frg_unit->frg_info[MEDIA_SIZE])
	{
		memcpy(temp, frg_unit->frg_info[MEDIA_SIZE]->text, MIN(frg_unit->frg_info[MEDIA_SIZE]->text_len,31));
		frg_unit->media_len = strtoul(temp, NULL, 10);
		/*to save memory*/
		free_text(&(frg_unit->frg_info[MEDIA_SIZE]), 1);
	}	
}

void frag_cnvg_sifter(frag_unit_t*  frg_unit, int thread_seq)
{	
	/*extract*/
	/*only keep one result*/
	/*to do filecontent boundary*/
	expect_outcome_t res[SIFTER_MAX_NUM];	
	memset(res,0,sizeof(res));
	for(int i=(int)FRAG_UNIT_ID;i<=(int)MEDIA_NAME;i++)
	{
		/*avoid repeat extract*/
		if(NULL!=frg_unit->frg_info[i])
		{
			continue;
		}
		if(0!=sifter(frag_rssb.sifter, frg_unit->service_id, 0, frag_info_str[i], 
					 SIFTER_MAX_NUM, res, frg_unit->text, frg_unit->text_num, NULL,0, thread_seq))
		{			
			frg_unit->frg_info[i] = (text_t*)calloc(1, sizeof(text_t));
			/*copy because frg_unit->text will be free*/
			if(FRAG_UNIT_ID==i||MEDIA_ID==i)
			{							
				frg_unit->frg_info[i]->text_len = res[0].datalen;
				frg_unit->frg_info[i]->text = (char*)malloc(frg_unit->frg_info[i]->text_len);
				memcpy(frg_unit->frg_info[i]->text, res[0].data, res[0].datalen);
			}
			else
			{
				frg_unit->frg_info[i]->text_len = res[0].datalen;
				frg_unit->frg_info[i]->text = (char*)malloc(frg_unit->frg_info[i]->text_len);
				memcpy(frg_unit->frg_info[i]->text, res[0].data, res[0].datalen);
			}
			memcpy(frg_unit->frg_info[i]->text_name, frag_info_str[i], strlen(frag_info_str[i]));
			frg_unit->frg_info[i]->text_type = i;
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
						   		    "{%s:%d} frag_sifter_default: [PID: %llu, service_id: %u, expect: (%s)(%s)(%u) ]", 
						    	    __FILE__,__LINE__, frg_unit->pid, frg_unit->service_id, frag_info_str[i], frg_unit->frg_info[i]->text, 
						    	    frg_unit->frg_info[i]->text_len);			
		}
	}

	/*if have MEDIA_ID or FRAG_UNIT_ID, converge is finished */
	if(NULL!=frg_unit->frg_info[MEDIA_ID] || NULL!=frg_unit->frg_info[FRAG_UNIT_ID])
	{
		frg_unit->frag_state = STAT_CNVG_OK;
	}
	
	if(STAT_CNVG_OK==frg_unit->frag_state)
	{
		/*to save memcory, free some useless content*/
		if(NULL!=frg_unit->text)
		{			
			free_text((&(frg_unit->text)), frg_unit->text_num);	
			frg_unit->text_num = 0;		
		}
		set_frag_unit_after_sifter(frg_unit);			
	}	
}

int frag_index_query(frag_unit_t*  frg_unit, int thread_seq)
{	
	int query_succ = 0;
	/*need index query*/
	frg_unit->frag_state = STAT_INDEX_QUERY;
	if(1==redis_index_query(frg_unit, thread_seq))
	{			
		query_succ = 1;
		frg_unit->frag_state = STAT_OK;
	}	
	return query_succ;	
}

int frag_cnvg_query(frag_unit_t*  frg_unit, int thread_seq)
{		
	int query_succ = 0;
	frg_unit->frag_state = STAT_CNVG_QUERY; 				
	if(1==redis_converge_query(frg_unit, frg_unit->thread_seq))
	{
		/*通过碎片预告识别的音视频，需要再次识别具体的协议类型以及抽取模板*/
		if(g_frag_cfg.forecast_switch && frg_unit->media_type==FILE_MAYBE_FRAG)
		{			
			atomic_inc(&frag_rssb.stat_info[RSSB_FRAG_FORECAST]);
			if(-1==frag_forecast_identify(frg_unit))
			{		
				frag_write_to_log(FRAG_FORECAST_ERROR, frg_unit->pid, frg_unit, NULL, 0);
				return -1;
			}
			else
			{				
				atomic_inc(&frag_rssb.stat_info[RSSB_FRAG_FORECAST_OK]);
			}
		}						
		query_succ = 1;
		frag_cnvg_sifter(frg_unit, frg_unit->thread_seq);
		if(STAT_CNVG_OK!=frg_unit->frag_state)
		{								
			/*查询单向流也无法抽取特征,按照普通音视频传输，不进行碎片整形*/
			frg_unit->service_id = 0;		
			frg_unit->frag_state = STAT_OK;				
			frg_unit->mid = frg_unit->pid;
			frag_write_to_log(CNVG_FAIL_PROC, frg_unit->pid, frg_unit, NULL, 0);				
		}
	}
	return query_succ;
}

void proc_index_queue(frag_unit_t* frg_unit)
{		
	frag_ivi_info_t* 	frag_ivi_info = (frag_ivi_info_t*)calloc(1, sizeof(frag_ivi_info_t));		
	frag_in_t* 			frg = NULL;
	long 				frglen = sizeof(frg);
	long				return_val = 0;
	
	int rec = MESA_lqueue_try_get_head(frg_unit->frg_index_lq, &frg, &frglen);	
	while(0==rec)
	{		
		frag_write_to_log(GET_FRAG_FROM_INDEX_LQ, frg->mid, frg, NULL, 0);
		atomic_inc(&frag_rssb.sysinfo_stat[RSSB_INDEX_QUEUE][QUEUE_OUT]);
		frg->mid = frg_unit->mid;
		frag_ivi_info->frg_unit = frg_unit; 
		frag_ivi_info->frg = frg;	
		return_val = frag_service(frag_ivi_info, frg->src_ip, frg->thread_seq);
		if(-1==return_val)
		{		
			free_frag_in(frg,0,NULL);
			frg = NULL;
		}
		memset(frag_ivi_info, 0, sizeof(frag_ivi_info_t));
		rec = MESA_lqueue_try_get_head(frg_unit->frg_index_lq, &frg, &frglen);
	}	
	free(frag_ivi_info);
	return;
}

void drop_cnvg_queue(frag_unit_t* frg_unit)
{			
	frag_in_t* 			frg = NULL;
	long 				frglen = sizeof(frg);
	
	int rec = MESA_lqueue_try_get_head(frg_unit->frg_cnvg_lq, &frg, &frglen);
	while(0==rec)
	{		
		frag_write_to_log(GET_FRAG_FROM_CNVG_LQ, frg->mid, frg, NULL, 0);
		atomic_inc(&frag_rssb.sysinfo_stat[RSSB_CNVG_QUEUE][QUEUE_OUT]);
		free_frag_in(frg,0,NULL);		
		frg = NULL;		
		rec = MESA_lqueue_try_get_head(frg_unit->frg_cnvg_lq, &frg, &frglen);
	}
	return;
}

void proc_cnvg_queue(frag_unit_t* frg_unit)
{			
	frag_ivi_info_t* 	frag_ivi_info = (frag_ivi_info_t*)calloc(1, sizeof(frag_ivi_info_t));		
	frag_in_t* 			frg = NULL;
	long 				frglen = sizeof(frg);
	long				return_val = 0;
	
	int rec = MESA_lqueue_try_get_head(frg_unit->frg_cnvg_lq, &frg, &frglen);
	while(0==rec)
	{		
		frag_write_to_log(GET_FRAG_FROM_CNVG_LQ, frg->mid, frg, NULL, 0);
		atomic_inc(&frag_rssb.sysinfo_stat[RSSB_CNVG_QUEUE][QUEUE_OUT]);
		frg->mid = frg_unit->mid;
		frag_ivi_info->frg_unit = frg_unit; 
		frag_ivi_info->frg = frg;	
		return_val = frag_service(frag_ivi_info, frg->src_ip, frg->thread_seq);
		if(-1==return_val)
		{
			free_frag_in(frg,0,NULL);			
			frg = NULL;
		}
		memset(frag_ivi_info, 0, sizeof(frag_ivi_info_t));
		rec = MESA_lqueue_try_get_head(frg_unit->frg_cnvg_lq, &frg, &frglen);
	}
	free(frag_ivi_info);
	return;
}

int frag_redis_index_twice(frag_unit_t* 	frg_unit)
{
	int cnvg_query = 0;
	/*淘汰的时候再次查询redis*/
	if(frg_unit->frag_state==STAT_CNVG_QUERY && NULL!=frg_unit->frg_cnvg_lq)
	{
		frag_write_to_log(SEND_CNVG_QUERY_2, frg_unit->pid, frg_unit, NULL, 0); 				
		atomic_inc(&frag_rssb.stat_info[RSSB_CNVG_TWICE_QUERY_SEND]);	
		cnvg_query = frag_cnvg_query(frg_unit, frg_unit->thread_seq);
		if(1==cnvg_query)
		{
			frag_write_to_log(RECV_CNVG_ACK_2, frg_unit->pid, frg_unit, NULL, 0); 
			atomic_inc(&frag_rssb.stat_info[RSSB_CNVG_TWICE_QUERY_RECV]);					
		}
		/*不是碎片*/
		else if(-1==cnvg_query)		
		{
			drop_cnvg_queue(frg_unit);	
			MESA_lqueue_destroy(frg_unit->frg_cnvg_lq, NULL, NULL);
			return 0;
		}
		else
		{
			frag_write_to_log(CNVG_QUERY_FAIL_2, frg_unit->pid, frg_unit, NULL, 0);
		}			
		/*索引查询*/			
		if(frg_unit->frag_state==STAT_CNVG_OK)
		{	
			if(NULL!=frg_unit->frg_info && NULL!=frg_unit->frg_info[FRAG_UNIT_ID])
			{
				frag_write_to_log(SEND_INDEX_QUERY_1, frg_unit->pid, frg_unit, NULL, 0);			
				atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_ONCE_QUERY_SEND]);			
				if(frag_index_query(frg_unit, frg_unit->thread_seq))
				{
					frag_write_to_log(RECV_INDEX_ACK_1, frg_unit->pid, frg_unit, NULL, 0);
					atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_ONCE_QUERY_RECV]);					
				}
				else
				{
					if(frg_unit->frag_state==STAT_INDEX_QUERY)
					{
						frg_unit->frg_index_lq =  MESA_lqueue_create(1, 0); 		
					}
					frag_write_to_log(INDEX_QUERY_FAIL_1, frg_unit->pid, frg_unit, NULL, 0);
				}
			}
			else
			{				
				frg_unit->frag_state = STAT_OK;
			}
		}	
		if(frg_unit->frag_state!=STAT_OK)
		{			
			/*按照普通的音视频处理，不进行整形*/
			frg_unit->service_id = 0;				
			frg_unit->mid = frg_unit->pid;
			/*create media*/
			if(g_frag_cfg.app_switch)
			{
				app_change_pid(frg_unit);	
			}
			frg_unit->frag_state = STAT_OK;
		}			
		if(frg_unit->frag_state==STAT_OK)
		{				
			media_create(frg_unit);
		}	
		proc_cnvg_queue(frg_unit);
		MESA_lqueue_destroy(frg_unit->frg_cnvg_lq, NULL, NULL);
		frg_unit->frg_cnvg_lq = NULL;
	}
		
	/*淘汰的时候再次查询redis*/
	if(frg_unit->frag_state==STAT_INDEX_QUERY && NULL!=frg_unit->frg_index_lq)
	{	
		if(NULL!=frg_unit->frg_info && NULL!=frg_unit->frg_info[FRAG_UNIT_ID])
		{
			frag_write_to_log(SEND_INDEX_QUERY_2, frg_unit->pid, frg_unit, NULL, 0);			
			atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_TWICE_QUERY_SEND]);			
			if(frag_index_query(frg_unit, frg_unit->thread_seq))
			{
				frag_write_to_log(RECV_INDEX_ACK_2, frg_unit->pid, frg_unit, NULL, 0);			
				atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_TWICE_QUERY_RECV]);	
			}
			else
			{
				frag_write_to_log(INDEX_QUERY_FAIL_2, frg_unit->pid, frg_unit, NULL, 0);
			}
		}
		
		if(frg_unit->frag_state!=STAT_OK)
		{
			/*按照普通的音视频处理，不进行整形*/
			frg_unit->service_id = 0;				
			frg_unit->mid = frg_unit->pid;
			frg_unit->frag_state = STAT_OK;						
			frag_write_to_log(INDEX_FAIL_PROC, frg_unit->pid, frg_unit, NULL, 0);
		}		
		if(frg_unit->frag_state==STAT_OK)
		{
			/*create media*/
			media_create(frg_unit);
		}
		proc_index_queue(frg_unit);		
		MESA_lqueue_destroy(frg_unit->frg_index_lq, NULL, NULL);
		frg_unit->frg_index_lq = NULL;
	}
	return 0;
}

/*节目数量统计*/
void media_stat(media_t* mdi, frag_unit_t* frg_unit)
{
	/*static hls and osmf*/
	switch(mdi->media_type)
	{
		case FILE_OSMF:	
			if(frg_unit->service_id!=0)
			{
				atomic_inc(&frag_rssb.data_info[RSSB_RECV_OSMF_MEDIA][TOTAL_PKTS]);	
			}	
			else
			{
				atomic_inc(&frag_rssb.data_info[RSSB_OSMF_TO_OTHER][TOTAL_PKTS]);
			}
			break;
		case FILE_FRAG:	
			if(frg_unit->service_id!=0)
			{
				atomic_inc(&frag_rssb.data_info[RSSB_RECV_FRAG_MEDIA][TOTAL_PKTS]);	
			}	
			else
			{
				atomic_inc(&frag_rssb.data_info[RSSB_FRAG_TO_OTHER][TOTAL_PKTS]);
			}
			break;
		case FILE_HLS:					
			if(frg_unit->service_id!=0)
			{
				atomic_inc(&frag_rssb.data_info[RSSB_RECV_HLS_MEDIA][TOTAL_PKTS]);	
			}	
			else
			{
				atomic_inc(&frag_rssb.data_info[RSSB_HLS_TO_OTHER][TOTAL_PKTS]);
			}			
			break;				
		default:
			atomic_inc(&frag_rssb.data_info[RSSB_RECV_OTHER_MEDIA][TOTAL_PKTS]);			
			atomic_inc(&g_frag_stat.media_stat[LOG_MEDIA_HTTP]);			
			break;
	}		
}

/*节目大小统计*/
void media_byte_stat(media_t* mdi, frag_unit_t* frg_unit, frag_in_t* frg)
{
	switch(mdi->media_type)
	{
		case FILE_OSMF:	
			if(frg_unit!=NULL && frg_unit->service_id!=0)
			{
				atomic_add(&frag_rssb.data_info[RSSB_RECV_OSMF_MEDIA][TOTAL_BYTES],frg->datalen);
			}
			else
			{
				atomic_add(&frag_rssb.data_info[RSSB_OSMF_TO_OTHER][TOTAL_BYTES],frg->datalen);
			}
			break;

		case FILE_FRAG:	
			if(frg_unit!=NULL && frg_unit->service_id!=0)
			{
				atomic_add(&frag_rssb.data_info[RSSB_RECV_FRAG_MEDIA][TOTAL_BYTES],frg->datalen);
			}
			else
			{
				atomic_add(&frag_rssb.data_info[RSSB_FRAG_TO_OTHER][TOTAL_BYTES],frg->datalen);
			}
			break;
			
		case FILE_HLS:	
			if(frg_unit!=NULL && frg_unit->service_id!=0)		
			{
				atomic_add(&frag_rssb.data_info[RSSB_RECV_HLS_MEDIA][TOTAL_BYTES],frg->datalen);
			}
			else
			{
				atomic_add(&frag_rssb.data_info[RSSB_HLS_TO_OTHER][TOTAL_BYTES],frg->datalen);
			}
			break;

		default:
			atomic_add(&frag_rssb.data_info[RSSB_RECV_OTHER_MEDIA][TOTAL_BYTES],frg->datalen);
			break;
	}				
}

/*节目创建时*/
int media_frag_removal(media_t* mdi, frag_unit_t* frg_unit)
{	
	/*proc absolute offset, such as hls*/
	/*for hls,osmf , include content_length*/	
	if(FILE_HLS==frg_unit->media_type || FILE_OSMF==frg_unit->media_type)
	{
		/*重复的分片不需要再处理*/
		if(frag_unit_removal(mdi, frg_unit->re_offset))
		{
			frg_unit->repeat_not_proc = 1;			
			frag_write_to_log(HLS_OSMF_REPEAT, frg_unit->mid, frg_unit, NULL, 0);				
			return -1;
		}
	}
	switch(frg_unit->media_type)
	{
		case FILE_HLS:			
			frg_unit->ab_offset = mdi->acc_offset;
			frg_unit->ab_offset_for_in = mdi->acc_offset;
			mdi->acc_offset += frg_unit->content_length;
			break;	
				
		case FILE_OSMF: 
			//OSMF ab_offset can get from the frist pkt
			frg_unit->ab_offset_for_in = mdi->acc_offset;
			mdi->acc_offset += frg_unit->content_length;
			break;
				
		default:						
			break;
	}	
	return 0;
}

/*节目偏移量*/
void frag_set_offset(uint8_t media_type, frag_in_t* frg, frag_unit_t* frg_unit)
{
	/*frg->offset is relative offset*/	
	if(NULL!=frg_unit)
	{
		frg->seq = frg_unit->re_offset;
		frg->offset += frg_unit->ab_offset; 
		switch(media_type)
		{
			case FILE_OSMF:					
				frg->offset_in = frg_unit->ab_offset_for_in + frg->offset;	
				break;
			case FILE_HLS:				
				frg->offset_in = frg->offset;
				break;
			default:
				break;
		}
	}
	else
	{
		frag_write_to_log(ADD_FRAG_NOUSE, frg->pid, frg, NULL, 0);	
	}
}

/*节目内去重**/
int media_removal(media_t* mdi, frag_unit_t* frg_unit, frag_in_t* frg, frag_ivi_info_t* 	frag_ivi_info)
{
	int frag_stat = 0;
	
	/*frag removal*//*OSMF is relative offset, not need removal*//*removal here, because save mem*/
	if(FILE_OSMF==mdi->media_type || FILE_HLS==mdi->media_type)
	{
		frag_set_offset(mdi->media_type, frg, frg_unit);				
		frag_stat = 1;
	}
	/*VOIP和PIC不去重*/
	else if(mdi->proto==AV_PROTOCOL_SIP || mdi->proto==AV_PROTOCOL_HTTP_STREAM)
	{	
		frg->seq = mdi->re_offset;		
		frg->offset_in = frg->offset;	
		frag_stat = 1;	
	}
	else
	{
		if(FILE_FRAG==mdi->media_type && NULL!=frg_unit)
		{
			frg->seq = frg_unit->re_offset;
			frg->offset += frg_unit->ab_offset; 
		}
		if(g_frag_cfg.IVI_switch)
		{
			frag_stat = frag_removal_and_merge(mdi->ivi, frag_ivi_info);
		}
		else
		{
			frag_stat = 1;
		}		
		if(frag_stat==1)
		{					
			frg->offset_in = frg->offset;	
		}	
	}	
	return frag_stat;
}

/*
void generate_td_meta(frag_unit_t* frg_unit, char* url, int url_len, char* etag, int etag_len, char* last_modify, int last_modify_len, char* serverIP)
{
	char*		td_buf = NULL;
	int 		td_buflen = 0;
	char		media_len[64] = {0};
	char		media_type[16] = {0};

	snprintf(media_type, sizeof(media_type), "%hhu", frg_unit->media_type);
	if(frg_unit->service_id==0)
	{
		snprintf(media_len, sizeof(media_len), "%lu", frg_unit->media_len);
	}
	else
	{
		snprintf(media_len, sizeof(media_len), "%d", 0);
	}
	//set_frag_unit_td
	td_buflen = strlen("URL:")+url_len+strlen("ServerIP:")+strlen(serverIP)+strlen("MediaType:")+strlen(media_type)
				+strlen("MediaLen:")+strlen(media_len)+strlen("Etag:")+etag_len+strlen("LastModify:")+last_modify_len+1;
	td_buf = (char*)calloc(1, td_buflen);
	
	char* p_td_buf = td_buf;
	memcpy(p_td_buf, "URL:" ,strlen("URL:"));
	p_td_buf += strlen("URL:");
	if(url_len>0)
	{
		memcpy(p_td_buf, url ,url_len);
		p_td_buf += url_len;
	}
	memcpy(p_td_buf, "ServerIP:" ,strlen("ServerIP:"));
	p_td_buf += strlen("ServerIP:");	
	if(strlen(serverIP)>0)
	{
		memcpy(p_td_buf, serverIP ,strlen(serverIP));
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
	if(etag_len>0)
	{
		memcpy(p_td_buf, etag ,etag_len);
		p_td_buf += etag_len;
	}
	memcpy(p_td_buf, "LastModify:" ,strlen("LastModify:"));
	p_td_buf += strlen("LastModify:");
	if(last_modify_len>0)
	{
		memcpy(p_td_buf, last_modify, last_modify_len);
		p_td_buf += last_modify_len;
	}		
	frg_unit->opt[MEDIA_OPT_TD_META] = (opt_in_t*)malloc(sizeof(opt_in_t));	
	frg_unit->opt[MEDIA_OPT_TD_META]->opt_len = td_buflen;
	frg_unit->opt[MEDIA_OPT_TD_META]->opt_type = 0;
	frg_unit->opt[MEDIA_OPT_TD_META]->opt_value = (char*)malloc(td_buflen);
	memcpy(frg_unit->opt[MEDIA_OPT_TD_META]->opt_value,td_buf,td_buflen);	
	if(NULL!=td_buf)
	{
		free(td_buf);
	}
}*/
int proc_mediainfo_opt(frag_unit_t* frg_unit, rssb_media_info_t* media_info)
{
	char* 		ptr = media_info->opt;
	uint32_t	opt_len = 0;
	uint32_t	opt_value_len = 0;
	uint8_t		opt_type = 0;
	char*		opt_value = NULL;
	char		temp[32] = {0};
	char 		serverIP[64] = {0};
	int 		opt_index = -1;	
		
	/*proc not extrace opt*/
	for(int i=0;i<media_info->opt_num;i++)
	{
		opt_len = *(uint32_t*)ptr;
		opt_type = *(uint8_t*)(ptr+sizeof(uint32_t));
		opt_value = ptr+sizeof(uint32_t)+sizeof(uint8_t);		
		opt_value_len = opt_len-sizeof(uint32_t)-sizeof(uint8_t);	
		if(opt_len>100000 || opt_value_len>100000) return -1;
		
		switch(opt_type)
		{
			case META_OPT_SERVICE_ID:
				memcpy(temp, opt_value, opt_value_len);
				frg_unit->service_id = atoi(temp);				
				break;

			case META_OPT_ETAG:
				//etag = opt_value;
				//etag_len = opt_value_len;
				opt_index = MEDIA_OPT_ETAG;
				break;

			case META_OPT_LAST_MODIFY: 
				//last_modify = opt_value;
				//last_modify_len = opt_value_len;
				opt_index = MEDIA_OPT_LAST_MODIFY;
				break;
				
			case META_OPT_INFO_BEFORE_MULTISRC:
				frg_unit->qd_info_from_cpz[frg_unit->qd_info_from_cpz_idx_last].mid = *(uint64_t*)opt_value;			
				frg_unit->qd_info_from_cpz[frg_unit->qd_info_from_cpz_idx_last].cap_ip = *(uint32_t*)(opt_value+sizeof(uint64_t));	
				frg_unit->qd_info_from_cpz_idx_last++;
				break;
				
			case META_OPT_LAYER_ADDR:
				/*set serverip*/
				get_serverIP(opt_value, opt_value_len, serverIP);
				opt_index = MEDIA_OPT_ADDR;				
				break;
				
			case META_OPT_SINGLE_KEY:			
				opt_index = MEDIA_OPT_SINGLE_KEY;				
				break;

			case META_OPT_LAYER_URL:				
				frg_unit->text = (text_t*)realloc(frg_unit->text, sizeof(text_t)*(frg_unit->text_num+1));
				frg_unit->text[frg_unit->text_num].text_type = opt_type;		
				frg_unit->text[frg_unit->text_num].text_len = opt_value_len;//because include len+type in AV system
				frg_unit->text[frg_unit->text_num].text = (char*)malloc(opt_value_len);
				memcpy(frg_unit->text[frg_unit->text_num].text, opt_value, opt_value_len);
				frg_unit->text_num++;
				opt_index = MEDIA_OPT_URL;				
				break;

			case META_OPT_REFERER:				
				opt_index = MEDIA_OPT_REFERER;
				break;
				
			case META_OPT_OFFSET:
				frg_unit->ab_offset = *(uint64_t*)(opt_value);		
				break;			

			case META_OPT_USER_AGENT:				
				opt_index = MEDIA_OPT_UA;				
				break;			
			case META_OPT_SERVER:
				opt_index = MEDIA_OPT_SERVER;
				break;
			case META_OPT_C2S_CONT_TYPE:
				opt_index = MEDIA_OPT_C2S_CONT_TYPE;
				break;
			case META_OPT_S2C_CONT_TYPE:
				opt_index = MEDIA_OPT_S2C_CONT_TYPE;
				break;

			default:					
				break;
		}
		if(opt_index==MEDIA_OPT_ADDR||
			opt_index==MEDIA_OPT_SINGLE_KEY||
			opt_index==MEDIA_OPT_URL||
			opt_index==MEDIA_OPT_REFERER||
			opt_index==MEDIA_OPT_OFFSET||
			opt_index==MEDIA_OPT_UA||
			opt_index==MEDIA_OPT_SERVER||
			opt_index==MEDIA_OPT_C2S_CONT_TYPE||
			opt_index==MEDIA_OPT_S2C_CONT_TYPE||
			opt_index==MEDIA_OPT_ETAG||
			opt_index==MEDIA_OPT_LAST_MODIFY)
		{
			if(NULL==frg_unit->opt[opt_index])
			{
				frg_unit->opt[opt_index] = (opt_in_t*)malloc(sizeof(opt_in_t));	
				frg_unit->opt[opt_index]->opt_len = opt_value_len;
				frg_unit->opt[opt_index]->opt_type = opt_type;
				frg_unit->opt[opt_index]->opt_value = (char*)malloc(opt_value_len);
				memcpy(frg_unit->opt[opt_index]->opt_value,opt_value,opt_value_len);
			}
		}
		/*next */
		ptr += sizeof(uint32_t)+sizeof(uint8_t)+opt_value_len;	
	}


	//ADD by dumeijie
	if(NULL==frg_unit->opt[MEDIA_OPT_PID])
	{
		frg_unit->opt[MEDIA_OPT_PID] = (opt_in_t*)malloc(sizeof(opt_in_t)); 
		frg_unit->opt[MEDIA_OPT_PID]->opt_len = sizeof(media_info->pid);
		frg_unit->opt[MEDIA_OPT_PID]->opt_value = (char*)malloc(frg_unit->opt[MEDIA_OPT_PID]->opt_len);
		memcpy(frg_unit->opt[MEDIA_OPT_PID]->opt_value,(char*)&media_info->pid,frg_unit->opt[MEDIA_OPT_PID]->opt_len);
	}
	if(NULL==frg_unit->opt[MEDIA_OPT_CAP_IP])
	{
		char ipbuf[32] = {0};
		int ipbuf_len = 32;
		memset(ipbuf,0,ipbuf_len);
		inet_ntop(AF_INET, &media_info->cap_IP, ipbuf, ipbuf_len);
		frg_unit->opt[MEDIA_OPT_CAP_IP] = (opt_in_t*)malloc(sizeof(opt_in_t)); 
		frg_unit->opt[MEDIA_OPT_CAP_IP]->opt_len = strlen(ipbuf);
		frg_unit->opt[MEDIA_OPT_CAP_IP]->opt_value = (char*)malloc(frg_unit->opt[MEDIA_OPT_CAP_IP]->opt_len);
		memcpy(frg_unit->opt[MEDIA_OPT_CAP_IP]->opt_value,ipbuf,frg_unit->opt[MEDIA_OPT_CAP_IP]->opt_len);
	}
	if(NULL==frg_unit->opt[MEDIA_OPT_PROTOCOL])
	{
		frg_unit->opt[MEDIA_OPT_PROTOCOL] = (opt_in_t*)malloc(sizeof(opt_in_t)); 
		frg_unit->opt[MEDIA_OPT_PROTOCOL]->opt_len = sizeof(media_info->protocol);
		frg_unit->opt[MEDIA_OPT_PROTOCOL]->opt_value = (char*)malloc(frg_unit->opt[MEDIA_OPT_PROTOCOL]->opt_len);
		memcpy(frg_unit->opt[MEDIA_OPT_PROTOCOL]->opt_value,(char*)&media_info->protocol,frg_unit->opt[MEDIA_OPT_PROTOCOL]->opt_len);
	}

/*
	if(g_frag_cfg.av_dedup_switch)
	{
		generate_td_meta(frg_unit, url, url_len, etag, etag_len, last_modify, last_modify_len, serverIP);
	}*/
	return 0;
}

void set_frag_unit(rssb_media_info_t* media_info, frag_unit_t* frg_unit)
{
	frg_unit->pid = media_info->pid;	
	frg_unit->media_len = media_info->media_len;	
	frg_unit->media_type = media_info->media_type;
	frg_unit->proto = media_info->protocol;
	frg_unit->hitservice = media_info->hitservice;
	frg_unit->data_flag = media_info->data_flag;
	frg_unit->flag = media_info->flag;
	frg_unit->capIP = media_info->cap_IP;	
	frg_unit->src_ip = media_info->src_ip;		
	frg_unit->thread_seq = media_info->thread_seq;	

	/*mediainfo opt*/
	proc_mediainfo_opt(frg_unit, media_info);	
}

long converge_mediainfo_search_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{
	frag_unit_t* 			frg_unit = (frag_unit_t*)data;	
	rssb_media_info_t*	 	media_info = (rssb_media_info_t*)user_arg;
	int						query = 0;

	/*7元组生成的PID的冲突*/
	if(NULL!=frg_unit)
	{
		MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
								"{%s:%d} add_mediainfo pid exist error: [PID:%llu, media_len:%llu, media_type:0x%02x, opt_num:%u]", 
								__FILE__,__LINE__, media_info->pid, media_info->media_len, media_info->media_type, media_info->opt_num);
		MESA_htable_del(frag_rssb.converge_hash, key, size, NULL);
		frg_unit = NULL;
		atomic_inc(&frag_rssb.stat_info[RSSB_FRAG_UNIT_EXIST]);
	}
	
	if(NULL==frg_unit)
	{
	 	frg_unit = (frag_unit_t*)calloc(1,sizeof(frag_unit_t));
        if(0>(MESA_htable_add(frag_rssb.converge_hash, key, size,(const void*)frg_unit)))
        {
			MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
						    		"{%s:%d} add_mediainfo converge hash MESA_htable_add error: [PID:%llu, media_len:%llu, media_type:0x%02x, opt_num:%u]", 
									__FILE__,__LINE__, media_info->pid, media_info->media_len, media_info->media_type, media_info->opt_num);
			return -1;
		} 		
		init_frag_unit(frg_unit, media_info->protocol);	
		/*stat info*/
		atomic_inc(&frag_rssb.stat_info[RSSB_CREATE_FRAG_UNIT]);
	}
	
	if(frg_unit->repeat_not_proc) return 0;
	
	/*set mediainfo*/
	frg_unit->mediainfo_cnt++;

	/*for frag_unit_removal*/
	if(0==frg_unit->content_length)
	{
		frg_unit->content_length = media_info->media_len;
	}
	set_frag_unit(media_info, frg_unit);
	
	/*write log*/
	frag_write_to_log(ADD_META, frg_unit->pid, frg_unit, NULL, 0);	
	
	/*碎片化场景*/	
	if(frg_unit->service_id)
	{		
		/*单向流汇聚*/
		if(frg_unit->frag_state==STAT_INIT)
		{
			frag_cnvg_sifter(frg_unit, frg_unit->thread_seq);
			/*双向流汇聚*/
			if(frg_unit->frag_state!=STAT_CNVG_OK)			
			{			
				/*send single-traffic query*/
				frag_write_to_log(SEND_CNVG_QUERY_1, frg_unit->pid, frg_unit, NULL, 0);					
				atomic_inc(&frag_rssb.stat_info[RSSB_CNVG_ONCE_QUERY_SEND]); 
				query = frag_cnvg_query(frg_unit, frg_unit->thread_seq);
				/*debug for second query*/
				/*
				frg_unit->frag_state = STAT_CNVG_QUERY; 	
				query = 0;
				*/
				/*查询成功*/
				if(1==query)
				{
					frag_write_to_log(RECV_CNVG_ACK_1, frg_unit->pid, frg_unit, NULL, 0); 
					atomic_inc(&frag_rssb.stat_info[RSSB_CNVG_ONCE_QUERY_RECV]);					
				}
				/*查询成功，但是不是碎片*/
				else if(-1==query)
				{
					MESA_htable_del(frag_rssb.converge_hash, key, size, NULL);
					frg_unit = NULL;
					return -1;
				}
				else 
				{
					frag_write_to_log(CNVG_QUERY_FAIL_1, frg_unit->pid, frg_unit, NULL, 0);
				}
				if(frg_unit->frag_state==STAT_CNVG_QUERY)
				{
					frg_unit->frg_cnvg_lq =  MESA_lqueue_create(1, 0);
				}				
			}	
		}	

		/*索引查询*/
		if(frg_unit->frag_state==STAT_CNVG_OK)
		{			
			if(NULL!=frg_unit->frg_info && NULL!=frg_unit->frg_info[FRAG_UNIT_ID])
			{			
				frag_write_to_log(SEND_INDEX_QUERY_1, frg_unit->pid, frg_unit, NULL, 0);			
				atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_ONCE_QUERY_SEND]);			
				if(frag_index_query(frg_unit, frg_unit->thread_seq))
				{
					frag_write_to_log(RECV_INDEX_ACK_1, frg_unit->pid, frg_unit, NULL, 0);
					atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_ONCE_QUERY_RECV]);					
				}
				else
				{
					if(frg_unit->frag_state==STAT_INDEX_QUERY)
					{
						frg_unit->frg_index_lq =  MESA_lqueue_create(1, 0); 		
					}
					frag_write_to_log(INDEX_QUERY_FAIL_1, frg_unit->pid, frg_unit, NULL, 0);
				}
			}	
			else
			{
				frg_unit->frag_state = STAT_OK;
			}
		}
	}
	else
	{
		MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_FATAL, FRAG_REASSEMBLY_MODULE_NAME, 
						    	"{%s:%d} add_mediainfo service_id==0 error: [PID:%llu, media_len:%llu, media_type:0x%02x, opt_num:%u]", 
								__FILE__,__LINE__, media_info->pid, media_info->media_len, media_info->media_type, media_info->opt_num);	
		MESA_htable_del(frag_rssb.converge_hash, key, size, NULL);
		frg_unit = NULL;
		atomic_inc(&frag_rssb.stat_info[RSSB_FRAG_UNIT_ERROR]);
		return -1;
	}

	if(frg_unit->frag_state==STAT_OK)
	{
		/*create media*/
		media_create(frg_unit);
	}	
	return 0;
}

int is_frag(uint8_t media_type)
{
	if(media_type==FILE_OSMF || media_type==FILE_HLS || media_type==FILE_FRAG || media_type==FILE_MAYBE_FRAG ||media_type==FILE_IOS|| media_type==FILE_ANDRIOD|| media_type==FILE_APP)
	{
		return 1;
	}
	return 0;
}

int av_redis_ack_parser(frag_unit_t*  frg_unit, char* str)
{	
	cJSON* root = cJSON_Parse(str);
	cJSON* url = NULL;
	cJSON* referer = NULL;
	int rec = 0;
		
	if(NULL!=root)
	{
		url = cJSON_GetObjectItem(root, "url");
		if(NULL!=url)
		{
			frg_unit->opt[MEDIA_OPT_URL] = (opt_in_t*)malloc(sizeof(opt_in_t));	
			frg_unit->opt[MEDIA_OPT_URL]->opt_len = strlen(url->valuestring);  
			frg_unit->opt[MEDIA_OPT_URL]->opt_type = META_OPT_LAYER_URL;
			frg_unit->opt[MEDIA_OPT_URL]->opt_value = (char*)malloc(strlen(url->valuestring));
			memcpy(frg_unit->opt[MEDIA_OPT_URL]->opt_value, url->valuestring, strlen(url->valuestring));
			rec = 1;	
		}
		referer = cJSON_GetObjectItem(root, "referer");
		if(NULL!=referer)
		{
			frg_unit->opt[MEDIA_OPT_REFERER] = (opt_in_t*)malloc(sizeof(opt_in_t));	
			frg_unit->opt[MEDIA_OPT_REFERER]->opt_len = strlen(referer->valuestring);  
			frg_unit->opt[MEDIA_OPT_REFERER]->opt_type = META_OPT_REFERER;
			frg_unit->opt[MEDIA_OPT_REFERER]->opt_value = (char*)malloc(strlen(referer->valuestring));
			memcpy(frg_unit->opt[MEDIA_OPT_REFERER]->opt_value, referer->valuestring, strlen(referer->valuestring));	
		}				
	}
	cJSON_Delete(root);
	return rec;
}

/*
*return query succ or fail : -1:fail 0:succ ,but no result  1:succ have result
*/
int redis_av_query(frag_unit_t*  frg_unit)
{
	int 			rec = 0;	
	char 			command[REDIS_CMMD_MAXLEN] = {0};		
	int 			cmmd_argc = 0;
	const char* 	cmmd_argv[2] = {NULL}; 
	size_t			cmmd_argvlen[2] = {0};
	redisReply*		reply = NULL;
	char*			index_key = (char*)malloc(frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len+1);

	/*索引信息的key是frag_unit_id*/
	memcpy(index_key, frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_value, frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len);
	index_key[frg_unit->opt[MEDIA_OPT_SINGLE_KEY]->opt_len] = '\0';
	snprintf(command, sizeof(command), "GET %s", index_key);
	cmmd_argc = 2;
	cmmd_argv[0] = "GET";
	cmmd_argvlen[0] = strlen("GET");
	cmmd_argv[1] = index_key;
	cmmd_argvlen[1] = strlen(index_key);
	
	rec = redis_excute_command(frg_unit->thread_seq, frag_rssb.logger, &reply, command ,cmmd_argc, cmmd_argv, cmmd_argvlen);
	if(0==rec)
	{
		rec = av_redis_ack_parser(frg_unit, reply->str);
		freeReplyObject(reply);		
	}	
	free(index_key);
	/*索引信息不需要删除*/	
	return rec;		
}

/*传统音视频节目单向流URL反查*/
int av_query(frag_unit_t* frg_unit)
{	
	int query_succ = 0;
	/*need index query*/	
	frg_unit->frag_state = STAT_INDEX_QUERY;
	int rec = redis_av_query(frg_unit);	
	if(1==rec)
	{		
		query_succ = 1;
		frg_unit->frag_state = STAT_OK;
	}
	return query_succ;	
}

void set_av_frag_unit_from_media(media_t* mdi, int opt_index, frag_unit_t* frg_unit)
{		
	frg_unit->mid = mdi->mid;
	frg_unit->pid = mdi->mid;
	frg_unit->thread_seq = mdi->thread_seq;
	frg_unit->opt[MEDIA_OPT_SINGLE_KEY] = mdi->opt[MEDIA_OPT_SINGLE_KEY][opt_index];
}

void proc_av_opt(frag_unit_t* frg_unit, int opt_index, media_t* mdi)
{	
	if(NULL==mdi->opt[MEDIA_OPT_URL][opt_index])
	{
		mdi->opt[MEDIA_OPT_URL][opt_index] = frg_unit->opt[MEDIA_OPT_URL];
		frg_unit->opt[MEDIA_OPT_URL] = NULL;
	}	
	if(NULL==mdi->opt[MEDIA_OPT_REFERER][opt_index])
	{
		mdi->opt[MEDIA_OPT_REFERER][opt_index] = frg_unit->opt[MEDIA_OPT_REFERER];
		frg_unit->opt[MEDIA_OPT_REFERER] = NULL;
	}	
}

long index_query_timeout_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{
	media_t* mdi = (media_t*)data;
	frag_unit_t* frg_unit = (frag_unit_t*)calloc(1,sizeof(frag_unit_t));

	if(NULL!=mdi)
	{
		/*传统音视频第二次查询*/
		if(mdi->media_service_type==MEDIA_SERVICE_TYPE_AV)
		{	
			/*每一组opt都查询*/
			for(int i=0;i<mdi->opt_index;i++)
			{			
				if(NULL==mdi->opt[MEDIA_OPT_URL][i] && NULL!=mdi->opt[MEDIA_OPT_SINGLE_KEY][i])
				{
					set_av_frag_unit_from_media(mdi, i, frg_unit);
					frag_write_to_log(SEND_AV_QUERY_2, frg_unit->pid, frg_unit, NULL, 0);		
					atomic_inc(&frag_rssb.stat_info[RSSB_AV_TWICE_QUERY_SEND]);			
					if(av_query(frg_unit))
					{
						frag_write_to_log(RECV_AV_ACK_2, frg_unit->pid, frg_unit, NULL, 0); 			
						atomic_inc(&frag_rssb.stat_info[RSSB_AV_TWICE_QUERY_RECV]);				
						proc_av_opt(frg_unit, i, mdi);
					}
					else
					{
						frag_write_to_log(AV_QUERY_FAIL_2, frg_unit->pid, frg_unit, NULL, 0); 
					}				
					frg_unit->opt[MEDIA_OPT_SINGLE_KEY] = NULL;
					memset(frg_unit, 0, sizeof(frg_unit));	
				}		
			}

			/*选择第一个存在的URL*/
			if(NULL==mdi->opt[MEDIA_OPT_URL][mdi->url_opt_index])
			{			
				for(int i=0;i<mdi->opt_index;i++)
				{
					if(NULL!=mdi->opt[MEDIA_OPT_URL][i])
					{
						mdi->url_opt_index = i;
						break;
					}	
				}
			}			
			/*第二次反查之后，可以查询多源了*/
			if(g_frag_cfg.av_dedup_switch && !FLAG_TEST(mdi->td_query,TD_QUERY_TYPE_MULTISRC))
			{			
				FLAG_SET(mdi->td_query,TD_QUERY_TYPE_MULTISRC);					
				proc_media_multisrc(mdi, 1);
			}
		}		
		/*voip第二次查询*/
		else if(mdi->media_service_type==MEDIA_SERVICE_TYPE_SIP)
		{
			set_frag_unit_from_media(mdi, frg_unit);
			frag_write_to_log(SEND_VOIP_QUERY_2, frg_unit->pid, frg_unit, NULL, 0); 
			atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_TWICE_QUERY_SEND]);	
			if(sip_index_query(frg_unit, frg_unit->thread_seq))
			{			
				frag_write_to_log(RECV_VOIP_ACK_2, frg_unit->pid, frg_unit, NULL, 0);	
				atomic_inc(&frag_rssb.stat_info[RSSB_INDEX_TWICE_QUERY_RECV]);
			}
			else
			{
				frag_write_to_log(VOIP_QUERY_FAIL_2, frg_unit->pid, frg_unit, NULL, 0);	
			}			
			if(frg_unit->frag_state == STAT_OK)
			{					
				proc_sip_opt(frg_unit, mdi);
			}		
			frg_unit->sip_diadata_ID = NULL;
			frg_unit->sip_data_dir = NULL;			
		}	
	}	
	free_frag_unit(frg_unit);
	return 0;
}

void index_query_timeout(void * context)
{
	struct timer_context_t *ctx = (struct timer_context_t *)context;
	uint64_t mid = ctx->mid;
	long rec_cb = 0;

	MESA_htable_search_cb(g_frag_run.media_hash, (const uint8_t *)&mid, sizeof(mid),
						  index_query_timeout_cb, NULL, &rec_cb);
}

void index_query_timeout_free(void * context)
{
   if(NULL!=context)
   {
		free(context);
   }
}



