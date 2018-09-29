#ifndef _H_COM_SENDTOBACK_H
#define _H_COM_SENDTOBACK_H

#include <sys/socket.h>

// Defination for all kinds of msg 
#define PROTO_MAGICNUM 0x5641
#define PROTO_VERSION  3

// Conf Msg
#define MSG_MANAGER	0x00		// System Manager Configure Message
#define MSG_CONFIG_ALL	0x01	// All Configure Update Message
#define MSG_CONFIG_INC	0x02	// Increase Configure Update Message
#define MSG_CONFIG_REQ	0x03	// Configure Request Message

#define MSG_D_CONFIG_ALL 0x04 // Configure Require's Response Message .
#define MSG_D_CONFIG_INC 0x05 // Configure Require's Response Message .
#define MSG_D_CONFIG_REQ 0x06 //  Dynamic Configure Item Require Message This message come from configure server ;
#define MSG_TUPLE5_URL_REQ 0x0A // Tuple5 to url Request;

#define MSG_P2P_D_CONFIG_ALL 0x71 		// Configure Require's Response Message .
#define MSG_P2P_D_CONFIG_INC 0x72 		// Configure Require's Response Message .
#define MSG_P2P_D_CONFIG_REQ 0x73 	// Configure Require's Response Message .
// Conf Operation defination
#define CONFIG_OP_TYPE_NO	0x00	// no any operation
#define CONFIG_OP_TYPE_ADD	0x01	// add a config rule
#define CONFIG_OP_TYPE_DEL	0x02	// del a config rule
#define CONFIG_OP_TYPE_MOD	0x03	// modify a config rule

// Response Msg

#define MSG_RESP_CHECKRESULT 	0x11	// Check Result notification
#define MSG_RESP_REJECT		0x12	// Reject Message notification
#define MSG_RESP_CHARACTER	0x13	// Character notification

// Log Msg
#define MSG_MANAGER_LOG	0x20	// Manager Log Msg
#define MSG_RESULT_LOG	0x21	// Result Log Msg for Block or Monitor
#define MSG_STATIC_LOG	0x22	// Valid Message
#define MSG_TUPLE5_URL_LOG 0x2A // Tuple response Message

// Data Msg
#define MSG_DATA_METAINFO 0x31
#define MSG_DATA_BODY 0x32
#define MSG_DATA_TAIL 0x33

// Definatin for all types of vidio and audio file;
#define FILE_UNKNOWN    0x00
#define FILE_VIDEO		0x60
#define FILE_WMV		0x61		
#define FILE_MPG		0x62		
#define FILE_FLV		0x63	
#define FILE_RMFF		0x65
#define FILE_AVI			0x66
#define FILE_SWF		0x67
#define FILE_MPG4		0x68
#define FILE_AIFF		0x69
#define FILE_OGG		0x6A
#define FILE_DRC			0x6B
#define FILE_DIRECTSHOW	0x6C
#define FILE_FLIC		0x6D
#define FILE_INDEO		0x6E
#define FILE_MKV			0x6F

#define FILE_AUDIO		0x70
#define FILE_MP3			0x71

#define BE_IMAGE_TYPE(t) ((t>=FILE_IMAGE) && (t<0x90))
#define BE_VIDEO_TYPE(t) ((t>=FILE_VIDEO) && (t<0x70))
#define BE_AUDIO_TYPE(t) ((t>=FILE_AUDIO) && (t<0x80))

#define FILE_IMAGE		0x80

#define FILE_JPG		0x84
#define FILE_BMP		0x85
#define FILE_GIF		0x86

#define MMS_TYPE		0x90
#define HTTP_STREAM_TYPE  0x90
#define RTSP_RDT_TYPE	0x92
#define RTSP_RTP_TYPE	0x91


