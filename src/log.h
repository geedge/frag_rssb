#ifndef _LOG_H
#define _LOG_H

#if(__GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__ >= 410)
#define atomic_inc(x) __sync_add_and_fetch((x),1)
#define atomic_dec(x) __sync_sub_and_fetch((x),1)
#define atomic_add(x,y) __sync_add_and_fetch((x),(y))
#define atomic_sub(x,y) __sync_sub_and_fetch((x),(y))
typedef long atomic_t;
#define ATOMIC_INIT(i)  { (i) }
#define atomic_read(x) __sync_add_and_fetch((x),0)
#define atomic_set(x,y) __sync_lock_test_and_set((x),y)
#else 
typedef long atomic_t;
#define atomic_inc(x)	((*(x))++)
#define atomic_dec(x)	((*(x))--)
#define atomic_add(x,y) ((*(x))+=(y))
#define atomic_sub(x,y) ((*(x))-=(y))
#define ATOMIC_INIT(i)  { (i) }
#define atomic_read(x) 	(*(x))
#define atomic_set(x,y)	((*(x))=(y))
#endif 

#define setbit(x,y)  x|=(1<<y)
#define clrbit(x,y)  x&=~(1<<y)

/*********************************************debug log type***********************************/
typedef enum
{
	ADD_META=0,
	ADD_FRAG,
	ADD_FRAG_NOUSE,
	GET_META,
	GET_FRAG,
	ADD_FRAG_TO_CNVG_LQ,	
	GET_FRAG_FROM_CNVG_LQ,
	ADD_FRAG_TO_INDEX_LQ,
	GET_FRAG_FROM_INDEX_LQ,
	ADD_FRAG_TO_WAIT_LQ,

	ADD_FRAG_TO_TAILQ,
	ADD_FRAG_FROM_TAILQ,
	
	ADD_FRAG_TO_APP_LQ,

	MEDIA_NO_META,
	HLS_OSMF_REPEAT,
	SET_OFFSET,
	
	APP_CHANGE_PID,

	/*cnvg查询*/
	SEND_CNVG_QUERY_1,
	RECV_CNVG_ACK_1,	
	CNVG_QUERY_FAIL_1,

	SEND_CNVG_QUERY_2,
	RECV_CNVG_ACK_2,	
	CNVG_QUERY_FAIL_2,
	
	CNVG_FAIL_PROC,    /*查询成功，但是因为模板抽取失败*/
	
	/*AV查询*/
	SEND_AV_QUERY_1,
	RECV_AV_ACK_1,
	AV_QUERY_FAIL_1,

	SEND_AV_QUERY_2,
	RECV_AV_ACK_2,
	AV_QUERY_FAIL_2,
		
	/*碎片索引查询*/
	SEND_INDEX_QUERY_1,
	RECV_INDEX_ACK_1,	
	INDEX_QUERY_FAIL_1,

	SEND_INDEX_QUERY_2,
	RECV_INDEX_ACK_2,	
	INDEX_QUERY_FAIL_2,
		
	INDEX_FAIL_PROC,    /*查询成功，但是因为模板抽取失败*/

	/*VOIP查询*/	
	SEND_VOIP_QUERY_1,
	RECV_VOIP_ACK_1,
	VOIP_QUERY_FAIL_1,

	SEND_VOIP_QUERY_2,
	RECV_VOIP_ACK_2,
	VOIP_QUERY_FAIL_2,

	/*降载*/
	SEND_AV_DEDUP_QUERY,
	RECV_AV_DEDUP_ACK,
	RECV_AV_DEDUP_ACK_KNOWN,
	RECV_AV_DEDUP_ACK_MULTI,
	AV_DEDUP_QUERY_TIMEOUT,
	AV_DEDUP_REPORT,		

	/*碎片预告，反查URL，URL不命中碎片特征*/
	FRAG_FORECAST_ERROR,
}FRAG_LOG_TYPE;

