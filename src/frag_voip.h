#ifndef _VOIP_REASSEMBLY_H
#define _VOIP_REASSEMBLY_H

#include "frag_reassembly_in.h"

/*index in g_sip_opt_type*/
/*typedef enum
{
	SIP_HMGET_INDEX,//NOT SAVE OPT
	SIP_KEY_INDEX,//NOT SAVE OPT
	SIP_DATA_FLAG_INDEX,//NOT SAVE OPT
	SIP_RTP_4TUPLE_OPT_INDEX,//NOT SAVE OPT
	SIP_URI_OPT_INDEX,
	
	SIP_FROM_OPT_INDEX,
	SIP_TO_OPT_INDEX,
	SIP_SGATEWAY_OPT_INDEX,//NOT NEED SENDBACK
	SIP_CGATEWAY_OPT_INDEX,//NOT NEED SENDBACK
	SIP_DURATION_OPT_INDEX,

	SIP_S_CODING_OPT_INDEX,
	SIP_C_CODING_OPT_INDEX,
	SIP_FROM_TAGS_OPT_INDEX,
	SIP_TO_TAGS_OPT_INDEX,
	SIP_CALL_ID_OPT_INDEX,
	
	SIP_CSEQ_OPT_INDEX,
	SIP_C_CONTACT_OPT_INDEX,
	SIP_S_CONTACT_OPT_INDEX,
	SIP_USERAGENT_OPT_INDEX,
	SIP_SERVER_OPT_INDEX,//NOT NEED SENDBACK
	
	SIP_C_CRYPTO_OPT_INDEX,//NOT NEED SENDBACK
	SIP_C_INLINE_OPT_INDEX,//NOT NEED SENDBACK
	SIP_S_CRYPTO_OPT_INDEX,//NOT NEED SENDBACK
	SIP_S_INLINE_OPT_INDEX,//NOT NEED SENDBACK
	SIP_RINGING_OPT_INDEX,//NOT NEED SENDBACK
	
	SIP_REASON_OPT_INDEX,
	SIP_SIP_4TUPLE_OPT_INDEX,
	SIP_S_VIA_OPT_INDEX,
	SIP_S_RECORD_ROUTES_OPT_INDEX,
	SIP_S_ROUTE_OPT_INDEX,
	
	SIP_C_VIA_OPT_INDEX,
	SIP_C_RECORD_ROUTES_OPT_INDEX,
	SIP_C_ROUTE_OPT_INDEX,
	SIP_RESCODE_OPT_INDEX,
	SIP_CAPIP_OPT_INDEX,

	SIP_OPT_NUM,
}sip_opt_index;

#define SIP_OPT_MAXNUM  					16
#define SIP_CALL_ID_OPT_INDEX  				11
#define SIP_FROM_TAGS_OPT_INDEX  			9
#define SIP_TO_TAGS_OPT_INDEX  				10
#define SIP_RESCODE_OPT_INDEX  				30
#define SIP_REASON_OPT_INDEX  				22
#define SIP_CSEQ_OPT_INDEX  				12
#define SIP_USERAGENT_OPT_INDEX  			15
#define SIP_URI_OPT_INDEX  					1
#define SIP_C_CONTACT_OPT_INDEX  			13
#define SIP_S_CONTACT_OPT_INDEX  			14
#define SIP_C_VIA_OPT_INDEX  				27
#define SIP_S_VIA_OPT_INDEX  				24
#define SIP_C_RECORD_ROUTES_OPT_INDEX  		28
#define SIP_S_RECORD_ROUTES_OPT_INDEX  		25
#define SIP_C_ROUTE_OPT_INDEX  				29
#define SIP_S_ROUTE_OPT_INDEX  				26
#define SIP_C_CODING_OPT_INDEX  			8
#define SIP_S_CODING_OPT_INDEX  			7
#define SIP_FROM_OPT_INDEX  				2
#define SIP_TO_OPT_INDEX  					3
#define SIP_RTP_4TUPLE_OPT_INDEX  			0
#define SIP_SIP_4TUPLE_OPT_INDEX  			23
#define SIP_DURATION_OPT_INDEX  			6
#define SIP_CAPIP_OPT_INDEX  				31
*/

#define SIP_PROTO_SIP 					"SIP-RTP"
#define SIP_PROTO_RTP 					"RTP"
#define SIP_METHOD_OPT 						"INVITE"
#define SIP_PROTO_OPT 						"SIP"

/*全量日志的最大选项个数*/
/*voip日志消息头sip_log_msg_header_t*/
#define SIP_LOG_CONT_CODE_NOTHING 			0
#define SIP_LOG_VERSION 					3
#define SIP_FULLLOG_MAGIC_NUM 				0x5632
#define SIP_FULLLOG_CONT_CODE_VERSION 		0X03
#define SIP_FULLLOG_MSG_TYPE 				0x21  
/*监测日志消息头sip_og_msg_header_t*/
#define SIP_SURVEYLOG_MAGIC_NUM 			0x5641
#define SIP_SURVEYLOG_MSG_TYPE 				0x21 