#define SERVICE_FLAG_FD 0		// FenDu Flag
#define SERVICE_FLAG_JC 1		// JianCe Flag
// Defination of service type ;
#define SERVICE_IPPORT_WHITE    0x01	//IP+PORT白名单
#define SERVICE_IPPORT_BLACK    0x02	//IP+PORT黑名单
#define SERVICE_URL_WHITE     0x03	//节目地址白名单
#define SERVICE_URL_BLACK     0x04	//节目地址黑名单
#define SERVICE_HOST_WHITE     0x05	//节目地址黑名单
#define SERVICE_HOST_BLACK     0x06	//节目地址黑名单

// added by cxj 20100811
#define SERVICE_P2P_STATIC_WHITEIP	0x15
#define SERVICE_P2P_BI_WHITEIP			0x16
#define SERVICE_P2P_MI_WHITEIP		0x17

#define SERVICE_P2P_EMULE_SERVER		0x51		//
#define SERVICE_P2P_EMULE_KEY			0x52		//
#define SERVICE_P2P_EMULE_FILEID		0x53		//
#define SERVICE_P2P_EMULE_NODE		0x54		//

#define SERVICE_P2P_BT_INFOHASH		0x41			// BT Infohash Black List
#define SERVICE_P2P_BT_NODE		0x44			// BT Bone Node
#define SERVICE_P2P_BT_SERVICE		0x61			// BT DHT Index Server List

#define SERVICE_P2P_BT_ECLIPSE_IPLIST	0x45
#define SERVICE_P2P_BT_POLLUTION_IPLIST	0x46
#define SERVICE_P2P_EMULE_POLLUTION_IPLIST	0x55


//
#define SERVICE_EMULE_SERVER       0x71	//节目唯一ID白名单
#define SERVICE_EMULE_KEYS       0x72	//节目唯一ID黑名单
#define SERVICE_EMULE_FHID     0x73	//emule的hashkey黑名单
#define SERVICE_EMULE_FNAME     0x74	//emule的hashkey黑名单
#define SERVICE_EMULE_NODE     0x75	//emule的hashkey黑名单

#define SERVICE_IPPORT_WHITE_JC    0x81	//IP+PORT白名单
#define SERVICE_IPPORT_BLACK_JC    0x82	//IP+PORT黑名单
#define SERVICE_URL_WHITE_JC    0x83	//IP+PORT白名单
#define SERVICE_URL_BLACK_JC    0x84	//IP+PORT黑名单
#define SERVICE_HOST_WHITE_JC    0x85	//IP+PORT黑名单
#define SERVICE_HOST_BLACK_JC    0x86	//IP+PORT黑名单

#define MAX_SERVICE_NUM		0xff		
// Manager Command Defination
#define MANAGER_AV_SYS_CMD	0x02010000	//音视频系统管理命令
#define MANAGER_AV_PIC_CMD	0x02020000	// 
#define MANAGER_AV_SYS_QUERY		0x01	//查询音视频模块状态
#define MANAGER_AV_SYS_OPEN		0x02	//打开音视频模块
#define MANAGER_AV_SYS_CLOSE		0x03	//关闭音视频模块
#define MANAGER_AV_SYS_CHANGE	0x04	//切换音视频模块处理状态
// add by cxj for P2P Manager Command
#define MANAGER_P2P_BI_GROUP_UPDATE		0x11	// BISys's IP List Update
#define MANAGER_P2P_MI_GROUP_UPDATE		0x12	// MISys's IP List Update
#define MANAGER_P2P_BT_ECLIPSE_UPDATE	0x13	// MISys's IP List Update



// Manager Command state Defination
#define MANAGER_PROCESS_STATE_UNKNOWN	0x00			// 未知
#define MANAGER_PROCESS_STATE_RUNING	0x10			// 处理中
#define MANAGER_PROCESS_STATE_OK		0x20			// 处理成功
#define MANAGER_PROCESS_STATE_UNTHORITY	0x30			// 未授权的操作
#define MANAGER_PROCESS_STATE_FAILED	0x40			// 可恢复的错误
#define MANAGER_PROCESS_STATE_ERROR		0x50			// 不可恢复的错误


#define MATCH_METHOD_SUB	0x00	// sub match;
#define MATCH_METHOD_FUL	0x01	// full match;