/*********************************************stat log type***********************************/
typedef enum
{
	LOG_MEDIA_CREATE=0,	
	LOG_MEDIA_RENEW,		
	
	LOG_MEDIA_EXPIRE,	
	LOG_MEDIA_OUTPUT,	
	
	LOG_MEDIA_HTTP,	
	LOG_MEDIA_OFFSET_ZERO,	
	LOG_MEDIA_DEDUP_QUERY,
	LOG_MEDIA_TD_QUERY,
	LOG_MEDIA_DEDUP_ACK,	
	LOG_MEDIA_DEDUP,		
	LOG_MEDIA_MULTI,			
	LOG_MEDIA_DEDUP_ACK_TIMEOUT,
	LOG_MEDIA_DEDUP_TIMEOUT,
	LOG_MEDIA_DEDUP_REPORT,
	LOG_MEDIA_DEDUP_REPORT_COMPLETE,
		
	LOG_AV_RECORD,	
	LOG_AV_DIGEST_RECORD,	
	
	MEDIALOG_TYPE_MAXNUM,	
}MEDIALOG_TYPE;

typedef enum
{
	/*recv data*/
	RECV=0,	
	RECV_DROP,
	INVALID_RECV,
	META_RECV, 	
	DATA_RECV,
	OTHER_RECV,		
	MEDIA_NOMETA,

	/*recv response message*/
	RESP_UNRECOGNIZED,
	RESP_CHARACTER,
	
	INVALID_RESP_CHECKRESULT,
	RESP_CHECKRESULT,	
	RESP_CHECKRESULT_WHITELIST_IN,  						//resp msg whitelist : level = 0;
	RESP_CHECKRESULT_BLACKLIST_IN,							//resp msg blacklist : level > 0;
	RESP_CHECKRESULT_BLACKLIST_IN_NOFOUND,					//resp msg blacklist : level > 0; not found 
	RESP_CHECKRESULT_VOIP_FULLLOG,							//VOIP full log 
	RESP_CHECKRESULT_VOIP_SURVEYLOG,						//VOIP survey log
	RESP_CHECKRESULT_BLACKLIST_IN_REPEAT,					//resp msg blacklist : level > 0; repeat
	RESP_CHECKRESULT_BLACKLIST_IN_MONITOR_LEVEL1,			//resp msg blacklist : level > 0;  monitor service, first
	RESP_CHECKRESULT_BLACKLIST_IN_MONITOR_LEVEL2,			//resp msg blacklist : level > 0;  monitor service, again
	RESP_CHECKRESULT_BLACKLIST_IN_BLOCK,					//resp msg blacklist : level > 0;  block service
	
	RESP_CHECKRESULT_FRAG_SURVEY,							//frag survey log
	RESP_CHECKRESULT_INDEX_SURVEY,							//frag survey log include index
	
	INVALID_RESP_PROG_SYNC,	
	RESP_PROG_SYNC,	
	RESP_PROG_SYNC_IN_NOFOUND,
	RESP_PROG_SYNC_AUDIO,
	RESP_PROG_SYNC_VIDEO,	

	/*send survey*/
	RESP_CHECKRESULT_BLACKLIST_OUT_BLOCK,
	AUDIO_LANG_MONITOR_OUT,									//hit audio lang monitor service, send monitor
	CONFIG_MONITOR_OUT,											//hit config monitor service,send monitor

	/*dumpfile*/
	MONITOR_DUMP_FILE,

	/*qd_ack*/
	RESP_CHARACTER_ACK,
	
	/*to_send ~~ DEDUP_DROP+SRC_SEND*/
	TO_SEND,
	
	FRAG_DEDUP,

	/*dedup drop*/
	DEDUP_DROP,
	
	/*wait drop*/
	WAIT_QUEUE_FULL_DROP,

	/*send data*/	
	SRC_SEND_LOCAL,
	SRC_SEND_CPZ,
	SRC_WINS_SEND,	

	/*json kafka*/
	MEDIA_JSON,
	SURVEY_JSON,
	
	LOG_TYPE_MAXNUM,
}LOG_TYPE;

