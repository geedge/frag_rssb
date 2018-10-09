#ifndef _FRAG_REASSEMBLY_IN_H
#define _FRAG_REASSEMBLY_IN_H

#include </usr/include/stdint.h>
#include <sys/queue.h>

#include 	"stream_fuzzy_hash.h"
#include 	"MESA_list_queue.h"
#include 	"MESA_timer.h"

#include 	"interval_index.h"
#include 	"hiredis.h"
#include 	"hircluster.h"
#include 	"soqav_dedup.h"

#include 	"main.h"
#include 	"sifter.h"
#include 	"frag_reassembly.h"
#include 	"frag_reassembly_in.h"

#define		LAY_ADDR_CNT						8
#define		PID_MAX_LEN							64
#define  	SIFTER_MAX_NUM						8 	//for sifter

#define 	TD_LEN								64
#define 	REDIS_CMMD_MAXLEN					2048 /*redis命令的最大长度*/

/*节目缓存前端机信息的最大个数*/
#define 	QD_MAXNUM							8	

/*log type*/
typedef enum
{
	MEDIA_NEW=0,
	MEDIA_RENEW,	
	MEDIA_RENEW_EXPIRE,
	MEDIA_EXPIRE,	
	MEDIA_MID_CHANGE,
	MEDIA_OFFSET_ZERO,	
	MEDIA_FROM_CAPIP,		
	MEDIA_FROM_CPZIP,	
	MEDIA_FROM_QDIP,	
}MEDIA_LOG_TYPE;

/*response from frag_index*/
#define CONVG_MAXNUM  	16
typedef struct frag_cnvg_s
{	
	text_t* 				opt[CONVG_MAXNUM];					 		
	int 					opt_num;	
 }frag_cnvg_t;

/*opt_unit_t is for trans*/
typedef struct opt_in_s
{
	uint32_t 				opt_len;    //the len of opt_value in CPZ
	uint8_t					opt_type; 
	char*					opt_value; 
}opt_in_t;

#define FRAG_FLAG_SEND_META		0x01	//节目的第一个数据包，需要发送元信息
#define FRAG_FLAG_MULTISRC		0x02    //多源标志，发送到其他粗瓶装
#define FRAG_FLAG_WINS			0x04    //副系统标志，发送wins
typedef struct frag_in_s
{	
	uint64_t				mid;
	uint64_t				new_mid;    //非多源情况下:mid=new_mid; 多源情况下:new_mid为多源汇聚的ID
	uint64_t				pid; 
	uint64_t				offset_in; 
	uint16_t				seq;
	uint64_t				offset:48;	
	char*					data;			//not copy
	uint32_t				datalen;
	uint32_t				multisrc_bizmanip;	  //多源聚合的IP	
//    int64_t 				create_time;	
	uint32_t				src_ip;	
	int 					thread_seq;	//record thread_seq when converge is not complete
	int 					frag_flag;	//本数据包的标志			
//	uint8_t					media_type;		
//	uint8_t					proto;		
}frag_in_t;

/*netdisk and other frag*/
#define	FRAG_UNIT_INFO_NUM	6	
typedef enum
{
	FRAG_UNIT_ID=0,				//from frag_sifter
	FRAG_UNIT_ABOFFSET=1,		//maybe from index info or frag_sifter
	FRAG_UNIT_REOFFSET=2, 		//maybe from index info or frag_sifter
	MEDIA_ID=3,					//maybe from index info or frag_sifter
	MEDIA_SIZE=4,				//maybe from index info or frag_sifter
	MEDIA_NAME=5,				//maybe from index info or frag_sifter		
}FRAG_UNIT_INFO;

/*frag_stat 整形的状态*/
#define 	STAT_INIT								0x00
#define 	STAT_CNVG_QUERY							0x01  //query but not result
#define 	STAT_CNVG_OK							0x02
#define 	STAT_INDEX_QUERY						0x03
#define 	STAT_OK									0x04

/* frag_unit_t  flag*/
#define 	FRAG_UNIT_ADDMEDIA						0x01
#define 	FRAG_UNIT_MONITOR			 			0x02	
#define 	FRAG_UNIT_OFFSET						0x04	
#define 	FRAG_UNIT_RECORD						0x08	
#define 	FRAG_UNIT_MULTI							0x10	

#define		WEBMAIL_CONT_INFO_NUM					10

/*SIP选项个数*/
#define 	SIP_OPT_NUM								32
/*SIP查询redis的命令个数*/
#define 	SIP_REDIS_CMMD_NUM						35
/*SIP的选型*/
typedef struct sip_opt_s
{
	char	opt_name[32];
	int		opt_type;     /*在全量日志里面的选项类型，不是JC日志里面的选项类型*/
}sip_opt_t;