/*全量日志类型g_sip_opt_type*/
#define OPT_VOIP_PID 					0x29
#define OPT_VOIP_DUATION_FULLLOG			0x2A
#define OPT_VOIP_VOICE_DIR_FULLLOG			0x2B
#define OPT_VOIP_CAP_IP_FULLLOG				0x2C

/*监测日志选项类型*/
#define OPT_LAYER_ADDR_V4 				0x3B //0x3B:RTP, 0x2D:SIP, change
#define OPT_VOIP_RECV_TIME 				0x2F
#define OPT_VOIP_DUATION 				0x30
#define OPT_VOIP_PROTOCOL 				0x31
#define OPT_VOIP_CALLING_ACCOUNT			0x32
#define OPT_VOIP_CALLED_ACCOUNT				0x33
#define OPT_VOIP_RELATION_RTP_LAYER_ADDR_V4 		0x2D //0x3B:RTP, 0x2D:SIP, change
#define OPT_VOIP_FROM_TO_STORE_IP 			0x36
#define OPT_VOIP_FROM_TO_STORE_URL 			0x37
#define OPT_VOIP_TO_FROM_STORE_IP 			0x38
#define OPT_VOIP_TO_FROM_STORE_URL 			0x39
#define OPT_VOIP_VOICE_DIR 				0x3C
#define OPT_VOIP_CAP_IP			 		0x3D

/*sip_sendlog_flag*/
#define SIP_SEND_FULL_LOG 					0x01
#define SIP_SEND_SURVEY_LOG					0x02

/*sip查询使用key1 key2 key3*/
#define SIP_KEY_1 							1
#define SIP_KEY_2 							2
#define SIP_KEY_3 							3

/*RTP数据前8个字节，4个字节的时间戳，4个字节的rtp_seq*/
#define VOIP_DATA_SEQ_OFFSET				4
#define VOIP_DATA_TIME_SEQ_LEN				8


typedef struct sip_fulllog_msg_body_s 
{
	char      	call_id[128];  // not NULL
	char      	from_tags[64]; 
	char      	to_tags[64];  
	char      	version[16];
	char      	method[16];
	char      	status[8];     //?? rescode not exist
	char      	reason[128];  
	char      	cseq[128];
	char      	src_ip[20];
	uint32_t  	src_port;
	char      	dst_ip[20];
	uint32_t  	dst_port;
	char      	useragent[128];
	char      	from_rtp_ip[20];
	uint32_t  	from_rtp_port;   
	char      	to_rtp_ip[20];
	uint32_t  	to_rtp_port;
	uint32_t  	paw_num;		//出入口编号，暂时为0
	char      	cly_ip[20];		//处理机器IP，暂时填写为粗瓶装IP
	time_t    	found_time;			
	uint16_t 	opt_num; 		//选项信息单元个数
}__attribute__((packed))sip_fulllog_msg_body_t;

typedef struct sip_surveylog_msg_body_s 
{
	unsigned long long prog_id;				// 节目的ID，由数据获取节点生成,非字符串
	uint32_t   	cfg_id;						// 命中配置ID
	uint32_t 	found_time;					// 发现时间
	uint32_t   	cap_ip;						// 为捕包机的IP地址
	uint8_t    	protocol;					// 传输层协议
	uint8_t		service;					// 业务类型
	uint8_t		level;						// 检测结果置信度
	uint8_t  	fd_type;					// 封堵类型
	uint16_t 	opt_num; 					//选项信息单元个数
	//struct opt_unit_t opt_info[opt_num];	//选项信息,仅供示意，不包含在头部
}__attribute__((packed))sip_surveylog_msg_body_t;

typedef struct sip_log_msg_header_s
{
	uint16_t 	magic_num; 				// 魔数，survey log值必须为0x5641 ; fulllog值必须为0x5632
	uint8_t  	cont_code:4;  			// 消息体编码格式
	uint8_t  	version:4;  			// 协议版本，目前值是3
	uint8_t		msg_type; 				// 消息类型
	uint32_t 	cont_len; 				// 消息体部分的字节长度
}sip_log_msg_header_t;


#ifdef __cplusplus
extern "C" {
#endif

void free_media_sip(media_t* mdi);
void free_frag_unit_sip_opt(frag_unit_t* 	frg_unit);


int redis_sip_index_query(frag_unit_t*  frg_unit, int thread_seq);
int sip_index_query(frag_unit_t*  frg_unit, int thread_seq);


int sip_send_full_log(media_t* mdi);
int sip_send_survey_log(media_t* mdi, char* survey, uint32_t survey_len);
void send_sip_log_when_expire(media_t* mdi);

void proc_sip_opt(frag_unit_t* frg_unit, media_t* mdi);
int proc_sip_mediainfo_opt(frag_unit_t* frg_unit, rssb_media_info_t* media_info);	


void set_frag_unit_from_media(media_t* mdi, frag_unit_t* frg_unit);
void set_sip_frag_unit(rssb_media_info_t* media_info, frag_unit_t* frg_unit);
int parse_sip_4tuple(char* str_4tuple,char* s_ip,char* s_port,char* c_ip,char* c_port);
#ifdef __cplusplus
}
#endif

#endif

