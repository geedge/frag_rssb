#ifndef _AV_INTERFACE_H
#define _AV_INTERFACE_H

#define PROTO_VERSION  					3
#define PROTO_MAGICNUM 					0x5641

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
#define MSG_PROG_SYNC					0x15	//not proc now	

/*prog sync*/
#define SYNC_SENDER_VIDEO_ANALYZER		0x01
#define	SYNC_SENDER_AUDIO_ANALYZER		0x02
#define	SYNC_ACTION_STOP_TO_SUBSYSTEM	0x01
#define	SYNC_ACTION_STOP_TO_MAINSYSTEM	0x02
#define	SYNC_ACTION_ELIMINATE_PROGRAM	0x03

/*元信息localIP选项，根据*/
#define OPT_SOURCE_IP					0x41

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

#define MSG_MEDIAINFO_HEAD_LEN	 24
/*Data_flag defination*/
typedef struct msg_metainfo_s
{
	char						prog_id[8];	
	char    					flag;
    uint8_t   					hitservice;
	uint64_t  					prog_len:48;
	uint32_t					cap_IP;
	uint8_t						protocol;		
	uint8_t						media_type;	
	uint8_t						data_flag;	
	uint8_t						opt_num;      
}msg_metainfo_t;

#define MSG_DATA_HEAD_LEN	(sizeof(msg_data_t))
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
	uint8_t						service;	
	char						level;					//the level of check result 
	char						pad[2];		
	uint32_t					cfg_id;				//config ID
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

	RECV_PROG_SYNC,
	RECV_PROG_SYNC_NOFOUND,	
	RECV_PROG_SYNC_AUDIO_STOP,
	RECV_PROG_SYNC_VIDEO_STOP,
	RECV_PROG_SYNC_UNKNOW,
	
	SEND_CONFIG_MONITOR,
	SEND_LANG_MONITOR,
}RESP_LOG_TYPE;
void resp_write_to_log(int type, resp_checkresult_t* check_res, void* param1, void* param2);


#endif