typedef struct qd_info_s
{
	uint64_t	mid;	
	uint32_t	cap_ip;
}qd_info_t;

typedef enum
{
	MEDIA_OPT_URL=0,		
	MEDIA_OPT_PID,
	MEDIA_OPT_ADDR,	
	MEDIA_OPT_SINGLE_KEY,
	MEDIA_OPT_UA,
	MEDIA_OPT_REFERER,
	MEDIA_OPT_ETAG,
	MEDIA_OPT_LAST_MODIFY,
	MEDIA_OPT_SERVER,
	MEDIA_OPT_TD_META,	
	MEDIA_OPT_OFFSET,
	MEDIA_OPT_C2S_CONT_TYPE,	
	MEDIA_OPT_S2C_CONT_TYPE,
	MEDIA_OPT_CAP_IP,
	MEDIA_OPT_PROTOCOL,
	
	MEDIA_OPT_INDEX_URL,
	MEDIA_OPT_INDEX_REFERER,
	MEDIA_OPT_INDEX_UA,	
	MEDIA_OPT_FD_SUBSTR,	
	
	MEDIA_OPT_S_IP,
	MEDIA_OPT_S_PORT,
	MEDIA_OPT_C_IP,
	MEDIA_OPT_C_PORT,
	
	MEDIA_OPT_MAXNUN,
}MEDIA_OPT;

typedef struct frag_unit_s
{	
	MESA_lqueue_head		frg_cnvg_lq;	
	MESA_lqueue_head		frg_index_lq;
	text_t** 				frg_info; 					 //useful info after sifter

	text_t* 				text;						 //from media_info opt to sifter
	int 					text_num;	
	int 					thread_seq;	

	uint64_t				pid;					 	 //from media_info
	uint64_t				media_len;					 //from media_info		
	       		
	uint32_t 				capIP;						//粗瓶装接收数据的前端机器IP
	uint32_t 				src_ip;	

#if K_PROJECT
	int						hitservice;   				 //from media_info	
#else
	uint8_t 				hitservice; 				 //from media_info	
	uint8_t					pad[3];
#endif
    uint8_t   				data_flag;					 //from media_info	
    int8_t   				flag;						 //from media_info	
	uint8_t					media_type;			     	 //from media_info		
	uint8_t					proto;						 //from data	
	
	qd_info_t 				qd_info_from_cpz[QD_MAXNUM];			  //前端机器的信息
	opt_in_t* 				opt[MEDIA_OPT_MAXNUN];				  		   
	uint64_t				content_length;				  //from media_info opt 
	
	uint64_t				mid;	
	uint64_t				re_offset;		
	uint64_t				ab_offset;
	uint64_t				ab_offset_for_in;
	uint64_t				ab_offset_for_mime;			  //mime extract data, update offset

	int 					service_id;		
    uint32_t   				mediainfo_cnt;			
	
	uint8_t					repeat_not_proc;			  //such hls, osmf , same reoffset		
	uint8_t					frag_state; 	
	uint8_t					multi_flag;  	
    uint8_t 				qd_info_from_cpz_idx_last;

	opt_in_t*				sip_diadata_ID; 			//存储META_OPT_SIP_DIADATA_ID 
	opt_in_t*				sip_data_dir; 				//存储META_OPT_SIP_DATA_DIR
	opt_in_t*				sip_rate_info; 				//存储META_OPT_SIP_DATA_DIR	
	opt_in_t*				sip_opt[SIP_OPT_NUM]; 	
}frag_unit_t;

#define INFO_MEDIA_NUM			1	
#define PID_MAXNUM				256	
#define	KEEP_REOFFSET_MAXNUM	2048

/*多源查询返回结果前的缓存队列*/
struct queue_item
{
	void* 						node;
	int							thread_seq;
	TAILQ_ENTRY(queue_item) 	entries;
};

TAILQ_HEAD(qw_queue, queue_item);

#define	AUDIO_WINS_DISABLE		0x01
#define	VEDIO_WINS_DISABLE		0x02
	
#define	SIP_SURVEY_TYPE_FD		0x01
#define	SIP_SURVEY_TYPE_JC		0x02
#define	SIP_SURVEY_TYPE_FD_JC	0x03

#define	MEDIA_SERVICE_TYPE_AV	0x00
#define	MEDIA_SERVICE_TYPE_FRAG	0x01
#define	MEDIA_SERVICE_TYPE_SIP	0x02
#define	MEDIA_SERVICE_TYPE_APP	0x03
#define	MEDIA_SERVICE_TYPE_PIC	0x04

