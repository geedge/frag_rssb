#ifndef _AV_INTERFACE_H
#define _AV_INTERFACE_H

#define	K_PROJECT						0  //是否是K项目，主要是建议的接口不一致

#define PROTO_VERSION  					3
#define PROTO_MAGICNUM 					0x5641

/*META_OPT_* same with av_master 来自前端*/
#define META_OPT_LAYER_ADDR				0x52   //META_OPT_LAYER_ADDR
#define	META_OPT_SINGLE_KEY				0x68
#define	META_OPT_LAYER_URL				0x61
#define META_OPT_ETAG					0x62
#define META_OPT_LAST_MODIFY			0x63
#define	META_OPT_REFERER				0x64   //AV_IdentifyFormatDoc
#define	META_OPT_USER_AGENT				0x65 
#define	META_OPT_C2S_CONT_TYPE			0x66  //cache and send because of boundary
#define	META_OPT_S2C_CONT_TYPE			0x67  //cache and send because of boundary
#define META_OPT_SERVICE_ID				0x71  /*碎片化使用，自用*/
#define	META_OPT_C2S_CONT				0x72  /*碎片化使用，自用*/
#define	META_OPT_S2C_CONT				0x73  /*碎片化使用，自用*/
#define META_OPT_CONT_DISPOSITION 		0x74  /*碎片化使用，自用*/
#define META_OPT_SIP_DIADATA_ID			0x81  /*VOIP使用*/
#define META_OPT_SIP_DATA_DIR			0x82 
#define META_OPT_SIP_RATE_INFO 			0x83
#define META_OPT_OFFSET 				0x69  /*offset的选项,与前端一致*/
#define META_OPT_SERVER 				0x6A

/*发送给内容分析程序的选项*/
#define OPT_SOURCE_IP					0x41  /*localIP选项*/
#define META_OPT_SIP_SEND_RATE_INFO 	0x71  /*VOIP业务*/

/*多源情况下，粗瓶装发送给其他粗瓶装的选项*/
#define META_OPT_INFO_BEFORE_MULTISRC 	0x91  /*多源汇聚之前的节目信息，包括MID和capIP，内部自用*/

#define	FILE_OSMF						0xA1
#define	FILE_HLS						0xA2
#define	FILE_IOS						0xA3
#define	FILE_ANDRIOD					0xA4
#define	FILE_APP						0xA5
#define	FILE_FRAG						0xA6  /*碎片预告识别的,例如url里面带有偏移量的，例如iqiyi*/
#define	FILE_MAYBE_FRAG 				0xA7  /*前端回传的可能是碎片的音视频*/
#define	FILE_AV							0x00  /*碎片预告识别的媒体类型转换为0x00*/

#define	VOIP_UNKNOWN_MEDIA_TYPE			0xB0

/*from papp*/
#define MSG_DATA_META	 				0x31
#define MSG_DATA_BODY 					0x32
#define MSG_DATA_TAIL 					0x33

/*live*/
#define MSG_LIVE_CHECK 					0x41   //from sapp 
#define MSG_LIVE_RESPONS 				0x42   //to sapp

/*from av_analyse*/
#define MSG_RESP_CHECKRESULT 			0x11	// Check Result notification
#define MSG_RESP_REJECT					0x12	// Reject Message notification, not proc now
#define MSG_RESP_CHARACTER				0x13	// Character notification, 1. wins ack, 2. send to sapp
#define MSG_RESP_CHECKRESULT_ACK      	0x14	//not send now
#define MSG_PROG_SYNC					0x15	//

/*prog sync*/
#define SYNC_SENDER_VIDEO_ANALYZER		0x01
#define	SYNC_SENDER_AUDIO_ANALYZER		0x02
#define	SYNC_ACTION_STOP_TO_SUBSYSTEM	0x01
#define	SYNC_ACTION_STOP_TO_MAINSYSTEM	0x02
#define	SYNC_ACTION_ELIMINATE_PROGRAM	0x03

/*封堵建议碎片化管控特征选项*/
#define OPT_FRAG_ORIGIN_URL				0x11
#define OPT_FRAG_CTRL_SUBSTR			0x12
#define OPT_FRAG_INDEX_URL				0x13

/*FD建议: 图片建议set opt_num=0*/
#define	SURVEY_PIC_TYPE					0x31
#define	SURVEY_PIC_MONITOR_TYPE			0xB1
#define	SURVEY_PIC_KEYWORD_TYPE			0xB2

// FD类型, 0表示动态黑白名单，1表示静态配置命中，2表示二级分析节点命中。
#define	FD_TYPE_DYNAMIC_LIST			0
#define	FD_TYPE_STATIC_CONF				1
#define	FD_TYPE_ANALYSE					2


/*=================================monitor service===============================================*/

