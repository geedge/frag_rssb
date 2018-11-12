#ifndef _MAIN_H
#define _MAIN_H

#include <sys/un.h>
#include </usr/include/stdint.h>

#define	APP_FUNC														0  //手机APP
#define	VOIP_FUNC														1  //VOIP 主要是配置文件不一致
#define	PIC_FUNC														0  //PIC 主要是配置文件不一致

#include "MESA_list_queue.h"
#include "MESA_htable.h"
#include "MESA_trace.h"
#include "MESA_timer.h"
#include "usm_api.h"
#include "Maat_rule.h"
#include "KafkaProducer.h"

#include "common.h"
#include "log.h"
#include "AV_interface.h"
#include "wiredLB.h"

/*本程序支持的业务类型*/
#define CPZ_AV_PIC	0
#define CPZ_VOIP	1

#define	MAX_THREAD_NUM													32

/*sned data;  send wins;  send whitelist*/
#define	DEST_MAXNUM														8
/*special media to windows*/
#define SPECIAL_MEDIA_TYPE_MAXNUM										16	//in main.conf
#define SPECIAL_MEDIA_TABLE_MAXNUM										256 //special media type is 0Xxx, less than 256

/*节目确认缓存大小*/
#define ACK_BUF_SIZE 													1500

typedef struct frag_rssb_parameter_s
{	
  	void*						asmis_log_handle;
	/*data msg rec*/
	void*						recv_bizman[MAX_THREAD_NUM];	
	void*						answer_sapp_bizman;	   				//send msg to sapp
	void*						cpz_send_bizman;					//send data to other cpz when multi
	void*						appdtc_handle;	   				    //send msg to sapp
	
	Maat_feather_t              feather;

	USM_t* 						a_usm_handle;
	unsigned int 				reader_cnt;
	int 						usm_on_flag;
	
	MESA_lqueue_head*  			recv_bizman_lq; 					//bizman recv frag queue, queue num=thread_num
	MESA_lqueue_head*  			monitor_file_lq;					//monitor service dump file
	MESA_lqueue_head*  			av_record_lq;						//av record file	
	MESA_lqueue_head*  			av_digest_record_lq;				//av fuzzy_record file	
	MESA_lqueue_head  			app_lq; 							//app queue

	MESA_htable_handle      	media_monitor_hash;					//media_monitor_hash because full_file monitoe	
	MESA_htable_handle      	dumpfile_hash;						//monitor dumpfile hash handle	
	MESA_htable_handle			media_hash; 						//media_hash	

	MESA_timer_t*				index_query_timer[MAX_THREAD_NUM]; 	
	char						index_query_timer_on[MAX_THREAD_NUM]; 	

	/*多源降载业务*/
	void*						dedup_hd;							//av_dedup_handle
	void*						dedup_logger;						//av_dedup log
	MESA_timer_t*				multisrc_timer[MAX_THREAD_NUM]; 	
	char						multisrc_timer_on[MAX_THREAD_NUM]; 	
	
	KafkaProducer*				kafka_producer; 					//kafka

	/*log*/
	FILE*						media_create_file;					//media create log
	FILE*						media_expire_file;					//media expire log
	FILE*						resp_file;							//survey log

	pthread_mutex_t				media_create_file_lock;
	pthread_mutex_t				media_expire_file_lock;
	pthread_mutex_t				resp_file_lock;
	
	struct tm					media_create_filetime;
	struct tm					media_expire_filetime;
	struct tm					resp_filetime;
	time_t						cur_time;
	void*						logger;		
	void*						mid_trace_hd;						//pid trace log handle	
	void*						frag_logger;						//about frag
	void*						voip_logger;						//about dedup

	/*send data*/
	int 						send_fd[MAX_THREAD_NUM];        	//unix socket : send frag		
	int 						send_sd[MAX_THREAD_NUM];			//udp socket : 
	
	/*special media type*/
	char						special_media_table[SPECIAL_MEDIA_TABLE_MAXNUM];

	/*send data to windows system*/
	int 						send_windows_sd[MAX_THREAD_NUM];		//udp socket : special media send to windows system
	/*resp msg*/
	int							recv_msg_fd;	             			//unix socket : response msg recv
	int							recv_msg_sd;							//udp socket : response msg recv	
	int							send_msg_sd;							//udp socket : response msg send	, send whitelist

	int							frag_loglevel;						//because cal IP,		

	/*feedback to qd*/
	msg_data_ack_t*				fb_ack_hdr[MAX_THREAD_NUM];	
	char						fb_ack_buf[MAX_THREAD_NUM][ACK_BUF_SIZE];

	/*av record*/
	FILE*						av_record_curfile[MAX_THREAD_NUM];
 	uint64_t					av_record_curcnt[MAX_THREAD_NUM];

	/*av fuzzy record*/
	FILE*						av_digest_record_curfile[MAX_THREAD_NUM];
 	uint64_t					av_digest_record_curcnt[MAX_THREAD_NUM];

    short                       expr_tableid;    	
}frag_rssb_parameter_t;