typedef enum
{
	BIZMAN_RECV_QUEUE=0,
	MULTISRC_QUEUE,
	APP_QUEUE,
	DUMPFILE_QUEUE,
	AV_RECORD_QUEUE,
	AV_DIGEST_RECORD_QUEUE,
	MONITOR_HASH,
	DUMPFILE_HASH,
	MEDIA_HASH,

	SYSLOG_TYPE_MAXNUM,	
}SYSLOG_TYPE;

/*----------frag_reassembly---------------*/
typedef enum
{
	RSSB_CREATE_FRAG_UNIT=0, 		//创建传输单元	
	RSSB_FREE_FRAG_UNIT,
	RSSB_FRAG_UNIT_EXIST,
	RSSB_FRAG_UNIT_ERROR,
	RSSB_FRAG_FORECAST,
	RSSB_FRAG_FORECAST_OK,

	RSSB_CNVG_ONCE_QUERY_SEND,					
	RSSB_CNVG_TWICE_QUERY_SEND,					
 	RSSB_CNVG_ONCE_QUERY_RECV,	
 	RSSB_CNVG_TWICE_QUERY_RECV,					
	RSSB_INDEX_ONCE_QUERY_SEND,					
	RSSB_INDEX_TWICE_QUERY_SEND,					
	RSSB_INDEX_ONCE_QUERY_RECV,		
	RSSB_INDEX_TWICE_QUERY_RECV,	

	RSSB_AV_ONCE_QUERY_SEND,					
	RSSB_AV_TWICE_QUERY_SEND, 				
 	RSSB_AV_ONCE_QUERY_RECV,	
 	RSSB_AV_TWICE_QUERY_RECV,	

	RSSB_SIP_ACK_KEY1,					
	RSSB_SIP_ACK_KEY2, 				
 	RSSB_SIP_ACK_KEY3,	

	RSSB_LOG_TYPE_MAXNUM,
}RSSB_LOG_TYPE;

typedef enum
{		
	RSSB_RECV_OTHER_MEDIA, 		//not HLS and osmf
	RSSB_RECV_HLS_MEDIA,	
	RSSB_HLS_TO_OTHER,			//是HLS碎片化音视频，但是无法完成关联，普通音视频处理
	RSSB_RECV_OSMF_MEDIA,		
	RSSB_OSMF_TO_OTHER,			//是OSMF碎片化音视频，但是无法完成关联，普通音视频处理
	RSSB_RECV_FRAG_MEDIA,		
	RSSB_FRAG_TO_OTHER,			//除了HLS OSMF的碎片
	
	RSSB_DATALOG_TYPE_MAXNUM,
}RSSB_DATALOG_TYPE;

typedef enum
{
	RSSB_CNVG_QUEUE,
	RSSB_INDEX_QUEUE,
	RSSB_WAIT_QUEUE,

	RSSB_CNVG_HASH,
	
	RSSB_SYSLOG_TYPE_MAXNUM,
}RSSB_SYSLOG_TYPE;

typedef enum
{
	TOTAL_PKTS=0,
	TOTAL_BYTES,
	PPS,
	BPS,
	

	FAIL_PKTS,
	FAIL_BYTES,
	FAIL_PPS,
	FAIL_BPS,	

	LOG_STAT_MAXNUM,
}LOG_STAT;

typedef enum
{
	NUM_CUR=0,
	NUM_IN,
	NUM_OUT,

	SYSLOG_STAT_MAXNUM,
}SYSLOG_STAT;

typedef enum
{
    QUEUE_CURRENT=0,
    QUEUE_IN,
    QUEUE_OUT,

    QUEUELOG_STAT_MAXNUM,
}QUEUELOG_STAT;

typedef enum
{
    HASH_CURRENT=0,
    HASH_NUM_EXPIRE,
    HASH_TIME_EXPIRE,

    HASHLOG_STAT_MAXNUM,
}HASHLOG_STAT;

void* thread_frag_rssb_sysinfo_output(void *param);
void* thread_frag_rssb_stat_output(void *param);

void* thread_sysinfo_output(void *param);
void* thread_stat_output(void *param);


#endif