#define MsgBodyHeadLen sizeof(struct msg_body_t)
#define UDP_SENDBACK_MAX_LEN 4*1024
#define UDP_SENDBACK_MAX_DATALEN UDP_SENDBACK_MAX_LEN-MsgHeadLen-MsgBodyHeadLen


typedef unsigned char   UINT8;
typedef unsigned int   UINT;
typedef unsigned long long UINT64;
typedef unsigned int   UINT32;
typedef unsigned short   UINT16;
typedef unsigned char  UCHAR;
typedef char CHAR;
typedef unsigned short USHORT;

#define MSG_HEADER_LEN	sizeof(struct msg_header_t)
struct msg_header_t {
	USHORT  magic_num; /* 魔数，值必须为0xECD5BCBC */
	UCHAR version; /* 版本号，目前值为1 */
	UCHAR msg_type; /* 消息类型 */
	UINT cont_len; /* 消息体字节长度 */
}; /* sizeof = 8B */

#define OPT_HEAD_LEN (sizeof(UINT)+1)
struct opt_unit_t {
	UINT opt_len; /* 本选项的字节长度，包括本字段 */
	UCHAR opt_type; /* 本选项的类型 */
	CHAR *opt_value; /* 本选项的内容 */
};

//统计日志定义
#define MAX_STATICSSENDNUM 10000
struct statlog_unit_t {
	UINT16		unit_len;  // 本统计日志单元的字节长度，包括本字段
	UINT16		log_type;  // 统计日志单元类型
 	UINT32   hit_num;  // 命中次数
       char  prog_id[8];  // 节目ID

};
struct msg_statlog_t {
	UINT32 log_time;  // 统计日志产生时间
	UINT32 log_num;	 // 本消息包含的统计日志单元数
};


//五元组定义
struct msg_tup5_req_t {
	UINT src_ip;		
	UINT dst_ip;
	USHORT src_port;
	USHORT dst_port;
	UCHAR protocol;
	CHAR pad[3];
};

// Service Configure Unit Defination

struct cfg_unit_t {
	UCHAR	op_type;
	UCHAR	service;
	UCHAR	weight;
	UCHAR	flag;
	UINT	cfg_id;
	UINT	unit_len;
	CHAR *	value;
};
#define CFG_UNIT_HEAD_LEN (sizeof(UCHAR)*4 + sizeof(UINT)*2)

// added by cxj 20110811
// dynamic Configure Unit Defination;
struct d_cfg_unit_t{
	UINT64	groupid;
	UINT service;
	UINT unit_len;
	CHAR *value;
};

#define D_CFG_UNIT_HEAD_LEN	(sizeof(uint64_t) * sizeof(INT) *2)

// Manager Configure Unit Defination

struct cmd_unit_t{
	UINT	optor_id;
	UINT	cmd_sn;
	UINT	cmd_code;
	UINT	cmd_val;
	UINT	opt_len;
};

#define CMD_UNIT_HEAD_LEN	(sizeof(UINT) *5)

struct msg_cmdlog_t {
	UINT	log_time;	// 本次日志生成时间。当值在[0,0x7FFFFFFF]区间时取值和意义同 time_t结构；值不在本区间时的意义以后将另行定义
	UINT	log_num;	// 本消息包含的日志条数
	// struct cmdlog_unit_t[log_num] cmdlog_unit;
};

#define MSG_CMDLOG_HEAD_LEN	(sizeof(UINT) *2)

struct cmdlog_unit_t {
	UINT	CLJ_IP;  //执行指令的服务器IP地址，网络序
	UINT	operator_ID;  // 产生该日志的指令的操作员ID
	UINT	cmd_SN; //产生该日志的指令流水号
	UINT	cmd_code;	// 产生该日志的指令
	UCHAR	state;	// 产生该日志的指令的处理状态
	UCHAR	pad;  // 保留字段，目前必须置0值
	USHORT	opt_len;  // 选项内容字节长度（包括最后的'\0'字符）
	// char * opt_value;
};

#define CMDLOG_UNIT_HEAD_LEN	20



