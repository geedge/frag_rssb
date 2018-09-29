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


#include "MESA_handle_logger.h"
#include "MESA_prof_load.h"
#include "MESA_htable.h"
#include "MESA_list_queue.h"
#include "MESA_trace.h"
#include "stream.h"

#include "cJSON.h"
#include "hiredis.h"
#include "hircluster.h"

#include "interval_index.h"
#include "soqav_dedup.h"
#include "app_detect.h"
#include "bizman.h"

#include "common.h"
#include "main.h"
#include "frag_reassembly_in.h"
#include "frag_redis.h"
#include "frag_proc.h"
#include "frag_av.h"
#include "frag_voip.h"
#include "log.h"
#include "field_stat2.h"

extern frag_rssb_parameter_t 		g_frag_run;
extern frag_rssb_configure_t 		g_frag_cfg;
extern frag_rssb_status_t 			g_frag_stat;

extern frag_reassembly_t 			frag_rssb;

int g_voip_fulllog_sock_fd = -1;
int g_voip_surveylog_sock_fd = -1;

const char* g_sip_cmmd_argv[SIP_REDIS_CMMD_NUM] =
{
        "HMGET",
        "",        
        "data_flag",
        "RTP_4tuple",
        "URI",
        "From",
        "To",
        "S_gateway",
        "C_gateway",
        "Duration",
        "S_coding",
        "C_coding",

        "From_tag",
        "To_tag",
        "Call_ID",
        "CSeq",
        "C_Contact",
        "S_Contact",
        "User_Agent",
        "Server",
        "C_crypto",
        "C_inline",
        "S_crypto",
        "S_inline",

        "Ringing_time",
        "Reason",
        "SIP_4tuple",
        "S_Via",
        "S_Record_Route",
        "S_Route",
        "C_Via",
        "C_Record_Route",
        "C_Route",
        "Rescode",        
};

/*opt_type is fulllog opt_type*/
sip_opt_t	g_sip_opt_type[SIP_OPT_NUM] = 
{
	{	"RTP_4tuple", 		0xFF},
	{	"URI",				0x20},
	{	"From",				0x25},
	{	"To",				0x26},
	{	"S_gateway",		0xFF},
	{	"C_gateway",		0xFF},
	{	"Duration",			0x2A},	//6
	{	"S_coding",			0x24}, //7
	{	"C_coding",			0x23}, //8
		
	{	"From_tag",			0xFF},	//9
	{	"To_tag",			0xFF},	//10
	{	"Call_ID",			0xFF},  //11
	{	"CSeq",				0xFF},  //12
	{	"C_Contact",		0x21},  //13
	{	"S_Contact",		0x21},	//14
	{	"User_Agent",		0xFF},  //15
	{	"Server",			0xFF},
	{	"C_crypto",			0xFF},
	{	"C_inline",			0xFF},
	{	"S_crypto",			0xFF},
	{	"S_inline", 		0xFF},

	{	"Ringing_time",		0xFF},
	{	"Reason",			0xFF},  //22
	{	"SIP_4tuple",		0xFF},  //23
	{	"S_Via",			0x22},	//24
	{	"S_Record_Route",	0x27},  //25
	{	"S_Route",			0x28},  //26
	{	"C_Via",			0x22},  //27
	{	"C_Record_Route",	0x27},  //28
	{	"C_Route",			0x28},	//29	
	{	"Rescode",			0xFF},	//30
};

void free_media_sip(media_t* mdi)
{		
	for(int i=0;i<SIP_OPT_NUM;i++)
	{
		if(NULL!=mdi->sip_opt[i])
		{
			free_opt(&mdi->sip_opt[i]);
		}
	}	
	if(NULL!=mdi->sip_rate_info)
	{
		free_opt(&mdi->sip_rate_info); 			
	}
	if(NULL!=mdi->sip_diadata_ID)
	{
		free_opt(&mdi->sip_diadata_ID); 			
	}	
	if(NULL!=mdi->jc_buf)
	{
		free(mdi->jc_buf); 			
	}	
	if(NULL!=mdi->fd_buf)
	{
		free(mdi->fd_buf); 			
	}	
}

void free_frag_unit_sip_opt(frag_unit_t* 	frg_unit)
{
	for(int i=0;i<SIP_OPT_NUM;i++)
	{
		if(NULL!=frg_unit->sip_opt[i])
		{
			free_opt(&frg_unit->sip_opt[i]);
		}
	}
	if(NULL!=frg_unit->sip_diadata_ID)
	{
		free_opt(&frg_unit->sip_diadata_ID);				
	}
	if(NULL!=frg_unit->sip_data_dir)
	{
		free_opt(&frg_unit->sip_data_dir);				
	}
	if(NULL!=frg_unit->sip_rate_info)
	{
		free_opt(&frg_unit->sip_rate_info);				
	}	
}