/*每个节目最多个多线程*/
#define OPT_MAXNUN 				256
typedef struct media_s
{		
	IVI_t*					ivi;							//complete frag removal
	IVI_t*                  save_ivi;
	sfh_instance_t*         fuzzy;	
	uint64_t				fuzzy_acc_len;             		//SFH_feed acc len	
	
	MESA_lqueue_head		app_frg_lq;						//app数据缓存

	text_t* 				media_info[INFO_MEDIA_NUM]; 	//MEDIA_INFO_TYPE		
	
	uint64_t				mid_after_multisrc;			    //多源之后mid可能被修改
	uint64_t				mid;							//前端发送的节目ID
	uint64_t*				pid;							//PID_MAXNUM, need when media_json 
    int64_t 				create_time;	
    int64_t 				renew_time;	
    int64_t 				lastpkt_time;	
    int64_t 				dedup_query_time;	

	char					td[TD_LEN];						
	char*					td_data;	
	uint32_t				td_datalen;		
	uint32_t				addrlist_len;					//for app
	char*					addrlist;
	
	opt_in_t* 				opt[MEDIA_OPT_MAXNUN][OPT_MAXNUN];	
	int						opt_index;
	int						url_opt_index;					//存在url的所在的数组位置

	struct qw_queue 		query_wait_lq;				 	//等待降载多源查询的结果
	MESA_timer_index_t* 	timer_idx;
	MESA_timer_index_t* 	index_query_timer_idx;

	uint64_t				byte_in;	
    uint64_t				byte_proc;		
    uint32_t				pkt_in;		
    uint32_t				pkt_proc;
	
	uint64_t				maxoffset;			 			
	uint64_t				media_len;					 	//from more frag_unit 	

#if K_PROJECT
    int	 					hit_service;					//monitor flag   
#else
	uint8_t 				hit_service;					//monitor flag	 
	uint8_t 				pad[3];					//monitor flag	 
#endif
	uint8_t					proto;							 //from data, from frag_unit
	uint8_t					media_type;			     	 	//from media_info, from frag_unit	
    uint8_t 				data_flag;						//monitor flag       
    int8_t 					meta_flag;						

	int8_t 					flag;							// PROG_FLAG_EXCP  PROG_FLAG_DUMP  
	uint8_t 				qdinfo_idx_last;
    uint8_t 				pid_idx_last;		
    uint8_t 				repeat_reoffset_idx_last;
	
	uint64_t				acc_offset;                 	//HLS acc offset	
	int64_t					repeat_reoffset[KEEP_REOFFSET_MAXNUM];  //HLS reoffset that have proc,最多保留KEEP_REOFFSET_MAXNUM分片，重复的进行去重

	qd_info_t 				qd_info[QD_MAXNUM];	   			//接收前端机器的mid和capIP对应关系
	qd_info_t 				qd_info_from_cpz[QD_MAXNUM];	//多源条件下，其他粗拼装的前端信息

	uint32_t				frag_unit_cnt;					//for pid
	uint32_t				configID;
	uint32_t				multisrc_bizmanip;
	int 					thread_seq;	                	//the first frag_unit thread_seq		
	
    char 					wins_dest_disabled_bit;			//stop send to subsystem,equal to MAX_EXCP_PORT
	uint8_t					media_service_type;			  	//HLS or OSMF...  SIP AV
	int8_t					td_query;			  			//0:init 
	int8_t					cache_flag;			  			
	uint8_t					td_complete;			  		//0:init 	
    uint8_t 				qdinfo_from_cpz_idx_last;	

    uint8_t 				sip_survey_type;				//SIP_SURVEY_TYPE_FD SIP_SURVEY_TYPE_JC SIP_SURVEY_TYPE_FD_JC
   //uint8_t 				sip_sendlog_flag;				
	opt_in_t*				sip_opt[SIP_OPT_NUM];			//opttype is fulllog type
	opt_in_t*				sip_rate_info; 				
	opt_in_t*				sip_diadata_ID; 				
	uint64_t				re_offset;						
	char*					fd_buf;								
	char*					jc_buf;								
	uint32_t				fd_buflen;								
	uint32_t				jc_buflen;	

	/*建议结果*/
    int64_t 				survey_time;	
	uint32_t				cfg_id;					//config ID
	uint8_t					service;	
	char					level;					//the level of check result 	
	char					log_url[512];
	char* 	 				monitor_path;
	//char					frag_bitmap[KEEP_REOFFSET_MAXNUM/8+1];			//bitmap for frag removal
}media_t;