typedef struct frag_rssb_configure_s
{		
	char						store_filepath[MAX_PATH_LEN];	
	char						trace_filepath[MAX_PATH_LEN];
	char						save_media_path[MAX_PATH_LEN];
	char						media_create_filename[MAX_PATH_LEN];	//create media
	char						media_expire_filename[MAX_PATH_LEN];	//expire media	
	char						resp_filename[MAX_PATH_LEN];				//about resp msg and monitor and block
	
	/*media hash*/
	uint32_t					media_hash_size;
	uint32_t					media_hash_max_elem_num;
	uint32_t					media_hash_expire_time;
	/*deduptd hash*/
	uint32_t					deduptd_hash_size;
	uint32_t					deduptd_hash_max_elem_num;
	uint32_t					deduptd_hash_expire_time;
	
	/*dumpfile hash*/
	uint32_t           			dumpfile_hash_size;
    uint32_t           			dumpfile_hash_max_elem_num;	
    uint32_t       				dumpfile_hash_expire_time;
	/*monitor hash*/
	uint32_t           			monitor_hash_size;
    uint32_t           			monitor_hash_max_elem_num;	
    uint32_t       				monitor_hash_expire_time;
	
	/*monitor service : save file*/
	char						monitor_file_root_dir[MAX_PATH_LEN];
	uint16_t					monitor_file_switch;
	uint16_t					monitor_file_days;	
	int16_t						fuzzy_digest_switch;	
	int16_t						modify_capIP_switch;	
	
	uint16_t					bizman_port;				//recv_bizman port, recv frag from sapp
	uint16_t					bizman_ack_port;			//answer_sapp_bizman port, send msg to sapp 	
	uint16_t					msg_port;		
	uint16_t					store_filepath_switch;

	uint16_t 					renew_time_min;
	uint16_t					renew_time_max;
	uint16_t					renew_time_step;
	/*send_msg_windows_sd , special media to windocs system*/	
	uint16_t					special_media_fwd_switch;        // is deputy system data like MP4 send to  remote windows  system 
	uint32_t					special_media_wins_ip_num;
	uint32_t					special_media_wins_port_num;
	in_addr_t					special_media_wins_ip[DEST_MAXNUM];
	in_port_t					special_media_wins_port[DEST_MAXNUM];	

	/*send_msg_sd*/
	struct sockaddr_in 			whitelist_addr[DEST_MAXNUM];	
	uint32_t					whitelist_addr_num;	

	/*send data by unix*/
	uint32_t					send_dest_addr_num;						//unix socket : send frag to dest	
	struct sockaddr_un			send_dest_addr[DEST_MAXNUM];			//unix socket : send frag to dest

	/*send data by udp*/
	uint16_t					send_dest_udp_port[DEST_MAXNUM];						//udp socket : send frag to dest	
	uint32_t					send_dest_udp_ip_num;
	in_addr_t					send_dest_udp_iplist[DEST_MAXNUM];		//udp socket : send frag to dest	

	/*avrecord*/	
	char						avrecord_filepath[MAX_PATH_LEN];	
	uint32_t					avrecord_maxnum;		
	uint32_t 					local_ip_nr;

	uint32_t					thread_num;		
	uint16_t					bizman_queue_mode;    	//0:无队列接收 1:队列缓冲		
	uint16_t					avrecord_switch;	
	uint16_t					forecast_switch;
	uint32_t					bizman_queue_maxnum;    //bizman 队列的最大值
	/*降载多源业务*/
    uint32_t       				td_data_maxsize;	
	int16_t						av_dedup_switch;
	uint32_t 					multisrc_wait_timeout;	
	uint32_t 					index_query_timeout;	
	int16_t						multisrc_timer_cb_maxtime;	
	int16_t						index_query_timer_cb_maxtime;	
	int16_t						dedup_invalid;	  //0:正常处理 1:重复不上报  2:	
	/*debug*/
	int16_t						all_hit_monitor_switch;
	int16_t                     all_hit_monitor_complete_rate;	
	int16_t                     all_hit_filename; 		//根据meidia_type命名文件后缀
	int16_t						hls_in_aboffset_mode;		
	int16_t						json_local_switch;	
	int16_t						frag_survey_invalid;			
	int16_t						ack_switch;	
	/*APP业务*/
	int16_t 					app_switch;
	
	int16_t						IVI_switch;	
	int16_t						asmis_switch;		
	int16_t						cpz_type;	

	uint32_t*    				voip_survey_log_iplist;   /*VOIP的监测日志接收IP*/
	uint16_t					voip_survey_log_port;				
	uint16_t					voip_survey_log_ipnum;	
	uint16_t					voip_full_log_port;				
	uint16_t					voip_full_log_ipnum;	
	uint32_t*    				voip_full_log_iplist;     /*VOIP的全量日志接收IP*/	
	
	int16_t						voip_filter_switch;		  /*VOIP根据media_type进行过滤*/

	char 						kafka_brokers[512];   
	int16_t 					media_json_switch;	
	uint32_t					fwd_ip_nr;
//WLB
	char						wlb_topic[WLB_MAX_TAG_SIZE];
	char						wlb_group_name[WLB_MAX_TAG_SIZE];
	char						user_tag[WLB_MAX_TAG_SIZE];
	WLB_handle_t 				rssb_wlb_handle;
	uint16_t					wlb_on;
	uint16_t					health_check_interval;
	uint16_t					data_port;
	uint32_t					capacity;
	uint32_t					cost;
	uint32_t					wlb_report_interval;
	uint32_t   					enable_override;                                     	
	uint32_t                    health_check_port;
	int16_t						save_media;
	
	uint32_t                    bfd_recv_port;
}frag_rssb_configure_t;