void proc_sip_opt(frag_unit_t* frg_unit, media_t* mdi)
{	
	for(int i=0;i<SIP_OPT_NUM;i++)
	{
		if(i==SIP_RTP_4TUPLE_OPT_INDEX)
		{	
			if(NULL==mdi->sip_opt[i] && NULL!=frg_unit->sip_data_dir)
			{
				mdi->sip_opt[i] = frg_unit->sip_data_dir;
				mdi->sip_opt[i]->opt_type = g_sip_opt_type[i].opt_type;
				frg_unit->sip_data_dir = NULL;
			}
		}
		else if(NULL==mdi->sip_opt[i] && NULL!=frg_unit->sip_opt[i])
		{
			mdi->sip_opt[i] = frg_unit->sip_opt[i];
			frg_unit->sip_opt[i] = NULL;
		}
	}
	if(NULL==mdi->sip_rate_info)
	{
		mdi->sip_rate_info = frg_unit->sip_rate_info;
		frg_unit->sip_rate_info = NULL;
	}
	if(NULL==mdi->sip_diadata_ID)
	{
		mdi->sip_diadata_ID = frg_unit->sip_diadata_ID;
		frg_unit->sip_diadata_ID = NULL;
	}	
	mdi->re_offset = frg_unit->re_offset;
	mdi->data_flag = frg_unit->data_flag;
}

/*
IN
str_4tuple 	待解析四元组，str型
IN/OUT
s_ip s_port c_ip c_port		预先分配足够空间
*/
int parse_sip_4tuple(char* str_4tuple,char* s_ip,char* s_port,char* c_ip,char* c_port)
{
	char* offset1 = 0;
	char* offset2 = 0;
	char* offset3 = 0;
	
	offset1 = (char*)memmem(str_4tuple,strlen(str_4tuple),"_",strlen("_"));
	if(offset1==NULL)
		return -1;

	int s_ip_len = offset1-str_4tuple;
	memcpy(s_ip,str_4tuple,s_ip_len);
	memcpy(s_ip + s_ip_len,"\0",1);

	offset2 = (char*)memmem(offset1+1,strlen(str_4tuple)-(offset1-str_4tuple)-1,">",strlen(">"));
	if(offset2==NULL)
		return -1;

	int s_port_len = offset2-offset1-1;
	memcpy(s_port,offset1+1,s_port_len);
	memcpy(s_port + s_port_len,"\0",1);

	offset3 = (char*)memmem(offset2+1,strlen(str_4tuple)-(offset2-str_4tuple)-1,"_",strlen("_"));
	if(offset3==NULL)
		return -1;

	int c_ip_len = offset3-offset2-1;
	memcpy(c_ip,offset2+1,c_ip_len);
	memcpy(c_ip + c_ip_len,"\0",1);
	
	int c_port_len = strlen(str_4tuple)-(offset3-str_4tuple)-1;
	memcpy(c_port,offset3+1,c_port_len);
	memcpy(c_port + c_port_len,"\0",1);

	return 0;
}

int get_log_opt_unit(char *value, int value_len, char *data, int data_len, unsigned char opt_type, unsigned short *opt_num)
{
	struct opt_unit_t *log_opt_unit = NULL;	
	assert(value_len<data_len);	
	*opt_num += 1;
	log_opt_unit = (struct opt_unit_t *)data;
	log_opt_unit->opt_len = value_len+5;
	log_opt_unit->opt_type = opt_type;
	memcpy(data+5, value, log_opt_unit->opt_len-5);
	return log_opt_unit->opt_len;
}

/*前端回传的格式转换 */
int get_log_opt_addr(char *value, int value_len, char *data, int data_len, unsigned char opt_type, unsigned short *opt_num)
{
	char 		tuple4[128] = {0};
	char      	src_ip[32] = {0};
	char      	src_port[8] = {0};
	char      	dst_ip[32] = {0};
	char      	dst_port[8] = {0};
	struct opt_unit_t *log_opt_unit = NULL;	
	layer_addr_v4_t *ipv4 = NULL;
	
	memcpy(tuple4, value, MIN(value_len, 127));
	if(0==parse_sip_4tuple(tuple4,src_ip,src_port,dst_ip,dst_port))
	{
		*opt_num += 1;
		log_opt_unit = (struct opt_unit_t *)data;
		log_opt_unit->opt_len = sizeof(unsigned int) + sizeof(unsigned char) + sizeof(layer_addr_v4_t);
		log_opt_unit->opt_type = opt_type;
		ipv4 = (layer_addr_v4_t*)(data+sizeof(unsigned int) + sizeof(unsigned char));
		ipv4->layer_cnt = 0;
		inet_pton(AF_INET, (const char*)src_ip, (void *)&ipv4->client_addr);
		inet_pton(AF_INET, (const char*)dst_ip, (void *)&ipv4->server_addr);
		ipv4->client_port = htons(atoi(src_port));
		ipv4->server_port = htons(atoi(dst_port));
		return log_opt_unit->opt_len;	
	}
	return  0;
}

int get_survey_log_msg_header(sip_log_msg_header_t *msg_header)
{
	msg_header->magic_num = SIP_SURVEYLOG_MAGIC_NUM;
	msg_header->cont_code = SIP_LOG_CONT_CODE_NOTHING;
	msg_header->version = SIP_LOG_VERSION;
	msg_header->msg_type = SIP_SURVEYLOG_MSG_TYPE;
	return 0;
}