/*recv : add_media_info for cnvg_hash*/
typedef struct rssb_media_info_s
{	
	uint64_t 				pid;
	uint64_t 				media_len;
	char* 					opt; 
	
	int 					opt_num;
	int 					thread_seq;	
	
	uint32_t				cap_IP;
	uint32_t				src_ip;	
	
#if K_PROJECT
	int						hitservice;
#else
	uint8_t 				hitservice;	
	uint8_t 				pad[3];
#endif
	uint8_t					media_type;
	uint8_t					protocol;
	uint8_t					data_flag;
	int8_t					flag;
}rssb_media_info_t;

typedef struct frag_reassembly_s
{	
	void*					sifter;
	void*					logger;
	void*					media_logger;

	MESA_htable_handle      converge_hash;  /*frag 碎片会话HASH*/	
	
	MESA_lqueue_head*  		wait_lq; 	
	
	uint16_t 				lq_num;
	uint16_t				logger_level;		
	
	uint32_t				cnvg_hash_thread_safe;
	uint32_t				cnvg_hash_size;
	uint32_t				cnvg_hash_max_elem_num; 
	uint32_t				cnvg_hash_expire_time;	

	/*stat**/
	uint32_t				sysinfo_interval;	
	uint32_t				stat_interval;	

	void*					sysinfo_handle;
	void*					stat_handle;
	
	/*sysinfo*/
	uint64_t                stat_info[RSSB_LOG_TYPE_MAXNUM];    
	uint64_t				data_info[RSSB_DATALOG_TYPE_MAXNUM][LOG_STAT_MAXNUM];	
	uint64_t				sysinfo_stat[RSSB_SYSLOG_TYPE_MAXNUM][SYSLOG_STAT_MAXNUM];	

	int						log_field_id[RSSB_LOG_TYPE_MAXNUM];	
	int						datalog_column_id[LOG_STAT_MAXNUM];	
	int						datalog_line_id[RSSB_DATALOG_TYPE_MAXNUM];
	int						syslog_column_id[SYSLOG_STAT_MAXNUM];	
	int						syslog_line_id[RSSB_SYSLOG_TYPE_MAXNUM];	

	/*redis*/
	struct timeval 			redis_tv;
	char					redis_addr[20480];
	redisClusterContext* 	redis_cluster_ctx[MAX_THREAD_NUM];
	redisContext* 			redis_ctx[MAX_THREAD_NUM];	
	int						redis_cluster_switch;	
	char					redis_ip[32];
	int						redis_port;
	
	uint32_t				wait_lq_num;
}frag_reassembly_t;

/*printaddr 格式, used in app*/
typedef struct __touple4_type
{
	char 					sip[64];
	char 					dip[64];
	unsigned short 			sport;
	unsigned short 			dport;
	int 					addr_type;
}touple4_type_t;

#define FRAG_CONTAIN_MAXNUM		8	                    //去重:完全覆盖的frag数，用于去重
/*for create_media  because IVI */
typedef struct frag_ivi_info_s
{	
	frag_unit_t*			frg_unit;
	frag_in_t*				frg;
	//query_detail_t*		query_detail;
	frag_in_t* 				frg_array[FRAG_CONTAIN_MAXNUM];//frag可能因为去重被拆分为多个frag	
	//char					td_result[TD_LEN];
	uint64_t				mid;
	int						frg_array_cnt;
	uint8_t					td_query;					//TD_QUERY_TYPE_MULTISRC TD_QUERY_TYPE_DEDUP
	uint8_t					thread_seq;					//for av dedup
}frag_ivi_info_t;


#ifdef __cplusplus
extern "C" {
#endif
int 		frag_add_wait_lq(frag_ivi_info_t* frag_ivi_info, uint32_t frag_stat, int thread_seq);
void 		init_frag_unit(frag_unit_t* frg_unit, uchar protocol);
int 		media_create(frag_unit_t* frg_unit);
int 		frag_service(frag_ivi_info_t* frag_ivi_info, uint32_t src_ip, int thread_seq);
void 		free_opt(opt_in_t** data);
void 		free_text(text_t** pp, int n_p);
int 		free_frag_in(void *data, long data_len, void *arg);
uint64_t 	make_mid(char * key, unsigned int size, unsigned char type);
void 		free_frag_unit(void* data);
void 		free_media(void* data);
int 		expire_media_hash_node(void *data, int eliminate_type);

#ifdef __cplusplus
}
#endif


#endif