#define	SERVICE_AUDIO_LANG				0x97	//声学所产生的语种识别建议
#define	SERVICE_AUDIO_LANG_FULL			0x98	//粗拼装根据上述建议产生的全文件监测日志

/*=================================ack msg===============================================*/
typedef struct msg_data_ack_s 
{
	char						prog_id[8];
	uint16_t					pad;
	uint64_t					ack_offset:48; 	
}msg_data_ack_t; 

/*=================================data msg===============================================*/
#define MSG_HEADER_LEN	sizeof(msg_header_t)
typedef struct msg_header_s 
{
	uint16_t  					magic_num; 
	uint8_t 					version;	 	
	uint8_t						msg_type; 		// msg data type
	uint32_t 					cont_len; 	
}msg_header_t; /* sizeof = 8B */

#define MSG_MEDIAINFO_HEAD_LEN	 (sizeof(msg_metainfo_t))
/*Data_flag defination*/
typedef struct msg_metainfo_s
{
	char						prog_id[8];	
	char    					flag;	
#if K_PROJECT
    uint8_t   					pad[3];
    int		   					hitservice;
	uint64_t  					prog_len;
#else
	uint8_t   					hitservice;
	uint64_t  					prog_len:48;
#endif
	uint32_t					cap_IP;
	uint8_t						protocol;		
	uint8_t						media_type;	
	uint8_t						data_flag;	
	uint8_t						opt_num;      
}msg_metainfo_t;

typedef struct msg_data_s
{
	char						prog_id[8];	
	union
	{
		struct
		{
			uint8_t				pad;          
			uint8_t				flags;       
		};
		uint16_t 				frag_seq;
	};	
	uint64_t  					offset:48;	   
}msg_data_t;
#define MSG_DATA_HEAD_LEN	(sizeof(msg_data_t))

/*==============================response msg==================================================*/
typedef struct msg_prog_sync_s
{
	char						prog_id[8];
	uint8_t						sender;		//0:reserved,1: video analyzer,2:audio analyzer
	char						action;		//0:reserved,1:stop send to subsystem,2:stop send to main system,3:eliminate this programm
}msg_prog_sync_t;

#define MSG_RESP_CHECKRESULT_LEN	(sizeof(resp_checkresult_t))
typedef struct resp_checkresult_s 
{
	char						prog_id[8];		
#if K_PROJECT
	int							service;	
	char						level;					//the level of check result 
	uint8_t 					opt_num;				// 包含的选项数
	uint8_t 					pad[6];					// 预留，必须填0值
#else
	uint8_t						service;	
	char						level;					//the level of check result 
	uint8_t 					opt_num;				// 包含的选项数
	uint8_t 					pad;					// 预留，必须填0值
#endif
	uint32_t					cfg_id;					//config ID
	uint32_t					file_header_size;	
	uint32_t					file_packet_size;
}resp_checkresult_t;

typedef struct msg_log_s
{
    char  						prog_id[8];			//  program rule id;
    uint32_t					cfg_id;				//	hitted rule id;
    uint32_t					fount_time;			//	found time
    uint32_t					server_ip;			//	server ip;
    uint32_t					client_ip;			//	client ip;
    uint16_t					server_port;		//	server port;
    uint16_t					client_port;		//	client port;
    uint8_t						protocol;			//  translation protocol num; 
    uint8_t						service;			//	service type;
    uint8_t						level;				//	result level;
    uint8_t  					fd_type;				// 封堵类型, 0表示动态黑白名单，1表示静态配置命中，2表示二级分析节点命中。
}msg_log_t;

typedef enum
{
	RECV_RESP=0,
	RECV_RESP_WHITELIST,
	RESP_MEDIA_NOFOUND,
	RESP_REPEAT,
	RESP_AUDIO_LANG_MONITOR_NEW,
	RESP_AUDIO_LANG_MONITOR_OLD,
	RESP_SEND_BLOCK,
	RESP_SEND_BLOCK_MULTI,

	RECV_PROG_SYNC,
	RECV_PROG_SYNC_NOFOUND,	
	RECV_PROG_SYNC_AUDIO_STOP,
	RECV_PROG_SYNC_VIDEO_STOP,
	RECV_PROG_SYNC_UNKNOW,
	
	SEND_CONFIG_MONITOR,
	SEND_LANG_MONITOR,
}RESP_LOG_TYPE;

/*VOIP 监测日志地址格式*/
typedef struct layer_addr_v4_s
{
	uint8_t 	layer_cnt;	
	uint8_t 	reserved[3];	
	uint32_t 	client_addr; 	/* network order, client IP */
	uint32_t 	server_addr; 	/* network order, server IP */
	uint16_t 	client_port;		/* network order */
	uint16_t 	server_port;		/* network order */
}layer_addr_v4_t;

void resp_write_to_log(int type, resp_checkresult_t* check_res, void* param1, void* param2, uint64_t param3);


#endif