int get_survey_log_msg_body(media_t* mdi, sip_surveylog_msg_body_t *msg_body, resp_checkresult_t* check_res)
{
	msg_body->prog_id = *(unsigned long long*)check_res->prog_id;
	msg_body->cfg_id = check_res->cfg_id;
	msg_body->found_time = time(NULL);
	msg_body->cap_ip = g_frag_cfg.local_ip_nr;
	msg_body->protocol = mdi->proto;
	msg_body->service = check_res->service;
	msg_body->level = check_res->level;
	msg_body->fd_type = FD_TYPE_ANALYSE;
	msg_body->opt_num = 0;
	return 0;
}

int sip_send_survey_log(media_t* mdi, char* survey, uint32_t survey_len)
{		
	resp_checkresult_t* check_res	= (resp_checkresult_t*)(survey + MSG_HEADER_LEN);			
	struct sockaddr_in dest_addr;	
	int send_ret = 0;
	char data[8*1024] = {0};	
	int data_len = sizeof(data);
	int used_len = 0;
	sip_surveylog_msg_body_t*log_msg_body = NULL;
	sip_log_msg_header_t*	log_msg_header = NULL;	
	char*	locate_url = NULL;
	uint32_t locate_urllen = survey_len-sizeof(msg_header_t)-sizeof(resp_checkresult_t);	
	char*	locate_url_pos = NULL;	
	char opt_buf[32] = {0};
	char locate_urlbuf[1024] = {0};

	if(g_voip_surveylog_sock_fd == -1)
	{
		g_voip_surveylog_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (-1 == g_voip_surveylog_sock_fd)
		{
			MESA_handle_runtime_log(g_frag_run.voip_logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
									(char*)"[%s:%d] sip_send_survey_log get socket fail." , __FILE__,__LINE__);				
			return -1;
		}
	}	
	log_msg_header = (sip_log_msg_header_t*)data;	
	get_survey_log_msg_header(log_msg_header);
	used_len += sizeof(sip_log_msg_header_t);

	log_msg_body = (sip_surveylog_msg_body_t *)(data+used_len);
	used_len += sizeof(sip_surveylog_msg_body_t);
	get_survey_log_msg_body(mdi, log_msg_body, check_res);

	/*opt  : OPT_LAYER_ADDR_V4 SIP_SIP_4TUPL4_OPT_INDEX TODO */	
	if(NULL!=mdi->sip_opt[SIP_SIP_4TUPLE_OPT_INDEX])
	{
		used_len += get_log_opt_addr(mdi->sip_opt[SIP_SIP_4TUPLE_OPT_INDEX]->opt_value, 
									 mdi->sip_opt[SIP_SIP_4TUPLE_OPT_INDEX]->opt_len,
									 data+used_len, 
									 data_len-used_len, 
									 OPT_LAYER_ADDR_V4,
									 &log_msg_body->opt_num);
	}
	
	/*opt : OPT_VOIP_RECV_TIME*/
	used_len += get_log_opt_unit((char*)&mdi->create_time, 
								 sizeof(mdi->create_time), 
								 data+used_len, 
								 data_len-used_len, 
								 OPT_VOIP_RECV_TIME,
								 &log_msg_body->opt_num);
	
	/*opt : OPT_VOIP_DUATION*/
	memset(opt_buf, 0, sizeof(opt_buf));
	snprintf(opt_buf, sizeof(opt_buf), "%" PRIu64 "", mdi->lastpkt_time-mdi->create_time);
	used_len += get_log_opt_unit(opt_buf, 
								  strlen(opt_buf), 
								  data+used_len, 
								  data_len-used_len, 
								  OPT_VOIP_DUATION,
								  &log_msg_body->opt_num);
		
	
	/*VOIP协议类型 SIP OPT_VOIP_PROTOCOL*/
	used_len += get_log_opt_unit((char*)SIP_PROTO_OPT, 
								 strlen(SIP_PROTO_OPT), 
								 data+used_len, 
								 data_len-used_len, 
								 OPT_VOIP_PROTOCOL,
								 &log_msg_body->opt_num);
	
	/*opt  : OPT_VOIP_CALLING_ACCOUNT*/
	if(NULL!=mdi->sip_opt[SIP_FROM_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_FROM_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_FROM_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  OPT_VOIP_CALLING_ACCOUNT,
										  &log_msg_body->opt_num);		
	}	
	/*opt  : OPT_VOIP_CALLED_ACCOUNT*/
	if(NULL!=mdi->sip_opt[SIP_TO_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_TO_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_TO_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  OPT_VOIP_CALLED_ACCOUNT,
										  &log_msg_body->opt_num);
	}
	if(locate_urllen>0)
	{
		locate_url = survey+sizeof(msg_header_t)+sizeof(resp_checkresult_t);
		memcpy(locate_urlbuf, locate_url, locate_urllen);
		locate_url_pos = (char*)memchr(locate_url, ':', locate_urllen);
		if(NULL!=locate_url_pos)
		{
			
			if(mdi->re_offset==2)
			{
				/*opt: OPT_VOIP _TO_FROM _STORE_IP*/
				used_len += get_log_opt_unit(locate_url, 
										 	 locate_url_pos-locate_url, 
										  	 data+used_len, 
										  	 data_len-used_len, 
										  	 OPT_VOIP_TO_FROM_STORE_IP,
										  	 &log_msg_body->opt_num);
				/*opt: OPT_VOIP_TO_FROM_STORE_URL*/
				used_len += get_log_opt_unit(locate_url_pos+1, 
										 	 locate_urllen-(locate_url_pos-locate_url+1), 
										  	 data+used_len, 
										  	 data_len-used_len, 
										  	 OPT_VOIP_TO_FROM_STORE_URL,
										  	 &log_msg_body->opt_num);
			}
			else
			{	
				/*opt: OPT_VOIP_FROM_TO_STORE_IP*/
				used_len += get_log_opt_unit(locate_url, 
										 	 locate_url_pos-locate_url, 
										  	 data+used_len, 
										  	 data_len-used_len, 
										  	 OPT_VOIP_FROM_TO_STORE_IP,
										  	 &log_msg_body->opt_num);
				/*opt: OPT_VOIP_FROM_TO_STORE_URL*/
				used_len += get_log_opt_unit(locate_url_pos+1, 
										 	 locate_urllen-(locate_url_pos-locate_url+1), 
										  	 data+used_len, 
										  	 data_len-used_len, 
										  	 OPT_VOIP_FROM_TO_STORE_URL,
										  	 &log_msg_body->opt_num);
			}
		}
	}
	
	/*opt  : OPT_VOIP_RELATION_RTP_LAYER_ADDR_V4 SIP_RTP_4TUPL4_OPT_INDEX TODO */	
	if(NULL!=mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX])
	{
		used_len += get_log_opt_addr(mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX]->opt_value, 
									 mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX]->opt_len,
									 data+used_len, 
									 data_len-used_len, 
									 OPT_VOIP_RELATION_RTP_LAYER_ADDR_V4,
									 &log_msg_body->opt_num);
	}
	
	log_msg_header->cont_len = used_len - sizeof(sip_log_msg_header_t);
	assert(used_len<(int)sizeof(data));				

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = g_frag_cfg.voip_survey_log_iplist[mdi->mid%g_frag_cfg.voip_survey_log_ipnum];
	dest_addr.sin_port = htons(g_frag_cfg.voip_survey_log_port);

	send_ret = sendto(g_voip_surveylog_sock_fd, data, used_len, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
	if(send_ret < 0)
	{
		MESA_handle_runtime_log(g_frag_run.voip_logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
										(char*)"[%s:%d] sip_send_survey_log fail." , __FILE__,__LINE__);
		g_frag_stat.stat_info[RESP_CHECKRESULT_VOIP_SURVEYLOG][FAIL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_VOIP_SURVEYLOG][FAIL_BYTES] += used_len;
		return -1;
	}
	else
	{
		g_frag_stat.stat_info[RESP_CHECKRESULT_VOIP_SURVEYLOG][TOTAL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_VOIP_SURVEYLOG][TOTAL_BYTES] += used_len;
	}
	MESA_handle_runtime_log(g_frag_run.voip_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} %20s : [MID:%" PRIu64 ", cfg_id:%u, service:%d, level:%d, opt_num:%d, url:%s]", 
					  		__FILE__,__LINE__, "sip_send_survey_log", mdi->mid, check_res->cfg_id, check_res->service, check_res->level, log_msg_body->opt_num, locate_urlbuf);
	return send_ret;
}