typedef struct frag_rssb_status_s
{	
	void*						fs_handle;	
	void*						sysfs_handle;	
 	uint32_t					stat_interval; 	
 	uint32_t					sysinfo_interval; 
	
	uint64_t					stat_info[LOG_TYPE_MAXNUM][LOG_STAT_MAXNUM];	
	uint64_t					send_stat[DEST_MAXNUM][LOG_STAT_MAXNUM];
	uint64_t					send_lq_stat[DEST_MAXNUM][LOG_STAT_MAXNUM];//add by dumeijie
	uint64_t					wins_send_stat[DEST_MAXNUM][LOG_STAT_MAXNUM];
	uint64_t					whitelist_send_stat[DEST_MAXNUM][LOG_STAT_MAXNUM];	
	uint64_t					media_stat[MEDIALOG_TYPE_MAXNUM];
	uint64_t					sysinfo_stat[SYSLOG_TYPE_MAXNUM][SYSLOG_STAT_MAXNUM];

	/*stat*/	
	int							log_column_id[LOG_STAT_MAXNUM];	
	int							log_line_id[LOG_TYPE_MAXNUM];
	int							sendlog_line_id[DEST_MAXNUM*2];
	int							wins_sendlog_line_id[DEST_MAXNUM];
	int							whitelist_sendlog_line_id[DEST_MAXNUM];		
	int							log_field_id[MEDIALOG_TYPE_MAXNUM];	//media_stat
	/*sysinfo*/	
	int							syslog_column_id[SYSLOG_STAT_MAXNUM];	
	int							syslog_line_id[SYSLOG_TYPE_MAXNUM];	

	/*field_stat*/
	char						fs_app[32];
	char						fs_ip[32];
	uint16_t					fs_port;
	uint16_t					fs_remote_switch;
}frag_rssb_status_t;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