// Dynamic Configure Defination;
// The Unit Defination be same with cfg_struct_t, only the flag value is not same;
#define DYNAMIC_WHITE_FLAG	0
#define DYNAMIC_BLACK_FLAG	1

struct msg_cfg_t {
	UINT	cfg_time;	
	UINT	cfg_num;
};
#define MSG_CFG_HEAD_LEN (sizeof(UINT) * 2)



struct avs_data_t {
	CHAR	prog_id[8];	// 节目的ID，由一级节点生成
	UINT8	pad;// 保留，目前必须置0值
	UINT8	flags;// 数据标志，详细定义见下面
	UINT64  offset:48;	// 本次数据在整个节目中的偏移位置

/*	unsigned long long offset;		// 本次数据的段内偏移位置
	UINT	dst_ip;		// 本次数据包的目的IP地址
	UINT	src_ip;		// 本次数据包的源IP地址
	USHORT	dst_port;	// 本次数据包的目的端口
	USHORT	src_port;	// 本次数据包的源端口
*/	
};
#define AVS_DATA_HEAD_LEN	(sizeof(struct avs_data_t))
//Protocol Defination
#define PROTO_HTTP		0x01
#define PROTO_SMTP		0x02
#define PROTO_POP3		0x03
#define PROTO_FTP		0x04

#define PROTO_HTTP_STREAM	0x81
#define PROTO_RTSP_RDT		0x82
#define PROTO_RTSP_RTP		0x83
#define PROTO_MMS		0x84
#define PROTO_ED2K		0x85

#define HITSER_WHITE _IPPORT 0x81 
#define HITSER_BLACK _IPPORT 0x82
#define HITSER_WHITE_URL     0x83
#define HITSER_BLACK_URL     0x84
#define HITSER_WHITE_DOMAIN  0x85
#define HITSER_BLACK_DOMAIN  0x86

// Data_flag defination
struct metainfo_t {
	CHAR	prog_id[8];	// 节目的ID，由一级节点生成
	CHAR    flag;
    UINT8   hitservice;
	UINT64  prog_len:48;
	UINT	cap_IP;
	UCHAR	protocol;		// 协议类型
	UCHAR	media_type;	// Media type
	UCHAR	data_flag;	// Data Flag;
	UCHAR	opt_num; //选项信息单元个数
	struct opt_unit_t * opt_unit;
};
#define METAINFO_HEAD_LEN	24
//响应消息格式
struct survey_ind_t {
	CHAR	prog_id[8];	// 节目的ID，由一级节点生成
	UCHAR	service;	// 业务类型
	CHAR	level;		// 检测结果敏感程度
	CHAR	pad[2];		// 保留，目前必须填0
	UINT	cfg_id;		// 命中的配置ID
	UINT	file_header_size;	//
	UINT	file_packet_size;
};
#define SURVEY_MSG_FIX_LEN	(sizeof(struct survey_ind_t))
#define SURVEY_PAYLOAD_LEN_MAX (1460-MSG_HEADER_LEN-SURVEY_MSG_FIX_LEN)

struct msg_survey_ack_t {
    CHAR    prog_id[8];
    UCHAR   service;
    CHAR    level;
    CHAR    pad[2];
    UINT    cfg_id;
};


struct live_data_t {
    CHAR    prog_id[8];
};


// for log 

struct msg_log_t
{

    CHAR  	prog_id[8];//  program rule id;
    UINT	cfg_id;				//	hitted rule id;
    UINT	fount_time;			//	found time
    UINT	server_ip;			//	server ip;
    UINT	client_ip;			//	client ip;
    USHORT	server_port;		//	server port;
    USHORT	client_port;		//	client port;
    UCHAR	protocol;			//  translation protocol num; 
    UCHAR	service;			//	service type;
    UCHAR	level;				//	result level;
    UCHAR  	fd_type;				// 封堵类型, 0表示动态黑白名单，1表示静态配置命中，2表示二级分析节点命中。
    USHORT   opt_num;			//	option number;
    struct opt_unit_t * opt_info;	// option point;

};
//#define MSG_LOG_FIX_LEN (sizeof(struct msg_log_t)-sizeof(void *))