int get_full_log_msg_header(sip_log_msg_header_t *msg_header)
{
	msg_header->magic_num = SIP_FULLLOG_MAGIC_NUM;
	msg_header->cont_code = SIP_LOG_CONT_CODE_NOTHING;
	msg_header->version = SIP_LOG_VERSION;
	msg_header->msg_type = SIP_FULLLOG_MSG_TYPE;
	return 0;
}

int get_full_log_msg_body(media_t* mdi, sip_fulllog_msg_body_t *msg_body)
{
	char      	sip_4tuple[128] = {0};
	char      	src_ip[32] = {0};
	char      	src_port[8] = {0};
	char      	dst_ip[32] = {0};
	char      	dst_port[8] = {0};
	char      	rtp_4tuple[128] = {0};	
	if(NULL!=mdi->sip_opt[SIP_CALL_ID_OPT_INDEX])
	{
		memcpy(msg_body->call_id, mdi->sip_opt[SIP_CALL_ID_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_CALL_ID_OPT_INDEX]->opt_len, 127));
	}	
	if(NULL!=mdi->sip_opt[SIP_FROM_TAGS_OPT_INDEX])
	{
		memcpy(msg_body->from_tags, mdi->sip_opt[SIP_FROM_TAGS_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_FROM_TAGS_OPT_INDEX]->opt_len, 63));
	}
	if(NULL!=mdi->sip_opt[SIP_TO_TAGS_OPT_INDEX])
	{
		memcpy(msg_body->to_tags, mdi->sip_opt[SIP_TO_TAGS_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_TO_TAGS_OPT_INDEX]->opt_len, 63));
	}
	if(mdi->re_offset)
	{
		 memcpy(msg_body->version, SIP_PROTO_SIP, strlen(SIP_PROTO_SIP));
	}
	else
	{
		 memcpy(msg_body->version, SIP_PROTO_RTP, strlen(SIP_PROTO_RTP));		
	}	
	memcpy(msg_body->method, SIP_METHOD_OPT, strlen(SIP_METHOD_OPT));
	if(NULL!=mdi->sip_opt[SIP_RESCODE_OPT_INDEX])
	{
		memcpy(msg_body->status, mdi->sip_opt[SIP_RESCODE_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_RESCODE_OPT_INDEX]->opt_len, 7));
	}	
	if(NULL!=mdi->sip_opt[SIP_REASON_OPT_INDEX])
	{
		memcpy(msg_body->reason, mdi->sip_opt[SIP_REASON_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_REASON_OPT_INDEX]->opt_len, 127));
	}
	if(NULL!=mdi->sip_opt[SIP_CSEQ_OPT_INDEX])
	{
		memcpy(msg_body->cseq, mdi->sip_opt[SIP_CSEQ_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_CSEQ_OPT_INDEX]->opt_len, 127));
	}
	/*SIP_4tuple*/
	if(NULL!=mdi->sip_opt[SIP_SIP_4TUPLE_OPT_INDEX])
	{		
		memcpy(sip_4tuple, mdi->sip_opt[SIP_SIP_4TUPLE_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_SIP_4TUPLE_OPT_INDEX]->opt_len, 127));
		if(0==parse_sip_4tuple(sip_4tuple,src_ip,src_port,dst_ip,dst_port))
		{
			snprintf(msg_body->src_ip, sizeof(msg_body->src_ip), "%s", src_ip);
			snprintf(msg_body->dst_ip, sizeof(msg_body->dst_ip), "%s", dst_ip);
			msg_body->src_port = atoi(src_port);
			msg_body->dst_port = atoi(dst_port);
		}
	}	
	if(NULL!=mdi->sip_opt[SIP_USERAGENT_OPT_INDEX])
	{
		memcpy(msg_body->useragent, mdi->sip_opt[SIP_USERAGENT_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_USERAGENT_OPT_INDEX]->opt_len, 127));
	}
	/*RTP_4tuple*/
	if(NULL!=mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX])
	{		
		memcpy(rtp_4tuple, mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX]->opt_value, MIN(mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX]->opt_len, 127));
		memset(src_ip, 0, sizeof(src_ip));
		memset(src_port, 0, sizeof(src_port));
		memset(dst_ip, 0, sizeof(dst_ip));
		memset(dst_port, 0, sizeof(dst_port));
		if(0==parse_sip_4tuple(rtp_4tuple,src_ip,src_port,dst_ip,dst_port))
		{
			snprintf(msg_body->from_rtp_ip, sizeof(msg_body->src_ip), "%s", src_ip);
			snprintf(msg_body->to_rtp_ip, sizeof(msg_body->dst_ip), "%s", dst_ip);
			msg_body->from_rtp_port = atoi(src_port);
			msg_body->to_rtp_port = atoi(dst_port);
		}
	}
	/*cpz_ip*/
	inet_ntop(AF_INET, &g_frag_cfg.local_ip_nr, msg_body->cly_ip, sizeof(msg_body->cly_ip)); 
	msg_body->found_time = time(NULL);
	return 0;
}

//int VOIP_udp_send_full_log(const AV_log_t* log_msg, struct opt_unit_t* opt, int opt_num, int acc_cnt)
int sip_send_full_log(media_t* mdi)
{	
	struct sockaddr_in dest_addr;	
	int send_ret = 0;
	char data[8*1024] = {0};	
	int data_len = sizeof(data);
	int used_len = 0;	
	char opt_buf[32] = {0};
	sip_fulllog_msg_body_t*	log_msg_body = NULL;
	sip_log_msg_header_t*	log_msg_header = NULL;	

	if(g_voip_fulllog_sock_fd == -1)
	{
		g_voip_fulllog_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (-1 == g_voip_fulllog_sock_fd)
		{
			MESA_handle_runtime_log(g_frag_run.voip_logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
									(char*)"[%s:%d] sip_send_full_log get socket fail." , __FILE__,__LINE__);				
			return -1;
		}
	}
	
	log_msg_header = (sip_log_msg_header_t*)data;	
	get_full_log_msg_header(log_msg_header);
	used_len += sizeof(sip_log_msg_header_t);

	log_msg_body = (sip_fulllog_msg_body_t *)(data+used_len);
	used_len += sizeof(sip_fulllog_msg_body_t);
	get_full_log_msg_body(mdi, log_msg_body);

	/*opt  : OPT_PID*/
	snprintf(opt_buf, sizeof(opt_buf), "%" PRIu64 "", mdi->mid);
	used_len += get_log_opt_unit(opt_buf, 
								  strlen(opt_buf), 
								  data+used_len, 
								  data_len-used_len, 
								  OPT_VOIP_PID,
								  &log_msg_body->opt_num);
	
	/*opt  : OPT_REQUEST_URL*/
	if(NULL!=mdi->sip_opt[SIP_URI_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_URI_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_URI_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_URI_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}	
	/*opt  : OPT_CONTACTS*/
	if(NULL!=mdi->sip_opt[SIP_C_CONTACT_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_C_CONTACT_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_C_CONTACT_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_C_CONTACT_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}
	else if(NULL!=mdi->sip_opt[SIP_S_CONTACT_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_S_CONTACT_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_S_CONTACT_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_S_CONTACT_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}
	/*opt  : OPT_VIA*/
	if(NULL!=mdi->sip_opt[SIP_C_VIA_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_C_VIA_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_C_VIA_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_C_VIA_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}
	else if(NULL!=mdi->sip_opt[SIP_S_VIA_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_S_VIA_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_S_VIA_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_S_VIA_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}
	/*opt  : OPT_FROM_RTP_MAP*/
	if(NULL!=mdi->sip_opt[SIP_C_CODING_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_C_CODING_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_C_CODING_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_C_CODING_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}
	/*opt  : OPT_TO_RTP_MAP*/
	if(NULL!=mdi->sip_opt[SIP_S_CODING_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_S_CODING_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_S_CODING_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_S_CODING_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}		
	/*opt  : OPT_FROM_ACCOUNT*/
	if(NULL!=mdi->sip_opt[SIP_FROM_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_FROM_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_FROM_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_FROM_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}	
	/*opt  : OPT_TO_ACCOUNT*/
	if(NULL!=mdi->sip_opt[SIP_TO_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_TO_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_TO_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_TO_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}	
	/*opt  : OPT_RECORD_ROUTES*/
	if(NULL!=mdi->sip_opt[SIP_C_RECORD_ROUTES_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_C_RECORD_ROUTES_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_C_RECORD_ROUTES_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_C_RECORD_ROUTES_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}
	else if(NULL!=mdi->sip_opt[SIP_S_RECORD_ROUTES_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_S_RECORD_ROUTES_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_S_RECORD_ROUTES_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_S_RECORD_ROUTES_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}	
	/*opt  : OPT_ROUTE*/
	if(NULL!=mdi->sip_opt[SIP_C_ROUTE_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_C_ROUTE_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_C_ROUTE_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_C_ROUTE_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}
	else if(NULL!=mdi->sip_opt[SIP_S_ROUTE_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_S_ROUTE_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_S_ROUTE_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_S_ROUTE_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);
	}	

	/*opt : OPT_VOIP_DUATION*/
	/*
	if(NULL!=mdi->sip_opt[SIP_DURATION_OPT_INDEX])
	{
		used_len += get_log_opt_unit(mdi->sip_opt[SIP_DURATION_OPT_INDEX]->opt_value, 
										  mdi->sip_opt[SIP_DURATION_OPT_INDEX]->opt_len, 
										  data+used_len, 
										  data_len-used_len, 
										  mdi->sip_opt[SIP_DURATION_OPT_INDEX]->opt_type,
										  &log_msg_body->opt_num);		
	}*/	
	memset(opt_buf, 0, sizeof(opt_buf));
	snprintf(opt_buf, sizeof(opt_buf), "%" PRIu64 "", mdi->lastpkt_time-mdi->create_time);
	used_len += get_log_opt_unit(opt_buf, 
								  strlen(opt_buf), 
								  data+used_len, 
								  data_len-used_len, 
								  OPT_VOIP_DUATION_FULLLOG,
								  &log_msg_body->opt_num);
	/*opt : OPT_VOIP_VOICE_DIR*/
	memset(opt_buf, 0, sizeof(opt_buf));
	snprintf(opt_buf, sizeof(opt_buf), "%d", mdi->re_offset);
	used_len += get_log_opt_unit(opt_buf, 
								  strlen(opt_buf), 
								  data+used_len, 
								  data_len-used_len, 
								  OPT_VOIP_VOICE_DIR_FULLLOG,
								  &log_msg_body->opt_num);
	
	log_msg_header->cont_len = used_len - sizeof(sip_log_msg_header_t);
	assert(used_len<(int)sizeof(data));				

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = g_frag_cfg.voip_full_log_iplist[mdi->mid%g_frag_cfg.voip_full_log_ipnum];
	dest_addr.sin_port = htons(g_frag_cfg.voip_full_log_port);

	send_ret = sendto(g_voip_fulllog_sock_fd, data, used_len, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
	if(send_ret < 0)
	{
		MESA_handle_runtime_log(g_frag_run.voip_logger,RLOG_LV_FATAL,FRAG_REASSEMBLY_MODULE_NAME,
										(char*)"[%s:%d] sip_send_full_log fail." , __FILE__,__LINE__);
		g_frag_stat.stat_info[RESP_CHECKRESULT_VOIP_FULLLOG][FAIL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_VOIP_FULLLOG][FAIL_BYTES] += used_len;
		return -1;
	}
	else
	{	
		g_frag_stat.stat_info[RESP_CHECKRESULT_VOIP_FULLLOG][TOTAL_PKTS]++; 
		g_frag_stat.stat_info[RESP_CHECKRESULT_VOIP_FULLLOG][TOTAL_BYTES] += used_len;	
	}
	MESA_handle_runtime_log(g_frag_run.voip_logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME, 
							"{%s:%d} %20s : [MID:%" PRIu64 ",opt_num:%d]", 
					  		__FILE__,__LINE__, "sip_send_full_log", mdi->mid,log_msg_body->opt_num);
	return send_ret;
}

void set_frag_unit_from_media(media_t* mdi, frag_unit_t* frg_unit)
{	
	frg_unit->mid = mdi->mid;
	frg_unit->pid = mdi->mid;
	frg_unit->thread_seq = mdi->thread_seq;
	frg_unit->sip_diadata_ID = mdi->sip_diadata_ID;
	frg_unit->sip_data_dir = mdi->sip_opt[SIP_RTP_4TUPLE_OPT_INDEX];	
}

void send_sip_log_when_expire(media_t* mdi)
{		
#if K_PROJECT
#else
	/*全量日志*/
	sip_send_full_log(mdi); 
	/*FD日志*/
	if(mdi->sip_survey_type&SIP_SURVEY_TYPE_FD)
	{
		sip_send_survey_log(mdi, mdi->fd_buf, mdi->fd_buflen);
	}				
	/*JC日志*/
	if(mdi->sip_survey_type&SIP_SURVEY_TYPE_JC)
	{
		sip_send_survey_log(mdi, mdi->jc_buf, mdi->jc_buflen);
	}
#endif
}

/*
*return query succ or fail : -1:fail 0:succ ,but no result  1:succ have result
*/
int redis_sip_index_3_query(char* index_key, frag_unit_t*  frg_unit, int thread_seq)
{
	int 			rec = 0;	
	char 			command[REDIS_CMMD_MAXLEN] = {0};	
	char*			p_cmmd = command;	
	int 			cmmd_len = 0;
	size_t			cmmd_argvlen[SIP_REDIS_CMMD_NUM] = {0};
	redisReply*		reply = NULL;

	g_sip_cmmd_argv[1] = index_key;
	for(int i=0;i<SIP_REDIS_CMMD_NUM;i++)
	{
		cmmd_argvlen[i] = strlen(g_sip_cmmd_argv[i]);
	}	

	/*just for record log*/
	for(int i=0;i<SIP_REDIS_CMMD_NUM;i++)
	{		
		snprintf(p_cmmd, REDIS_CMMD_MAXLEN-cmmd_len, "%s ", g_sip_cmmd_argv[i]);		
		cmmd_len = strlen(g_sip_cmmd_argv[i])+1;
		p_cmmd += cmmd_len;
	}
	MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,
                                                                "{%s:%d} [before  %d redis cluster exec command %" PRIu64 "='%s' succ].",
                                                                __FILE__,__LINE__, thread_seq, frg_unit->pid,command);	
	rec = redis_excute_command(thread_seq, frag_rssb.logger, &reply, command ,SIP_REDIS_CMMD_NUM, g_sip_cmmd_argv, cmmd_argvlen);
	MESA_handle_runtime_log(frag_rssb.logger, RLOG_LV_INFO, FRAG_REASSEMBLY_MODULE_NAME,
                                                                "{%s:%d} [after %d redis cluster exec command %" PRIu64 "='%s' succ].",
                                                                __FILE__,__LINE__, thread_seq, frg_unit->pid,command);
	if(0==rec)
	{
		/*索引信息hash 存储顺序*/
		for(size_t i=0;i<reply->elements;i++)
		{				
			if(reply->element[i]->type==REDIS_REPLY_STRING)
			{
				/*0: data_flag*/
				if(i==0)
				{					
					frg_unit->data_flag = (uint8_t)atoi(reply->element[i]->str);
				}
				/*1: RTP_4tuple*/
				else if(i==1)
				{
					if(0==strncmp(frg_unit->sip_data_dir->opt_value, reply->element[i]->str, frg_unit->sip_data_dir->opt_len))
					{
						frg_unit->re_offset = 1;
					}
					else
					{
						frg_unit->re_offset = 2;
					}
					rec = 1;
				}				
				else
				{	
					frg_unit->sip_opt[i] = (opt_in_t*)malloc(sizeof(opt_in_t));
					frg_unit->sip_opt[i]->opt_len = reply->element[i]->len;  
					frg_unit->sip_opt[i]->opt_value = (char*)malloc(reply->element[i]->len);
					frg_unit->sip_opt[i]->opt_type = g_sip_opt_type[i].opt_type;
					memcpy(frg_unit->sip_opt[i]->opt_value, reply->element[i]->str, reply->element[i]->len);			
				}
			}										
		}		
		freeReplyObject(reply);		
	}	
	/*索引信息不需要删除*/	
	return rec;		
}

int sip_parse_querykey(char* key,char* key2,char* key3)
{
	if(key == NULL)
	{
		return -1;
	}	
	sscanf(key,"%[0-9,.,:]_%[0-9,.,:]",key2,key3);
	if((key2 == NULL)||(key3 == NULL))
	{
		return -1;
	}
	return 0;
}

/*
*return query succ or fail : -1:fail 0:succ ,but no result  1:succ have result
*/
int redis_sip_index_query(frag_unit_t*  frg_unit, int thread_seq)
{
	if(frg_unit->sip_diadata_ID==NULL || frg_unit->sip_diadata_ID->opt_len==0) return 0;
	int query_succ = 0;
	int key_type = 0;
	char index_key[3][1024] = {{0}};

	memcpy(index_key, frg_unit->sip_diadata_ID->opt_value, frg_unit->sip_diadata_ID->opt_len);
	if(-1==sip_parse_querykey(index_key[0],index_key[1],index_key[2]))
	{
		return 0;
	}

	for(int i=0;i<3;i++)
	{	
		query_succ = redis_sip_index_3_query(index_key[i], frg_unit,thread_seq);
		if(query_succ==1)
		{
			key_type = i+1;
			break;
		}
	}
	return key_type;		
}

int sip_index_query(frag_unit_t*  frg_unit, int thread_seq)
{	
	int key_type = 0;
	int query_succ = 0;
	/*need index query*/	
	frg_unit->frag_state = STAT_INDEX_QUERY;
	key_type = redis_sip_index_query(frg_unit, thread_seq);	
	if(key_type)
	{		
		query_succ = 1;
		switch(key_type)
		{
			case SIP_KEY_1:				
				atomic_inc(&frag_rssb.stat_info[RSSB_SIP_ACK_KEY1]);			
				break;

			case SIP_KEY_2:				
				atomic_inc(&frag_rssb.stat_info[RSSB_SIP_ACK_KEY2]);			
				break;

			case SIP_KEY_3:
				atomic_inc(&frag_rssb.stat_info[RSSB_SIP_ACK_KEY3]);			
				break;
		}		
		frg_unit->frag_state = STAT_OK;
	}
	return query_succ;	
}

int proc_sip_mediainfo_opt(frag_unit_t* frg_unit, rssb_media_info_t* media_info)
{
	char* 		ptr = media_info->opt;
	uint32_t	opt_len = 0;
	uint32_t	opt_value_len = 0;
	uint8_t		opt_type = 0;
	char*		opt_value = NULL;
	
	/*proc not extrace opt*/
	for(int i=0;i<media_info->opt_num;i++)
	{
		opt_len = *(uint32_t*)ptr;
		opt_type = *(uint8_t*)(ptr+sizeof(uint32_t));
		opt_value = ptr+sizeof(uint32_t)+sizeof(uint8_t);		
		opt_value_len = opt_len-sizeof(uint32_t)-sizeof(uint8_t);	
		if(opt_len>100000 || opt_value_len>100000) return -1;
		if(META_OPT_SIP_DIADATA_ID==opt_type)
		{
			if(NULL!=frg_unit->sip_diadata_ID)
			{
				free_opt(&frg_unit->sip_diadata_ID);				
			}			
			frg_unit->sip_diadata_ID = (opt_in_t*)malloc(sizeof(opt_in_t));			
			frg_unit->sip_diadata_ID->opt_len = opt_value_len;  
			frg_unit->sip_diadata_ID->opt_type = opt_type;
			frg_unit->sip_diadata_ID->opt_value = (char*)malloc(opt_value_len);
			memcpy(frg_unit->sip_diadata_ID->opt_value, opt_value,opt_value_len);				
		}		
		else if(META_OPT_SIP_DATA_DIR==opt_type)
		{			
			if(NULL!=frg_unit->sip_data_dir)
			{
				free_opt(&frg_unit->sip_data_dir);				
			}			
			frg_unit->sip_data_dir = (opt_in_t*)malloc(sizeof(opt_in_t));			
			frg_unit->sip_data_dir->opt_len = opt_value_len;  
			frg_unit->sip_data_dir->opt_type = opt_type;
			frg_unit->sip_data_dir->opt_value = (char*)malloc(opt_value_len);
			memcpy(frg_unit->sip_data_dir->opt_value, opt_value,opt_value_len);	
		}	
		else if(META_OPT_SIP_RATE_INFO==opt_type)
		{			
			if(NULL!=frg_unit->sip_rate_info)
			{
				free_opt(&frg_unit->sip_rate_info);				
			}			
			frg_unit->sip_rate_info = (opt_in_t*)malloc(sizeof(opt_in_t));			
			frg_unit->sip_rate_info->opt_len = opt_value_len;  
			frg_unit->sip_rate_info->opt_type = opt_type;
			frg_unit->sip_rate_info->opt_value = (char*)malloc(opt_value_len);
			memcpy(frg_unit->sip_rate_info->opt_value, opt_value,opt_value_len);	
		}			
		ptr += sizeof(uint32_t)+sizeof(uint8_t)+opt_value_len;
	}
	
	return 0;
}

void set_sip_frag_unit(rssb_media_info_t* media_info, frag_unit_t* frg_unit)
{
	frg_unit->pid = media_info->pid;	
	frg_unit->mid = media_info->pid;	
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
	proc_sip_mediainfo_opt(frg_unit, media_info);	
}