#define FD_TYPE_DYNAMIC	0
#define FD_TYPE_STATIC		1
#define FD_TYPE_ANALYSE	2

#define SET_LOG_MSG(log,pid,cid,ft,sip,cip,sport,cport,pro,s,l,ftype,o_num,o_info)	\
{			\
	memcpy(log.prog_id,pid,8); 	\
	log.cfg_id = cid;\
	log.fount_time = ft;	\
	log.server_ip = sip;	\
	log.client_ip = cip;	\
	log.server_port =sport;	\
	log.client_port =cport;	\
	log.protocol = pro;	\
	log.service = s;	\
	log.level = l;	\
	log.fd_type = ftype;	\
	log.opt_num = o_num;	\
	log.opt_info = o_info;	\
}


struct msg_tup5_resp_t {
	UINT	src_ip;	   // 五元组之一，本次数据的源IP地址
	UINT	dst_ip;	   // 五元组之一，本次数据的目的IP地址
	USHORT	src_port;  // 五元组之一，本次数据的源端口
	USHORT	dst_port;  // 五元组之一，本次数据的目的端口
	UCHAR	protocol;  // 五元组之一，传输层协议类型
	UCHAR	pad; 	// 保留字段，必须置0值
	USHORT  cont_len;  	// 消息内容内容字节长度
	char *cont;        	// 消息的URL内容
};
#define MSG_TUP5_RESP_FIX_LEN 16


// Common
int feedback_mem (int service, void *info, char *buf, int buf_len);
// soft balance
int feedback_mem_soft_balance (unsigned long long pid, char *buf, int buf_len);
// soft balance
int direct_feedback_mem_soft_balance (unsigned long long pid, char *buf, int buf_len, int thread_num);
// hardware balance
int feedback_mem_balance (unsigned long long pid, char *buf, int buf_len);
// feedback init
int feedback_init();
//void gfn_AppCounterInit();
//void gfn_AppCounterFree();
//int gfn_AppCounterAdd(int ProtolType);
//void gfn_AddSendBackOpt(struct msg_header_t *pMsgHeader,struct msg_body_t   *pMsgBodyHeader,
//						char *pSendBackBuf,int OptType,char *SrcData,int SrcLen,char **ppOptbuf);
//void gfn_AddSendBackData(struct msg_header_t *pMsgHeader,struct msg_body_t   *pMsgBodyHeader,
//						 char *pSendBackBuf,char *SrcData,int SrcLen,char **ppOptbuf);
//int gfn_iSendToBEnd(tdLog* pstLog, int iLen, CBuffer* pBuf);



//void InitLogMessageLocal();
//int LogMessageLocal(char *buf,int buflen);


#ifdef __cplusplus
extern "C"
{
#endif
	

struct CFG_Common
{
	UCHAR	pad;
	UCHAR	service;
	UCHAR	weight;
	UCHAR	flag;
	UINT	cfg_id;
};

#define CFG_COMMON_LEN	sizeof(struct CFG_Common)

struct CFG_AV_Content
{
	struct CFG_Common comm_header;	// common header;
	UINT contentlen;					// length of content;
	char * content;						// AV config Content;
};

#define CFG_AV_CONTENT_FIXLEN  (CFG_COMMON_LEN+sizeof(UINT))

struct CFG_IP_SETTING{
	UINT sip;							// server ip;
	UINT mask;						//
	USHORT port;						// server port;
	UCHAR udpkey[4];
}__attribute__((__packed__));


int sendnto(int s, const void *msg, int n, unsigned int flags, const struct sockaddr *to, int tolen);
	
int gfn_iSetAVNewRuleTable(int iScanType,int iTableType,int iRuleNum,void * pstRTable);
	
int gfn_CScan_AVSearchMem(int iScanType,int iRuleType, char *data, int datalen, int begPos,struct cfg_unit_t *gResult,  int *pFoundPostion, void **ppParam,int thread_num);

#ifdef __cplusplus
}
#endif
	


#endif
